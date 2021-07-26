#include <CborDeserializer.h>
CborDeserializer::CborDeserializer(uint8_t *buffer, size_t size) {
  _buffer = new uint8_t[size];
  _capacity = size;
}
CborDeserializer &CborDeserializer::begin() {
  _err = 0;
  _err = cbor_parser_init(_buffer, _size, 0, &_decoder, &it)
};
CborDeserializer &CborDeserializer::end(){};

CborDeserializer &CborDeserializer::member(const char *name, bytes &t,
                                           const char *desc) {
  size_t size;
  if (!_err && cbor_value_is_byte_string(&_it) &&
      cbor_value_calculate_string_length(&_it, &size) == 0) {
    t.reserve(size);
    _err = cbor_value_copy_byte_string(&_it, t.data(), &size, 0);
    t.resize(size);
  }
  return *this;
}

CborDeserializer &CborDeserializer::member(const char *name, string &t,
                                           const char *desc) {
  size_t size;
  if (!_err && cbor_value_is_text_string(&_it) &&
      cbor_value_calculate_string_length(&_it, &size) == 0) {
    t.reserve(size);
    _err = cbor_value_copy_byte_string(&_it, t.data(), &size, 0);
    t.resize(size);
  }
  return *this;
}

CborDeserializer &CborDeserializer::member(const char *name, int &t,
                                           const char *desc) {
  if (!_err && cbor_value_is_integer(&_it)) {
    _err = cbor_value_get_int(&_it, &t);
  }
  return *this;
}

CborDeserializer &CborDeserializer::member(const char *name, const int &t,
                                           const char *desc) {
  if (!_err) _err = cbor_value_advance(_it);  // don't overwrite t
  return *this;
}

CborDeserializer &CborDeserializer::fromBytes(const bytes &bs) {
  _size = bs.size() < _capacity ? bs.size() : _capacity;
  memcpy(_buffer, bs.data(), _size);
  return *this;
};
bool success() { return _err == 0; };
