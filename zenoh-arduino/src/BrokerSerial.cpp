
#include <BrokerSerial.h>
#include <util.h>

BrokerSerial::BrokerSerial(Thread &thr, Stream &serial)
    : Broker(thr),
      _serial(serial),
      keepAliveTimer(thr, 3000, true),
      connectTimer(thr, 2000, true)
{
  uptimePub = &publisher<uint64_t>("system/uptime");
  latencyPub = &publisher<uint64_t>("system/latency");

  outgoingCbor >> _toFrame;
  _toFrame >> [&](const bytes &bs)
  { _serial.write(bs.data(), bs.size()); };

  serialRxd >> _bytesToFrame >> _frameToCbor;
  // BROKER feedbacks
  _frameToCbor >> CborFilter::nw(B_PUBLISH) >> publishReceived;
  _frameToCbor >> CborFilter::nw(B_RESOURCE) >> resourceIdReceived;
  publishReceived >> [&](const cbor &cb)
  {
    string resource = cb.to_array()[1];
    if (resource == _loopbackTopic)
    {
      bytes data = cb.to_array()[2];
      uint64_t ts = cbor::decode(data);
      latencyPub->on(Sys::millis() - ts);
    }
  };
}
BrokerSerial::~BrokerSerial() {}

void BrokerSerial::init()
{
  // Sink<TimerMsg, 3> &me = *this;
  keepAliveTimer >> [&](const TimerMsg &tm)
  {
    //   if (connected())
    uptimePub->on(Sys::millis());
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
    uptimePub->on(Sys::millis());
  };
  _serial.setTimeout(0);
}

void BrokerSerial::onRxd(void *me)
{
  BrokerSerial *brk = (BrokerSerial *)me;
  bytes data;
  while (brk->_serial.available())
  {
    data.push_back(brk->_serial.read());
  }
  brk->serialRxd.emit(data);
}

void BrokerSerial::subscribe(string topic)
{
  _toFrame.on(cbor::array{B_SUBSCRIBER, topic});
}

void BrokerSerial::resourceId(string topic)
{
  _toFrame.on(cbor::array{B_RESOURCE, topic});
}
