#ifndef _UTIL_H_
#define _UTIL_H_
#include <string>
#include <vector>
using namespace std;
typedef vector<uint8_t> bytes;
string hexDump(bytes );
string charDump(bytes);
#endif