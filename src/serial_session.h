#ifndef _SESSION_SERAIL_H_
#define _SESSION_SERIAL_H_
#include <limero.h>
#include <listener.h>
#include <serial.h>
#include <util.h>
typedef enum { CMD_OPEN, CMD_CLOSE } TcpCommand;

class SerialSessionError;

class SerialSession : public Actor, public Invoker {
  SerialSessionError *_errorInvoker;
  int _serialfd;
  Serial _serialPort;
  string _port;

public:
  ValueSource<bytes> incoming;
  Sink<bytes> outgoing;
  ValueSource<bool> connected;
  ValueSource<TcpCommand> command;
  SerialSession(Thread &thread, Config config);
  bool init();
  bool connect();
  bool disconnect();
  bool send(bytes);
  bool sendEvent(uint8_t);
  int sendFrame(uint8_t header, bytes &data);
  void onError();
  int fd();
  void invoke();
};

class SerialSessionError : public Invoker {
  SerialSession &_serialSession;

public:
  SerialSessionError(SerialSession &serialSession)
      : _serialSession(serialSession){};
  void invoke() { _serialSession.onError(); }
};
#endif