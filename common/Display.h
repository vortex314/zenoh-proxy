#include <util.h>

#include <sstream>
using namespace std;
class Display {
  stringstream _ss;

 public:
  template <typename T>
  Display &member( T &t,const char *name, const char *desc) {
    _ss << name << ":" << t << ",";
    return *this;
  }
  Display &member( bytes &t,const char *name, const char *desc) {
    _ss << name << ":" << hexDump(t) << ",";
    return *this;
  }
  string toString() {
    _ss << ends;
    return _ss.str();
  }
  Display &begin() {
    stringstream().swap(_ss);
    return *this;
  }
    Display &end() {
    return *this;
  }
};