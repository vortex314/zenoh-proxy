#include <ppp_frame.h>
#include <serial_session.h>

SerialSession::SerialSession(Thread &thread, Config config)
    : Actor(thread),
      outgoing(10, [&](const bytes &data) { _serialPort.txd(data); }) {
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
  _serialPort.disconnect();
  return true;
}

void SerialSession::invoke() {
  INFO(" reading data");
  int rc = _serialPort.rxd(_rxdBuffer);
  if (rc == 0) {                  // read ok
    if (_rxdBuffer.size() == 0) { // but no data
      WARN(" 0 data ");
    } else {
      handleRxd(_rxdBuffer);
      _rxdBuffer.clear();
    }
  }
}

void SerialSession::handleRxd(bytes &bs) {
  for (uint8_t b : bs) {
    if (b == PPP_FLAG_CHAR) {
      _lastFrameFlag = Sys::millis();
      handleFrame(_inputFrame);
      _inputFrame.clear();
    } else {
      _inputFrame.push_back(b);
    }
  }
  if ((Sys::millis() - _lastFrameFlag) > _frameTimeout) {
    //   cout << " skip  bytes " << hexDump(bs) << endl;
    //   cout << " frame data drop " << hexDump(frameData) << flush;
    toStdout(bs);
    _inputFrame.clear();
  }
}

bool SerialSession::handleFrame(bytes &bs) {
  if (bs.size() == 0)
    return false;
  if (ppp_deframe(_cleanData, bs)) {
    incoming = _cleanData;
    return true;
  } else {
    toStdout(bs);
    return false;
  }
}

void SerialSession::toStdout(bytes &bs) {
  if (bs.size()) {
    //  cout << hexDump(bs) << endl;
    fwrite("\033[32m", 5, 1, stdout);
    fwrite(bs.data(), bs.size(), 1, stdout);
    fwrite("\033[39m", 5, 1, stdout);
  }
}

void SerialSession::onError() { disconnect(); }

int SerialSession::fd() { return _serialPort.fd(); }