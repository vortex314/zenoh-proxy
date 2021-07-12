
#include "tcp_session.h"

TcpSession::TcpSession(Thread &thread, Config config) 
: Actor(thread), outgoing(10){
  _errorInvoker = new TcpSessionError(*this);
}

bool TcpSession::connect() {
  const char *locator = "tcp/localhost:7447";
  INFO(" open tcp %s", locator);
  _zn_socket_result_t result = _zn_open_tx_session(locator);
  if (result.tag == _z_res_t_OK) {
    INFO(" tcp open OK ");
    _socket = result.value.socket;
    thread().addReadInvoker(_socket, this);
    thread().addErrorInvoker(_socket, _errorInvoker);
    connected = true;
    return false;
  } else {
    INFO(" tcp open failed");
    connected = false;
    thread().deleteInvoker(_socket);
    return true;
  }
}

bool TcpSession::disconnect() {
  INFO(" tcp close ");
  _zn_close_tx_session(_socket);
  connected = false;
  return true;
}
// RXD handler
void TcpSession::invoke() {
  INFO(" TCP RXD ");
}

void TcpSession::onError() {
  WARN("onError() invoked");
  disconnect(); 
  }