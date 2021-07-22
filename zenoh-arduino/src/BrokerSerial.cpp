
#include <BrokerSerial.h>
#include <util.h>

BrokerSerial::BrokerSerial(Thread &thr, Stream &serial)
    : Broker(thr),
      _serial(serial),
      keepAliveTimer(thr, 3000, true),
      connectTimer(thr, 2000, true) {}
BrokerSerial::~BrokerSerial() {}

void BrokerSerial::init() {
  // outgoing
  connected = false;
  uptimePub = &publisher<uint64_t>("system/uptime");
  latencyPub = &publisher<uint64_t>("system/latency");
  INFO(" %X %X ", uptimePub, latencyPub);
  uptimeSub = &subscriber<uint64_t>(brokerSrcPrefix + "system/uptime");

  serialRxd >> _bytesToFrame >> _frameToCbor;
  _frameToCbor >> incomingCbor;
  serialRxd >> [&](const bytes &bs) { INFO("uC RXD %d ", bs.size()); };
  _frameToCbor >>
      [&](const cbor &cb) { INFO("uC RXD CBOR %s ", cbor::debug(cb).c_str()); };
  outgoingCbor >> _toFrame;
  //  incomingCbor.async(thread());
  //  outgoingCbor.async(thread());

  _toFrame >> [&](const bytes &bs) { _serial.write(bs.data(), bs.size()); };

  keepAliveTimer >> [&](const TimerMsg &tm) {
    INFO(" connected : %s ", connected() ? "true" : "false");
    if (connected())
      uptimePub->on(Sys::millis());
    else
      _toFrame.on(cbor::array{B_CONNECT, brokerSrcPrefix});
  };
  incomingCbor >> filter([&](const cbor &in) {
    int msgType = in.to_array()[0];
    return msgType == B_CONNECT;
  }) >>
      [&](const cbor &in) {
        INFO("uC RXD %s ", cbor::debug(in).c_str());

        connected = true;
        INFO(" subscribers : %d publishers : %d ", _subscribers.size(),
             _publishers.size());
        for (auto sub : _subscribers) {
          _toFrame.on(cbor::array{B_SUBSCRIBER, sub->key(), sub->id()});
        }
        for (auto pub : _publishers) {
          _toFrame.on(cbor::array{B_PUBLISHER, pub->key(), pub->id()});
        }
      };

  uptimeSub >> [&](const uint64_t &t) {
    latencyPub->on(t - Sys::millis());
    _loopbackReceived = Sys::millis();
  };

  connectTimer >> [&](const TimerMsg &tm) {
    uint64_t timeSinceLoopback = Sys::millis() - _loopbackReceived;
    if (timeSinceLoopback > 3000) connected = false;
  };

  _serial.setTimeout(0);
}

void BrokerSerial::onRxd(void *me) {
  BrokerSerial *brk = (BrokerSerial *)me;
  bytes data;
  while (brk->_serial.available()) {
    data.push_back(brk->_serial.read());
  }
  INFO(" data : %d ", data.size());
  brk->serialRxd.emit(data);
}
