#include "util.h"
#include <iomanip>
#include <sstream>

string hexDump(bytes bs) {
  static char HEX[] = "0123456789ABCDEF";
  string out;
  for (uint8_t b : bs) {
    out += HEX[b >> 4];
    out += HEX[b & 0xF];
    out += ' ';
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