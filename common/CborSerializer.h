#ifndef E04F2BFB_223A_4990_A609_B7AA6A5E6BE8
#define E04F2BFB_223A_4990_A609_B7AA6A5E6BE8

#include <Log.h>
#include <cbor.h>
#include <util.h>

#include <assert.h>
#include <iostream>
#include <sstream>
using namespace std;

class CborSerializer
{
  uint32_t _capacity;
  size_t _size;
  CborError _err;
  CborEncoder _encoderRoot;
  CborEncoder _encoder;
  uint8_t *_buffer;

public:
  CborSerializer(int size);
  CborSerializer &member(const int &t, const char *n = "", const char *d = "");
  CborSerializer &member(int &t, const char *n = "", const char *d = "");
//  CborSerializer &member(int32_t &t, const char *n = "", const char *d = "");
  CborSerializer &member(uint32_t &t, const char *n = "", const char *d = "");
  CborSerializer &member(int64_t &t, const char *n = "", const char *d = "");
  CborSerializer &member(uint64_t &t, const char *n = "", const char *d = "");
  CborSerializer &member(string &t, const char *n = "", const char *d = "");
  CborSerializer &member(float &t, const char *n = "", const char *d = "");
  CborSerializer &member(double &t, const char *n = "", const char *d = "");
  CborSerializer &member(bytes &t, const char *n = "", const char *d = "");
  CborSerializer &begin();
  CborSerializer &end();
  bytes toBytes();
  ~CborSerializer();
};
#endif /* E04F2BFB_223A_4990_A609_B7AA6A5E6BE8 */
