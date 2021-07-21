
#include <BrokerSerial.h>
#include <util.h>

BrokerSerial::BrokerSerial(Thread &thr, Stream &serial)
    : Broker(thr),
      _serial(serial),
      keepAliveTimer(thr, 3000, true),
      connectTimer(thr, 2000, true)
{
}
BrokerSerial::~BrokerSerial() {}

void BrokerSerial::init()
{
  // outgoing
  connected = false;
  uptimePub = &publisher<uint64_t>("system/uptime");
  latencyPub = &publisher<uint64_t>("system/latency");
  INFO(" %X %X ", uptimePub, latencyPub);
  uptimeSub = &subscriber<uint64_t>(brokerDstPrefix + "system/uptime");
  outgoingCbor >> _toFrame;
//  incomingCbor.async(thread());
//  outgoingCbor.async(thread());

  _toFrame >> [&](const bytes &bs)
  {
    INFO(" ");
    _serial.write(bs.data(), bs.size());
  };

  serialRxd >> _bytesToFrame >> _frameToCbor;
  INFO(" ");
  keepAliveTimer >> [&](const TimerMsg &tm)
  {
    INFO(" %d ", connected());
    if (connected())
      uptimePub->on(Sys::millis());
    else
      _toFrame.on(cbor::array{B_CONNECT, brokerSrcPrefix});
  };
  INFO(" ");

  incomingCbor >> [&](const cbor &in)
  {
    int msgType = in.to_array()[0];
    if (msgType == B_CONNECT)
    {
      connected = true;
      for (auto sub : _subscribers)
      {
        _toFrame.on(cbor::array{B_SUBSCRIBER, sub->key(), sub->id()});
      }
      for (auto pub : _publishers)
      {
        _toFrame.on(cbor::array{B_PUBLISHER, pub->key(), pub->id()});
      }
    }
  };
  INFO(" ");

  *uptimeSub >> [&](const uint64_t &t)
  {
    latencyPub->on(t - Sys::millis());
    _loopbackReceived = Sys::millis();
  };
  INFO(" ");

  connectTimer >> [&](const TimerMsg &tm)
  {
    uint64_t timeSinceLoopback = Sys::millis() - _loopbackReceived;
    connected = timeSinceLoopback < 3000;
  };
  INFO(" ");

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
