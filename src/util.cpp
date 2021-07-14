#include "util.h"
#include <iomanip>
#include <sstream>
#include <Log.h>

string hexDump(bytes bs,const char* spacer) {
  static char HEX[] = "0123456789ABCDEF";
  string out;
  string_format(out, "[%d]", bs.size());
  for (uint8_t b : bs) {
    out += HEX[b >> 4];
    out += HEX[b & 0xF];
    out += spacer;
  }
  return out;
}

string charDump(bytes bs) {
  string out;
  for (uint8_t b : bs) {
    if (isprint(b))
      out += (char)b;
    else
      out += '.';
  }
  return out;
}