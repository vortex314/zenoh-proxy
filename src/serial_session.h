#ifndef _SESSION_SERAIL_H_
#define _SESSION_SERIAL_H_
#include <limero.h>
#include <serial.h>
#include <util.h>
#include <listener.h>
typedef enum { CMD_OPEN, CMD_CLOSE } TcpCommand;

class SerialSession : public Actor,public Listener {
  int _serialfd;
  Serial _serialPort;
  string _port;

public:
  ValueSource<bytes> incoming;
  Sink<bytes> outgoing;
  ValueSource<bool> connected;
  ValueSource<TcpCommand> command;
  SerialSession(Thread &thread, Config config) : Actor(thread), outgoing(10) {}
  bool init();
  bool connect();
  bool disconnect();
  bool send(bytes);
  bool sendEvent(uint8_t);
  int sendFrame(uint8_t header ,bytes& data) ;
  void onError();
  void onRxd();
  int fd();

};
#endif