#ifndef ZENOHSERIAL_H
#define ZENOHSERIAL_H
#include <ArduinoJson.h>
#include <cbor11.h>
#include <limero.h>
#include <ppp_frame.h>
#include <serial_protocol.h>
#include <string>

using namespace std;

typedef vector<uint8_t> bytes;

#define QOS 0
#define TIMEOUT 10000L
#define TIMER_KEEP_ALIVE 1
#define TIMER_CONNECT 2
#define TIMER_SERIAL 3

//=================================================================
class BytesToCbor : public LambdaFlow<bytes, cbor>
{
public:
  BytesToCbor()
      : LambdaFlow<bytes, cbor>([](cbor &msg, const bytes &data)
                                {
                                  msg = cbor::decode(data);
                                  //          INFO(" msg %s", cbor::debug(msg).c_str());
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
class FrameExtractor : public Flow<bytes, bytes>
{
  bytes _inputFrame;
  bytes _cleanData;
  uint64_t _lastFrameFlag;
  uint32_t _frameTimeout = 1000;

public:
  FrameExtractor() : Flow<bytes, bytes>() {}
  void on(const bytes &bs) { handleRxd(bs); }
  void toStdout(const bytes &bs)
  {
    if (bs.size())
    {
      // Serial.println(bs.data(),bs.size());
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
class FrameGenerator : public LambdaFlow<cbor, bytes>
{
public:
  FrameGenerator()
      : LambdaFlow<cbor, bytes>([&](bytes &out, const cbor &in)
                                {
                                  out = ppp_frame(cbor::encode(in));
                                  return true;
                                }){};
};

class ZenohSerial : public Actor
{
  Stream &_serial;
  ValueSource<bool> connected;
  ValueSource<bytes> incoming;
  ValueSource<bytes> serialRxd;
  ValueFlow<cbor> publishReceived;
  ValueFlow<cbor> resourceIdReceived;

  BytesToCbor _frameToCbor;
  FrameExtractor _bytesToFrame;
  FrameGenerator _toFrame;

  string _loopbackTopic = "dst/esp32/system/loopback";
  string _dstPrefix = "dst/esp32/";
  uint64_t _loopbackReceived;

private:
  template <typename T>
  void publish(string topic, T t)
  {
    cbor msg = cbor::array{Z_PUBLISH, topic, cbor::encode(t)};
    _toFrame.on(msg);
    resourceId(topic);
  }
  template <typename T>
  void publish(uint32_t rid, T t)
  {
    cbor msg = cbor::array{Z_PUBLISH, rid, cbor::encode(t)};
    _toFrame.on(msg);
  }
  void subscribe(string topic);
  void resourceId(string topic);

public:
  static void onRxd(void *);
  TimerSource keepAliveTimer;
  TimerSource connectTimer;
  ZenohSerial(Thread &thr, Stream &serial);
  ~ZenohSerial();
  void init();
};

template <typename T>
class ToTopic : public Sink<T>
{
public:
  string name;
  uint32_t id = 0;
  ZenohSerial &_zenohSerial;
  ToTopic(ZenohSerial &zenohSerial) : Sink<T>(3), _zenohSerial(zenohSerial){};
  void on(const T &t)
  {
    if (id)
      _zenohSerial.publish(id, t);
    else
      _zenohSerial.publish(name, t);
  }
};

#endif // ZenohSERIAL_H
