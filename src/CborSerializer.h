#ifndef E04F2BFB_223A_4990_A609_B7AA6A5E6BE8
#define E04F2BFB_223A_4990_A609_B7AA6A5E6BE8

#include <Log.h>
#include <cbor.h>
#include <util.h>

#include <assert>
#include <iostream>
#include <sstream>
using namespace std;

class CborSerializer {
  uint32_t _capacity;
  size_t _size;
  CborError _err;
  CborEncoder _encoderRoot;
  CborEncoder _encoder;
  uint8_t *_buffer;

 public:
  CborSerializer(int size);
  CborSerializer &member(const char *name, const int &t, const char *desc);
  CborSerializer &member(const char *name, int &t, const char *desc);
  CborSerializer &member(const char *name, uint64_t &t, const char *desc);
  CborSerializer &member(const char *name, int64_t &t, const char *desc);
  CborSerializer &member(const char *name, string &t, const char *desc);
  CborSerializer &member(const char *name, double &t, const char *desc);
  CborSerializer &member(const char *name, bytes &t, const char *desc);
  CborSerializer &begin();
  CborSerializer &end();
  bytes toBytes();
  ~CborSerializer();
};
#endif /* E04F2BFB_223A_4990_A609_B7AA6A5E6BE8 */
