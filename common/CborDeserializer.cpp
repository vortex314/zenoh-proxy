#include <CborDeserializer.h>
CborDeserializer::CborDeserializer(size_t size)
{
  _buffer = new uint8_t[size];
  _capacity = size;
}
CborDeserializer &CborDeserializer::begin()
{
  _err = CborNoError;
  _err = cbor_parser_init(_buffer, _size, 0, &_decoder, &_it);
  return *this;
};

CborDeserializer &CborDeserializer::end() { return *this; };

CborDeserializer &CborDeserializer::member(bytes &t, const char *name,
                                           const char *desc)
{
  size_t size;
  if (!_err && cbor_value_is_byte_string(&_it) &&
      cbor_value_calculate_string_length(&_it, &size) == 0)
  {
    t.reserve(size);
    _err = cbor_value_copy_byte_string(&_it, t.data(), &size, 0);
    t.resize(size);
  }
  return *this;
}

CborDeserializer &CborDeserializer::member(string &t, const char *name,
                                           const char *desc)
{
  size_t size;
  if (!_err && cbor_value_is_text_string(&_it) &&
      cbor_value_calculate_string_length(&_it, &size) == 0)
  {
    char temp[size + 1];
    _err = cbor_value_copy_text_string(&_it, temp, &size, 0);
    t = temp;
  }
  return *this;
}

CborDeserializer &CborDeserializer::member(int &t, const char *name,
                                           const char *desc)
{
  if (!_err && cbor_value_is_integer(&_it))
  {
    _err = cbor_value_get_int(&_it, &t);
  }
  return *this;
}

CborDeserializer &CborDeserializer::member(const int &t, const char *name,
                                           const char *desc)
{
  int x;
  if (!_err && cbor_value_is_integer(&_it))
  {
    _err = cbor_value_get_int(&_it, &x);
  }
  return *this;
}

CborDeserializer &CborDeserializer::fromBytes(const bytes &bs)
{
  _size = bs.size() < _capacity ? bs.size() : _capacity;
  memcpy(_buffer, bs.data(), _size);
  return *this;
};
bool CborDeserializer::success() { return _err == 0; };
