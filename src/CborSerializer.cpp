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
  CborSerializer(int size) {
    _buffer = new uint8_t[size];
    _capacity = size;
    //    cbor_encoder_init(&_encoderRoot, _buffer, _capacity, 0);
    //   cbor_encoder_create_array(&_encoderRoot, &_encoder,
    //   CborIndefiniteLength);
  }
  CborSerializer &member(const char *name, const int &t, const char *desc) {
    _err = cbor_encode_int(&_encoder, t);
    assert(_err == 0);
    return *this;
  }
  CborSerializer &member(const char *name, int &t, const char *desc) {
    _err = cbor_encode_int(&_encoder, t);
    assert(_err == 0);
    return *this;
  }
  CborSerializer &member(const char *name, uint64_t &t, const char *desc) {
    _err = cbor_encode_uint(&_encoder, t);
    assert(_err == 0);
    return *this;
  }
  CborSerializer &member(const char *name, int64_t &t, const char *desc) {
    _err = cbor_encode_int(&_encoder, t);
    assert(_err == 0);
    return *this;
  }
  CborSerializer &member(const char *name, string &t, const char *desc) {
    _err = cbor_encode_text_string(&_encoder, t.data(), t.size());
    return *this;
  }

  CborSerializer &member(const char *name, double &t, const char *desc) {
    _err = cbor_encode_double(&_encoder, t);
    assert(_err == 0);
    return *this;
  }

  CborSerializer &member(const char *name, bytes &t, const char *desc) {
    _err = cbor_encode_byte_string(&_encoder, t.data(), t.size());
    assert(_err == 0);
    return *this;
  }

  CborSerializer &begin() {
    cbor_encoder_init(&_encoderRoot, _buffer, _capacity, 0);
    _err = cbor_encoder_create_array(&_encoderRoot, &_encoder,
                                     CborIndefiniteLength);
    assert(_err == 0);
    return *this;
  }

  CborSerializer &end() {
    _err = cbor_encoder_close_container(&_encoderRoot, &_encoder);
    assert(_err == 0);
    _size = cbor_encoder_get_buffer_size(&_encoderRoot, _buffer);
    return *this;
  }

  bytes toBytes() { return bytes(_buffer, _buffer + _size); }
  ~CborSerializer() { delete _buffer; }
};