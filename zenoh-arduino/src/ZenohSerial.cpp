
#include <ZenohSerial.h>

ZenohSerial::ZenohSerial(Thread &thr, Stream &serial)
    : Actor(thr), _serial(serial), keepAliveTimer(thr, 1000, true),
      connectTimer(thr, 1000, true) {
  _toFrame >> [&](const bytes &bs) { _serial.write(bs.data(), bs.size()); };
  serialRxd >> _bytesToFrame >> _frameToCbor;
  // ZENOH feedbacks
  _frameToCbor  >> CborFilter::nw(Z_PUBLISH) >> publishReceived;
}
ZenohSerial::~ZenohSerial() {}

void ZenohSerial::init() {

  // Sink<TimerMsg, 3> &me = *this;
  keepAliveTimer >> [&](const TimerMsg &tm) {
    if (connected())
      publish(_loopbackTopic, Sys::millis());
  };
  connectTimer >> [&](const TimerMsg &tm) {
    if (Sys::millis() > (_loopbackReceived + 3000)) {
      connected = false;
      subscribe(_dstPrefix + "**");
      publish(_loopbackTopic, true);
    } else {
      connected = true;
    }
    publish(_loopbackTopic, true);
  };
  _serial.setTimeout(0);
}

void ZenohSerial::onRxd(void *me) {
  ZenohSerial *zenohSerial = (ZenohSerial *)me;
  bytes data;
  while (zenohSerial->_serial.available()) {
    data.push_back(zenohSerial->_serial.read());
  }
  zenohSerial->serialRxd.emit(data);
}

void ZenohSerial::subscribe(string topic) {
  _toFrame.on(cbor::encode(cbor::array {Z_SUBSCRIBE,topic}));
}
