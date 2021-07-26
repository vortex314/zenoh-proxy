#ifndef AA72A63D_3140_4FF2_BCAF_6F744DBBD62B
#define AA72A63D_3140_4FF2_BCAF_6F744DBBD62B
#include <cbor.h>
#include <util.h>

class CborDeserializer {
  CborParser _decoder;
  CborValue _rootIt,_it;
  CborError _err;
  uint8_t *_buffer;
  size_t _size;
  size_t _capacity;

 public:
  CborDeserializer(size_t size);
  CborDeserializer &begin();
  CborDeserializer &end();
  CborDeserializer &member(bytes &t, const char *n="",const char *d="");
  CborDeserializer &member(string &t, const char *n="",const char *d="");
  CborDeserializer &member(int &t, const char *n="",const char *d="");
  CborDeserializer &member(const int &t, const char *n="",const char *d="");
  CborDeserializer &fromBytes(const bytes &bs);
  bool success();
};
#endif /* AA72A63D_3140_4FF2_BCAF_6F744DBBD62B */
