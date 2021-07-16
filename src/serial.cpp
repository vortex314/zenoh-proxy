

#include "serial.h"

#include <Sys.h>

#include <thread>
using namespace std;

typedef struct {
  uint32_t baudrate;
  uint32_t symbol;
} BAUDRATE;

BAUDRATE BAUDRATE_TABLE[] = {
    {50, B50},           {75, B75},           {110, B110},
    {134, B134}, //
    {150, B150},         {200, B200},         {300, B300},
    {600, B600},         {1200, B1200},       {1800, B1800},
    {2400, B2400},       {4800, B4800},       {9600, B9600},       //
    {19200, B19200},     {38400, B38400},     {57600, B57600},     //
    {115200, B115200},   {230400, B230400},   {460800, B460800},   //
    {500000, B500000},   {576000, B576000},   {921600, B921600},   //
    {1000000, B1000000}, {1152000, B1152000}, {1500000, B1500000}, //
    {2000000, B2000000}, {2500000, B2500000}, {3000000, B3000000}, //
    {3500000, B3500000}, {4000000, B4000000}};



#include <iostream>
void logRaw(const string line) {
  cout << line << flush;
  logger.writer()((char *)line.c_str(), line.size());
}

std::string stringify(std::string s) {
  std::string result;
  result += '"';
  result += s;
  result += '"';
  return result;
}

#define USB() logger.application(_port.c_str())
//=====================================================================================
Serial::Serial() {
  _baudrate = B115200;
  _fd = -1;
}

Serial::~Serial() {}
//=====================================================================================
int baudSymbol(uint32_t br) {
  for (uint32_t i = 0; i < sizeof(BAUDRATE_TABLE) / sizeof(BAUDRATE); i++)
    if (BAUDRATE_TABLE[i].baudrate == br)
      return BAUDRATE_TABLE[i].symbol;
  ERROR("connect: baudrate %d  not found, default to 115200.", br);
  return B115200;
}

int baudToValue(int symbol) {
  for (uint32_t i = 0; i < sizeof(BAUDRATE_TABLE) / sizeof(BAUDRATE); i++)
    if (BAUDRATE_TABLE[i].symbol == symbol)
      return BAUDRATE_TABLE[i].baudrate;
  return 0;
}
//=====================================================================================
int Serial::port(string port) {
  _port = port;
  if (_port.find("/dev/tty") == 0) {
    _portShort = _port.substr(8);
  } else {
    size_t pos = _port.rfind('/');
    if (pos == string::npos) {
      _portShort = _port;
    } else {
      _portShort = _port.substr(pos + 1);
    }
  }
  return 0;
}

const string &Serial::port() { return _port; }
//=====================================================================================
int Serial::init() {
  FD_ZERO(&_rfds);
  FD_ZERO(&_wfds);
  FD_ZERO(&_efds);
  _maxFd = 0;
  return 0;
}
//=====================================================================================
int Serial::connect() {
  struct termios options;

  INFO("Connecting to '%s' ....", _port.c_str());
  _fd = ::open(_port.c_str(), O_EXCL | O_RDWR | O_NOCTTY | O_NDELAY);

  if (_fd == -1) {
    ERROR("connect: Unable to open '%s' errno : %d : %s", _port.c_str(), errno,
          strerror(errno));
    _fd = -1;
    return errno;
  }
  if (isatty(_fd) == 0) {
    ERROR("device is not a tty '%s' errno : %d : %s", _port.c_str(), errno,
          strerror(errno));
    close(_fd);
    _fd = -1;
    return errno;
  }
  INFO("open '%s' succeeded.fd=%d", _port.c_str(), _fd);
  //	fcntl(_fd, F_SETFL, FNDELAY);

  if (tcgetattr(_fd, &options) < 0) {
    ERROR("tcgetattr() failed '%s' errno : %d : %s", _port.c_str(), errno,
          strerror(errno));
    close(_fd);
    _fd = -1;
    return errno;
  }
  // setting into RAW mode, no pre-processing
  options.c_iflag &=
      ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
  options.c_oflag &= ~OPOST;
  options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  options.c_cflag &= ~(CSIZE | PARENB);
  options.c_cflag |= CS8;
  options.c_cflag &= ~HUPCL; // avoid DTR drop at close time

  INFO("set baudrate to %d", baudToValue(_baudrate));
  if (cfsetspeed(&options, _baudrate) < 0) {
    ERROR("cfsetipeed() failed '%s' errno : %d : %s", _port.c_str(), errno,
          strerror(errno));
    close(_fd);
    _fd = -1;
    return errno;
  }

  if (tcsetattr(_fd, TCSANOW, &options) < 0) {
    ERROR("tcsetattr() failed '%s' errno : %d : %s", _port.c_str(), errno,
          strerror(errno));
    close(_fd);
    _fd = -1;
    return errno;
  }
  setFds();
  _connected = true;
  modeRun();
  return 0;
}
//=====================================================================================
int Serial::disconnect() {
  if (close(_fd) < 0) {
    WARN("closing serial fd failed => errno : %d : %s", _port.c_str(), errno,
         strerror(errno));
    _fd = -1;
    setFds();
    _connected = false;
    return errno;
  };
  _fd = -1;
  setFds();
  _connected = false;
  return 0;
}
//=====================================================================================
int Serial::rxd(bytes &out) {
  out.clear();
  if (!_connected)
    return ENOTCONN;
  char buffer[1024];
  int rc;
  while (true) {
    rc = read(_fd, buffer, sizeof(buffer));
    if (rc > 0) {
      DEBUG("read() = %d bytes", rc);
      for (int i = 0; i < rc; i++)
        out.push_back(buffer[i]);
    } else if (rc < 0) {
      DEBUG("read returns %d => errno : %d = %s", rc, errno, strerror(errno));
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        return 0;
      return errno;
    } else { // no data
      return 0;
    }
  }
}
//=====================================================================================
int Serial::txd(const bytes &buffer) {
  if (!_connected)
    return ENOTCONN;
  const uint8_t *buf;
  size_t len;
  buf = buffer.data();
  len = buffer.size();
  while (len > 0) {
    int rc = write(_fd, buf, len);
    if (rc == len)
      break;
    if (rc >= 0) {
      INFO(" second part %d", rc);
      buf += rc;
      len -= rc;
      tcdrain(_fd);
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      DEBUG("write(%d,.,%d) failed '%s' errno : %d : %s", _fd, len,
            _port.c_str(), errno, strerror(errno));
      tcdrain(_fd);
    } else {
      WARN("write(%d,.,%d) failed '%s' errno : %d : %s", _fd, len,
           _port.c_str(), errno, strerror(errno));
      return errno;
    }
  }
  fsync(_fd);
  return 0;
}
//=====================================================================================
int Serial::baudrate(uint32_t br) {
  _baudrate = baudSymbol(br);
  return 0;
}

uint32_t Serial::baudrate() { return baudToValue(_baudrate); }
//=====================================================================================
int Serial::fd() { return _fd; }

void Serial::setFds() {
  FD_ZERO(&_rfds);
  FD_ZERO(&_wfds);
  FD_ZERO(&_efds);
  _maxFd = 0;
  if (_fd != -1) {
    FD_SET(_fd, &_rfds);
    FD_SET(_fd, &_efds);
    if (_maxFd < _fd)
      _maxFd = _fd;
  }
  _maxFd += 1;
}

int Serial::waitForRxd(uint32_t timeout) {
  struct timeval tv;
  int rc;
  tv.tv_sec = timeout / 1000;
  tv.tv_usec = (timeout * 1000) % 1000000;
  fd_set rfds = _rfds;
  fd_set efds = _efds;
  rc = select(_maxFd, &rfds, NULL, &efds, &tv);
  if (rc == 0) { // timeout
    DEBUG(" select timeout.");
    return ETIMEDOUT;
  } else if (rc < 0) {
    WARN("select() : error : %s (%d)", strerror(errno), errno);
    return EIO;
  } else if (rc > 0) { // one of the fd was set
    if (FD_ISSET(_fd, &rfds))
      return 0;
  }
  return EIO;
}

void wait() {
  // wait 1 msec
  struct timespec tim;
  tim.tv_sec = 0;
  tim.tv_nsec = 1000000; // 1 ms
  nanosleep(&tim, NULL);
}
//=====================================================================================
int Serial::modeRun() {
  int flags;
  // RESET
  wait();
  flags = TIOCM_DTR;
  int rc = ioctl(_fd, TIOCMBIS, &flags); // set DTR pin
  if (rc)
    WARN("ioctl()= %s (%d)", strerror(errno), errno);
  flags = TIOCM_RTS;
  rc = ioctl(_fd, TIOCMBIC, &flags); // clear RTS pin
  if (rc)
    WARN("ioctl()= %s (%d)", strerror(errno), errno);
  wait();
  // raise RTS & DTR
  flags = TIOCM_RTS;
  rc = ioctl(_fd, TIOCMBIS, &flags); // Set RTS & DTR pin
  if (rc)
    WARN("ioctl()= %s (%d)", strerror(errno), errno);
  wait();
  flags = TIOCM_DTR;
  rc = ioctl(_fd, TIOCMBIS, &flags); // Set RTS & DTR pin
  if (rc)
    WARN("ioctl()= %s (%d)", strerror(errno), errno);
  return rc;
}
//=====================================================================================
int Serial::modeProgram() {
  int flags;
  // RESET
  wait();
  flags = TIOCM_DTR;
  int rc = ioctl(_fd, TIOCMBIS, &flags); // set DTR pin
  if (rc)
    WARN("ioctl()= %s (%d)", strerror(errno), errno);
  flags = TIOCM_RTS;
  rc = ioctl(_fd, TIOCMBIC, &flags); // clear RTS pin
  if (rc)
    WARN("ioctl()= %s (%d)", strerror(errno), errno);
  // PROGRAM mode
  flags = TIOCM_RTS;
  rc = ioctl(_fd, TIOCMBIS, &flags); // set RTS pin
  if (rc)
    WARN("ioctl()= %s (%d)", strerror(errno), errno);
  flags = TIOCM_DTR;
  if (rc)
    WARN("ioctl()= %s (%d)", strerror(errno), errno);
  rc = ioctl(_fd, TIOCMBIC, &flags); // clear DTR pin
  return rc;
}
#include <time.h>

/*
int main(int argc, char **argv) {
  serialUnitTest(argc, argv);
  loopbackTest(argc, argv);
}*/
