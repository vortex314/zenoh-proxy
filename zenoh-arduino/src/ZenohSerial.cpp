
#include <ZenohSerial.h>

#include <util.h>

ZenohSerial::ZenohSerial(Thread &thr, Stream &serial)
    : Actor(thr), _serial(serial), keepAliveTimer(thr, 3000, true),
      connectTimer(thr, 2000, true)
{
  _toFrame >> [&](const bytes &bs)
  {
    _serial.write(bs.data(), bs.size());
  };

  serialRxd >> _bytesToFrame >> _frameToCbor;
  // ZENOH feedbacks
  _frameToCbor >> CborFilter::nw(Z_PUBLISH) >> publishReceived;
  _frameToCbor >> CborFilter::nw(Z_RESOURCE) >> resourceIdReceived;
  publishReceived >> [&](const cbor &cb)
  {
    string resource = cb.to_array()[1];
    if (resource == _loopbackTopic)
    {
      bytes data = cb.to_array()[2];
      uint64_t ts = cbor::decode(data);
      publish(H("src/esp32/system/latency"),Sys::millis() - ts);
      publish("src/esp32/system/latency", Sys::millis() - ts);
    }
  };
}
ZenohSerial::~ZenohSerial() {}

void ZenohSerial::init()
{

  // Sink<TimerMsg, 3> &me = *this;
  keepAliveTimer >> [&](const TimerMsg &tm)
  {
    //   if (connected())
    publish(_loopbackTopic, Sys::millis());
  };
  connectTimer >> [&](const TimerMsg &tm)
  {
    if (Sys::millis() > (_loopbackReceived + 3000))
    {
      connected = false;
      subscribe(_dstPrefix + "**");
      resourceId(_dstPrefix);
    }
    else
    {
      connected = true;
    }
    publish(_loopbackTopic, Sys::millis());
  };
  _serial.setTimeout(0);
}

void ZenohSerial::onRxd(void *me)
{
  ZenohSerial *zenohSerial = (ZenohSerial *)me;
  bytes data;
  while (zenohSerial->_serial.available())
  {
    data.push_back(zenohSerial->_serial.read());
  }
  zenohSerial->serialRxd.emit(data);
}

void ZenohSerial::subscribe(string topic)
{
  _toFrame.on(cbor::array{Z_SUBSCRIBE, topic});
}

void ZenohSerial::resourceId(string topic)
{
  _toFrame.on(cbor::array{Z_RESOURCE, topic});
}
