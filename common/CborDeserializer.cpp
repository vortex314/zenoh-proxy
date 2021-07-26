#include <CborDeserializer.h>
#include <Log.h>
#undef NDEBUG
CborDeserializer::CborDeserializer(size_t size) {
  _buffer = new uint8_t[size];
  _capacity = size;
}
CborDeserializer &CborDeserializer::begin() {
  _err = CborNoError;
   
  _err = cbor_parser_init(_buffer, _size, 0, &_decoder, &_rootIt);
  assert(_err == CborNoError);
  _err = cbor_value_enter_container(&_rootIt,&_it);

  return *this;
};

CborDeserializer &CborDeserializer::end() { return *this; };

CborDeserializer &CborDeserializer::member(bytes &t, const char *name,
                                           const char *desc) {
  size_t size;
  if (!_err && cbor_value_is_byte_string(&_it) &&
      cbor_value_calculate_string_length(&_it, &size) == 0) {
    t.reserve(size);
    _err = cbor_value_copy_byte_string(&_it, t.data(), &size, 0);
    assert(_err == CborNoError);
    t.resize(size);
  }
  return *this;
}

CborDeserializer &CborDeserializer::member(string &t, const char *name,
                                           const char *desc) {
  size_t size;
  if (!_err && cbor_value_is_text_string(&_it) &&
      cbor_value_calculate_string_length(&_it, &size) == 0) {
    char temp[size + 1];
    _err = cbor_value_copy_text_string(&_it, temp, &size, 0);
    assert(_err == CborNoError);
    t = temp;
  }
  return *this;
}

CborDeserializer &CborDeserializer::member(int &t, const char *name,
                                           const char *desc) {
//  INFO(" looking for %s , _err : %d ,isInteger : %s size:%d, capacity :%d",name,_err,cbor_value_is_integer(&_it)?"true":"false" ,_size,_capacity);                                           
  if (!_err && cbor_value_is_integer(&_it)) {
    _err = cbor_value_get_int(&_it, &t);
//    INFO(" found %d ",t);
    assert(_err == CborNoError);
  }
  return *this;
}

CborDeserializer &CborDeserializer::member(const int &t, const char *name,
                                           const char *desc) {
  int x;
  if (!_err && cbor_value_is_integer(&_it)) {
    _err = cbor_value_get_int(&_it, &x);
    assert(_err == CborNoError);
  }
  return *this;
}

CborDeserializer &CborDeserializer::fromBytes(const bytes &bs) {
  assert(bs.size() < _capacity);
  _size = bs.size() < _capacity ? bs.size() : _capacity;
  memcpy(_buffer, bs.data(), _size);
  return *this;
};
bool CborDeserializer::success() { return _err == 0; };
