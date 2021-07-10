#ifndef _TCP_SESSION_H_
#define _TCP_SESSION_H_
#include "limero.h"
#include "tcp.h"
#include "util.h"

class TcpSession : public Actor {
  int _socket;

public:
  ValueSource<bytes> incoming;
  Sink<bytes> outgoing;
  ValueSource<bool> connected;
  ValueSource<bool> disconnected;
  TcpSession(Thread &thread, Config config) : Actor(thread), outgoing(10){};
  bool connect();
  bool disconnect();
  void onError();
  void onRxd();
  int fd();
};
#endif