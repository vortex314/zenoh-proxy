#include <ppp_frame.h>
#include <serial_session.h>

SerialSession::SerialSession(Thread &thread, Config config)
    : Actor(thread), outgoing(10) {
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

bool SerialSession::sendEvent(uint8_t header) {
  bytes empty;
  return sendFrame(header, empty) == 0;
}

int SerialSession::sendFrame(uint8_t header, bytes &data) {
  bytes buffer;
  buffer.push_back(header);
  for (uint32_t i = 0; i < data.size(); i++)
    buffer.push_back(data[i]);
  bytes outFrame;
  ppp_frame(outFrame, buffer);
  return _serialPort.txd(outFrame);
}

void SerialSession::invoke() {
  bytes rxdBuffer;
  _serialPort.rxd(rxdBuffer);
  // TODO handle data for frames
}

void SerialSession::onError() { disconnect(); }

int SerialSession::fd() { return _serialPort.fd(); }