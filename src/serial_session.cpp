#include <ppp_frame.h>
#include <serial_session.h>

bool SerialSession::init() {
  _port = "/dev/ttyUSB0";
  _serialPort.port(_port.c_str());
  _serialPort.baudrate(115200);
  _serialPort.init();
  return true;
}

bool SerialSession::connect() {
  _serialPort.connect();
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

void SerialSession::onRxd() {
  bytes rxdBuffer;
  _serialPort.rxd(rxdBuffer);
  // TODO handle data for frames
}

void SerialSession::onError() { disconnect(); }

int SerialSession::fd() { return _serialPort.fd(); }