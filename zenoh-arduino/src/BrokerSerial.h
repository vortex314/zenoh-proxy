#ifndef BrokerSerial_H
#define BrokerSerial_H
#include <broker.h>
#include <broker_protocol.h>
#include <limero.h>
#include <Frame.h>
#include <CborDeserializer.h>
#include <CborSerializer.h>
#include <CborDump.h>

#include <string>

using namespace std;

typedef vector<uint8_t> bytes;

#define QOS 0
#define TIMEOUT 10000L
#define TIMER_KEEP_ALIVE 1
#define TIMER_CONNECT 2
#define TIMER_SERIAL 3

//================================================================

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


class BrokerSerial : public broker::Broker
{
  Stream &_serial;
  ValueSource<bytes> serialRxd;
  broker::Publisher<uint64_t> *uptimePub;
  broker::Publisher<uint64_t> *latencyPub;
  broker::Subscriber<uint64_t> *uptimeSub;

  BytesToFrame _bytesToFrame;
  FrameToBytes _frameToBytes;
  CborSerializer _toCbor;
  CborDeserializer _fromCbor;

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
