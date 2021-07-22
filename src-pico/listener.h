#ifndef _LISTENER_H_
#define _LISTENER_H_
#include <limero.h>
#include <set>
#include <unordered_map>

class Listener {
public:
  virtual void onRxd() = 0;
  virtual void onError() = 0;
  virtual int fd() = 0;
};

class Watcher : public Actor {
  std::set<int> _fd;
  std::unordered_map<int, Listener *> _listeners;
  fd_set _rfds;
  fd_set _wfds;
  fd_set _efds;
  int _maxFd;
  uint32_t _timeout;

public:
  Watcher(Thread &thr) : Actor(thr) {
    FD_ZERO(&_rfds);
    FD_ZERO(&_wfds);
    FD_ZERO(&_efds);
    _maxFd = 0;
  }

  void addFd(Listener *listener) {
    int fd = listener->fd();
    _listeners.emplace(fd, listener);
    _fd.insert(fd);
    FD_ZERO(&_rfds);
    FD_ZERO(&_wfds);
    FD_ZERO(&_efds);
    _maxFd = 0;
    for (int fd : _fd) {
      FD_SET(fd, &_rfds);
      FD_SET(fd, &_efds);
      if (_maxFd < fd)
        _maxFd = fd;
    }
    _maxFd += 1;
  }

  void removeFd(Listener *listener) {
    int fd = listener->fd();
    _fd.erase(fd);
    _fd.erase(fd);
    FD_ZERO(&_rfds);
    FD_ZERO(&_wfds);
    FD_ZERO(&_efds);
    _maxFd = 0;
    if (_fd.size())
      for (int fd : _fd) {
        FD_SET(fd, &_rfds);
        FD_SET(fd, &_efds);
        if (_maxFd < fd)
          _maxFd = fd;
      }
  }
  int waitIO(uint32_t msec) {
    struct timeval tv;
    int rc;
    tv.tv_sec = msec / 1000;
    tv.tv_usec = (msec * 1000) % 1000000;
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
      for (int fd : _fd) {
        if (FD_ISSET(fd, &rfds))
          _listeners.find(fd)->second->onRxd();
        if (FD_ISSET(fd, &efds))
          _listeners.find(fd)->second->onError();
      }
    }
    return EIO;
  }
};
#endif