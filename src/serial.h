#ifndef _SERIAL_H
#define _SERIAL_H

// For convenience
#include <Log.h>
#include <asm-generic/ioctls.h>
#include <fcntl.h>
#include <linux/serial.h>
#include <netdb.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include <deque>
#include <fstream>
#include <map>
#include <vector>

/*

DTR   RTS   ENABLE  IO0  Mode
1     1     1       1     RUN
0     0     1       1     RUN
1     0     0       0     RESET
0     1     1       0     PROGRAM

*/

using namespace std;

typedef vector<uint8_t> bytes;

struct SerialStats {
  uint64_t connectionCount = 0ULL;
  uint64_t connectionErrors = 0ULL;
  uint64_t messagesSent = 0ULL;
};

class Serial {
  string _port;       // /dev/ttyUSB0
  string _portShort;  // USB0
  int _baudrate;
  int _fd = 0;
  fd_set _rfds;
  fd_set _wfds;
  fd_set _efds;
  int _maxFd;

  uint8_t _separator;
  bool _connected = false;
  void setFds();

 public:
  Serial();
  ~Serial();
  // commands
  int init();
  int connect();
  int disconnect();
  int waitForRxd(uint32_t milliSec);
  int rxd(bytes &buffer);
  int txd(const bytes &);

  int modeRun();
  int modeProgram();

  // properties
  int baudrate(uint32_t);
  uint32_t baudrate();
  int port(string);
  const string &port();
  int separator(uint8_t);
  int fd();
  bool connected() { return _connected; }

  const string &portShort(void) const { return _portShort; };
};

#endif
