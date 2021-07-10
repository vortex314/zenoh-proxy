#ifndef _UTIL_H_
#define _UTIL_H_
#include "ArduinoJson.h"
#include <string>
#include <vector>
using namespace std;

typedef vector<uint8_t> bytes;
typedef JsonObject Config;

string hexDump(bytes);
string charDump(bytes);
#endif