#include <Log.h>
#include <broker_protocol.h>
#include <cbor.h>
#include <util.h>

#include <assert>
#include <iostream>
#include <sstream>
using namespace std;
const int MsgPublish::TYPE;
const int MsgPublisher::TYPE;
const int MsgSubscriber::TYPE;

class CborSerializer {
  uint32_t _capacity;
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
    _err = cbor_encode_double(&_encoder, t.data(), t.size());
    assert(_err == 0);
    return *this;
  }

  CborSerializer &member(const char *name, bytes &t, const char *desc) {
    _err = cbor_encode_byte_string(&_encoder, t.data(), t.size());
    assert(_err == 0);
    return *this;
  }

  bytes toBytes() {
    _err = cbor_encoder_close_container(&_encoderRoot, &_encoder);
    assert(_err == 0);
    int sz = cbor_encoder_get_buffer_size(&_encoderRoot, _buffer);
    return bytes(_buffer, _buffer + sz);
  }
  CborSerializer &operator()() {
    cbor_encoder_init(&_encoderRoot, _buffer, _capacity, 0);
    _err = cbor_encoder_create_array(&_encoderRoot, &_encoder,
                                     CborIndefiniteLength);
    assert(_err == 0);
    return *this;
  };
  ~CborSerializer() { delete _buffer; }
};

Log logger(1024);

int main(int argc, char **argv) {
  cout << "Start " << argv[0] << endl;
  CborSerializer toCbor(1024);
  bytes data = {0x1, 0x2, 0x3};

  MsgPublish msgPublish = {1, data};
  MsgPublisher msgPublisher = {2, "system/uptime"};
  MsgSubscriber msgSubscriber = {3, "system/*"};
  MsgConnect msgConnect = {"src/esp32/"};
  msgConnect.reflect(toCbor());
  COUT << " msgConnect " << hexDump(toCbor().toBytes()) << endl;
  msgPublisher.reflect(toCbor());
  COUT << " msgPublisher " << hexDump(toCbor().toBytes()) << endl;
  msgSubscriber.reflect(toCbor());
  COUT << " msgSubscriber " << hexDump(toCbor().toBytes()) << endl;
  msgPublish.reflect(toCbor());
  COUT << " msgPublish " << hexDump(toCbor().toBytes()) << endl;
}