#include <util.h>

#include <sstream>
using namespace std;
class Display {
  stringstream _ss;

 public:
  template <typename T>
  Display &member(const char *name, T &t, const char *desc) {
    _ss << name << ":" << t << ",";
    return *this;
  }
  Display &member(const char *name, bytes &t, const char *desc) {
    _ss << name << ":" << hexDump(t) << ",";
    return *this;
  }

  template <typename T>
  Display &ro(const char *name, const T &t, const char *desc) {
    _ss << name << ":" << t << ",";
    return *this;
  }
  Display &ro(const char *name, const bytes &t, const char *desc) {
    _ss << name << ":" << hexDump(t) << ",";
    return *this;
  }
  string toString() {
    _ss << ends;
    return _ss.str();
  }
  Display &operator()() {
    stringstream().swap(_ss);
    return *this;
  }
};