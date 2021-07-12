
#include "tcp_session.h"

TcpSession::TcpSession(Thread &thread, Config config)
    : Actor(thread),
      outgoing(
          10,
          [&](const bytes &data) {
            int rc = send(_socket, data.data(), data.size(), MSG_NOSIGNAL);
            if ( rc < 0 ) WARN("TCP recv() %d %d:%s", rc, errno, strerror(errno));
          },
          "tcp.outgoing") {
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
  thread().deleteInvoker(_socket);
  return true;
}
// RXD handler
void TcpSession::invoke() {
  uint8_t data[512];
  size_t len = 512;
  bytes rxd;
  rxd.reserve(512);
  INFO(" TCP RXD ");
  int rb = recv(_socket, data, 512, 0);
  if (rb > 0) {
    bytes msg(data, data + rb);
    INFO("TCP RXD %s ",hexDump(msg).c_str());
    incoming.emit(msg);
  }
  else if (rb < 0)
    WARN("TCP recv() %d %d:%s", rb, errno, strerror(errno));
  else {
    WARN("TCP recv()=0 ");
  }
}

void TcpSession::onError() {
  WARN("onError() invoked");
  disconnect();
}