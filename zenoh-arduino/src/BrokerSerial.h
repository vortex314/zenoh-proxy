#ifndef BrokerSerial_H
#define BrokerSerial_H
#include <ArduinoJson.h>
#include <broker.h>
#include <broker_protocol.h>
#include <cbor11.h>
#include <limero.h>
#include <ppp_frame.h>

#include <string>

using namespace std;

typedef vector<uint8_t> bytes;

#define QOS 0
#define TIMEOUT 10000L
#define TIMER_KEEP_ALIVE 1
#define TIMER_CONNECT 2
#define TIMER_SERIAL 3

//=================================================================
class FrameToCbor : public LambdaFlow<bytes, cbor>
{
public:
  FrameToCbor()
      : LambdaFlow<bytes, cbor>([](cbor &msg, const bytes &data)
                                {
                                  msg = cbor::decode(data);
                                  INFO(" uC RXD CBOR %s", cbor::debug(msg).c_str());
                                  return msg.is_array();
                                }){};
};
//================================================================
class CborFilter : public LambdaFlow<cbor, cbor>
{
  int _msgType;

public:
  CborFilter(int msgType)
      : LambdaFlow<cbor, cbor>([this](cbor &out, const cbor &in)
                               {
                                 out = in;
                                 return in.to_array()[0] == cbor(_msgType);
                               })
  {
    _msgType = msgType;
  };
  static CborFilter &nw(int msgType) { return *new CborFilter(msgType); }
};
//================================================================
class BytesToFrame : public Flow<bytes, bytes>
{
  bytes _inputFrame;
  bytes _cleanData;
  uint64_t _lastFrameFlag;
  uint32_t _frameTimeout = 1000;

public:
  BytesToFrame() : Flow<bytes, bytes>() {}
  void on(const bytes &bs) { handleRxd(bs); }
  void toStdout(const bytes &bs)
  {
    if (bs.size())
    {
      Serial.println("ignoring bytes : " + bs.size());
    }
  }

  bool handleFrame(bytes &bs)
  {
    if (bs.size() == 0)
      return false;
    if (ppp_deframe(_cleanData, bs))
    {
      emit(_cleanData);
      return true;
    }
    else
    {
      toStdout(bs);
      return false;
    }
  }

  void handleRxd(const bytes &bs)
  {
    for (uint8_t b : bs)
    {
      if (b == PPP_FLAG_CHAR)
      {
        _lastFrameFlag = Sys::millis();
        handleFrame(_inputFrame);
        _inputFrame.clear();
      }
      else
      {
        _inputFrame.push_back(b);
      }
    }
    if ((Sys::millis() - _lastFrameFlag) > _frameTimeout)
    {
      //   cout << " skip  bytes " << hexDump(bs) << endl;
      //   cout << " frame data drop " << hexDump(frameData) << flush;
      toStdout(bs);
      _inputFrame.clear();
    }
  }
  void request(){};
};
//=========================================================================
class CborToFrame : public LambdaFlow<cbor, bytes>
{
public:
  CborToFrame()
      : LambdaFlow<cbor, bytes>([&](bytes &out, const cbor &in)
                                {
                                  INFO("uC TXD %s", cbor::debug(in).c_str());
                                  out = ppp_frame(cbor::encode(in));
                                  return true;
                                }){};
};

class BrokerSerial : public broker::Broker
{
  Stream &_serial;
  ValueSource<bytes> serialRxd;
  broker::Publisher<uint64_t> *uptimePub;
  broker::Publisher<uint64_t> *latencyPub;
  broker::Subscriber<uint64_t> *uptimeSub;

  FrameToCbor _frameToCbor;
  BytesToFrame _bytesToFrame;
  CborToFrame _toFrame;

  string _loopbackTopic = "dst/esp32/system/loopback";
  string _dstPrefix = "dst/esp32/";
  uint64_t _loopbackReceived;

public:
  ValueSource<bool> connected;
  static void onRxd(void *);
  TimerSource keepAliveTimer;
  TimerSource connectTimer;
  BrokerSerial(Thread &thr, Stream &serial);
  ~BrokerSerial();
  void init();
};

#endif // BrokerSerial_H
