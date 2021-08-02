#include <CborDeserializer.h>
#include <Log.h>

#undef assert
#define assert(expr) \
  if (!(expr)) WARN("%s:%d failed %s ", __FILE__, __LINE__, #expr)

CborDeserializer::CborDeserializer(size_t size) {
  _buffer = new uint8_t[size];
  _capacity = size;
}
CborDeserializer &CborDeserializer::begin() {
  _err = CborNoError;

  _err = cbor_parser_init(_buffer, _size, 0, &_decoder, &_rootIt);
  assert(_err == CborNoError);
  _err = cbor_value_enter_container(&_rootIt, &_it);
  return *this;
};

CborDeserializer &CborDeserializer::end() { return *this; };

CborDeserializer &CborDeserializer::member(bytes &t, const char *name,
                                           const char *desc) {
  size_t size;
  if (!_err && cbor_value_is_byte_string(&_it) &&
      cbor_value_calculate_string_length(&_it, &size) == 0) {
    byte *temp;
    size_t size;
    _err = cbor_value_dup_byte_string(&_it, &temp, &size, 0);
    assert(_err == CborNoError);
    t = bytes(temp, temp + size);
    free(temp);
  }
  _err = cbor_value_advance(&_it);
  return *this;
}

CborDeserializer &CborDeserializer::member(string &t, const char *name,
                                           const char *desc) {
  if (!_err && cbor_value_is_text_string(&_it)) {
    char *temp;
    size_t size;
    _err = cbor_value_dup_text_string(&_it, &temp, &size, 0);
    assert(_err == CborNoError);
    t = temp;
    ::free(temp);
  }
  _err = cbor_value_advance(&_it);
  return *this;
}

CborDeserializer &CborDeserializer::member(int &t, const char *name,
                                           const char *desc) {
  if (!_err && cbor_value_is_integer(&_it)) {
    _err = cbor_value_get_int(&_it, &t);
    assert(_err == CborNoError);
  }
  _err = cbor_value_advance_fixed(&_it);
  return *this;
}

CborDeserializer &CborDeserializer::member(uint64_t &t, const char *name,
                                           const char *desc) {
  if (!_err && cbor_value_is_unsigned_integer(&_it)) {
    _err = cbor_value_get_uint64(&_it, &t);
    assert(_err == CborNoError);
  }
  _err = cbor_value_advance_fixed(&_it);
  return *this;
}

CborDeserializer &CborDeserializer::member(const int &t, const char *name,
                                           const char *desc) {
  int x;
  if (!_err && cbor_value_is_integer(&_it)) {
    _err = cbor_value_get_int(&_it, &x);
    assert(_err == CborNoError);
  }
  _err = cbor_value_advance_fixed(&_it);
  return *this;
}

CborDeserializer &CborDeserializer::fromBytes(const bytes &bs) {
  assert(bs.size() < _capacity);
  _size = bs.size() < _capacity ? bs.size() : _capacity;
  memcpy(_buffer, bs.data(), _size);
  return *this;
};
bool CborDeserializer::success() { return _err == 0; };
