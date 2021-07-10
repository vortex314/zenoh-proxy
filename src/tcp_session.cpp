
#include "tcp_session.h"

bool TcpSession::connect() {
  const char *locator = "tcp/localhost:7447";
  INFO(" open tcp %s", locator);
  _zn_socket_result_t result = _zn_open_tx_session(locator);
  if (result.tag == _z_res_t_OK) {
    INFO(" tcp open OK ");
    _socket = result.value.socket;
    connected = true;
    return false;
  } else {
    INFO(" tcp open failed");
    connected = false;
    return true;
  }
}

bool TcpSession::disconnect() {
  INFO(" tcp close ");
  _zn_close_tx_session(_socket);
  connected = false;
  return true;
}

void TcpSession::onRxd() {}

void TcpSession::onError() { disconnect(); }