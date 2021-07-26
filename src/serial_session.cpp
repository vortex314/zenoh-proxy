#include <ppp_frame.h>
#include <serial_session.h>

SerialSession::SerialSession(Thread &thread, Config config)
    : Actor(thread), outgoing(10, [&](const bytes &data) {
//        INFO("TXD %s => %s", _serialPort.port().c_str(), hexDump(data).c_str());
        _serialPort.txd(data);
      }) {
  _errorInvoker = new SerialSessionError(*this);
}

bool SerialSession::init() {
  _port = "/dev/ttyUSB0";
  _serialPort.port(_port.c_str());
  _serialPort.baudrate(115200);
  _serialPort.init();
  return true;
}

bool SerialSession::connect() {
  _serialPort.connect();
  thread().addReadInvoker(_serialPort.fd(), this);
  thread().addErrorInvoker(_serialPort.fd(), _errorInvoker);
  return true;
}

bool SerialSession::disconnect() {
  thread().deleteInvoker(_serialPort.fd());
  _serialPort.disconnect();
  return true;
}
// on data incoming on filedescriptor
void SerialSession::invoke() {
  int rc = _serialPort.rxd(_rxdBuffer);
  if (rc == 0) {                   // read ok
    if (_rxdBuffer.size() == 0) {  // but no data
      WARN(" 0 data ");
    } else {
      incoming = _rxdBuffer;
    }
  }
}
// on error issue onf ile descriptor
void SerialSession::onError() { disconnect(); }

int SerialSession::fd() { return _serialPort.fd(); }