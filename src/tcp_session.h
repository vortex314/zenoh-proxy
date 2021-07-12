#ifndef _TCP_SESSION_H_
#define _TCP_SESSION_H_
#include "limero.h"
#include "tcp.h"
#include "util.h"

class TcpSessionError;

class TcpSession : public Actor, public Invoker {
  int _socket;
  TcpSessionError *_errorInvoker;

public:
  ValueSource<bytes> incoming;
  Sink<bytes> outgoing;
  ValueSource<bool> connected;
  ValueSource<bool> disconnected;
  TcpSession(Thread &, Config ) ;
  bool connect();
  bool disconnect();
  void onError();
  void invoke();
  int fd();
};

class TcpSessionError : public Invoker {
  TcpSession &_tcpSession;

public:
  TcpSessionError(TcpSession &tcpSession) : _tcpSession(tcpSession){};
  void invoke() { _tcpSession.onError(); }
};
#endif