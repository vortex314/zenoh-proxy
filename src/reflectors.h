#define JSON_NOEXCEPTION
#include "json.hpp"
#include "util.h"
#include <Log.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
using json = nlohmann::json;

class CborSerializer {
  json js;

public:
  template <typename T>
  CborSerializer &member(const char *name, T &t, const char *desc) {
    COUT << " serialize member :" << name << endl;
    js.push_back(t);
    return *this;
  }
  template <typename T>
  CborSerializer &ro(const char *name, const T &t, const char *desc) {
    COUT << " serialize ro :" << name << endl;
    js.push_back(t);
    return *this;
  }
  bytes toBytes() { return json::to_cbor(js); }
  CborSerializer &operator()() {
    js = json::array();
    return *this;
  };
};

class CborDeserializer {
  json js;
  uint32_t _index;
  bool _success;

public:
  template <typename T>
  CborDeserializer &member(const char *name, T &t, const char *desc) {
    COUT << "member '" << name << endl;
    _success = js.size() > _index;
    COUT << js << " index: " << _index << endl;
    t = js[_index++].get<T>();
    COUT << endl;
    return *this;
  }
  CborDeserializer &member(const char *name, bytes &t, const char *desc) {
    COUT << "member bytes '" << name << endl;
    _success = js.size() > _index;
    t = js[_index++].get<bytes>();
    return *this;
  }
  template <typename T>
  CborDeserializer &ro(const char *name, const T &t, const char *desc) {
    COUT << "ro '" << name << endl;
    _index++;
    return *this;
  }
  CborDeserializer &fromBytes(const bytes &bs) {
    js = json::from_cbor(bs);
    _index = 0;
    _success = js.is_array();
    COUT << js << endl;
    return *this;
  };
  bool success() { return _success; };
  CborDeserializer &operator()() {
    js = json::array();
    _index = 0;
    return *this;
  };
};

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