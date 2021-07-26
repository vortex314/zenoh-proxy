#ifndef _UTIL_H_
#define _UTIL_H_
#include <string>
#include <vector>

#include "ArduinoJson.h"
using namespace std;

typedef vector<uint8_t> bytes;
typedef uint8_t byte;
typedef JsonObject Config;

string hexDump(bytes, const char *spacer = " ");
string charDump(bytes);

#define FNV_PRIME 16777619
#define FNV_OFFSET 2166136261

constexpr uint32_t fnv1(uint32_t h, const char *s) {
  return (*s == 0) ? h
                   : fnv1((h * FNV_PRIME) ^ static_cast<uint32_t>(*s), s + 1);
}

constexpr uint32_t H(const char *s) { return fnv1(FNV_OFFSET, s); }

#define COUT cout << " I " << __SHORT_FILE__ << ":" << __LINE__ << "\t| "
#define CERR cerr << " W " << __SHORT_FILE__ << ":" << __LINE__ << "\t| "

#endif