#include <Log.h>
#include <TinyCborWrapper.h>
#include <broker_protocol.h>
#include <cbor.h>
#include <iostream>
#include <sstream>
#include <util.h>
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
  CborSerializer &member(const char *name, const int &t, const char *desc,
                         int mode = 0) {
    _err = cbor_encode_int(&_encoder, t);
    COUT << _err << endl;
    return *this;
  }
  CborSerializer &member(const char *name, int &t, const char *desc,
                         int mode = 0) {
    cbor_encode_int(&_encoder, t);
    return *this;
  }
    CborSerializer &member(const char *name, uint64_t &t, const char *desc,
                         int mode = 0) {
    cbor_encode_uint(&_encoder, t);
    return *this;
  }
      CborSerializer &member(const char *name, int64_t &t, const char *desc,
                         int mode = 0) {
    cbor_encode_int(&_encoder, t);
    return *this;
  }
  CborSerializer &member(const char *name, string &t, const char *desc) {
    cbor_encode_text_string(&_encoder, t.data(), t.size());
    return *this;
  }

  CborSerializer &member(const char *name, bytes &t, const char *desc) {
    cbor_encode_byte_string(&_encoder, t.data(), t.size());
    return *this;
  }

  template <typename T>
  CborSerializer &ro(const char *name, const T &t, const char *desc) {
    COUT << " serialize ro :" << name << endl;
    return *this;
  }
  bytes toBytes() {
    _err = cbor_encoder_close_container(&_encoderRoot, &_encoder);
    int sz = cbor_encoder_get_buffer_size(&_encoderRoot, _buffer);
    return bytes(_buffer, _buffer + sz);
  }
  CborSerializer &operator()() {
    cbor_encoder_init(&_encoderRoot, _buffer, _capacity, 0);
    _err = cbor_encoder_create_array(&_encoderRoot, &_encoder,
                                     CborIndefiniteLength);
    if (_err)
      CERR << " cbor_encoder_init failed." << endl;
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
  msgPublisher.reflect(toCbor());
  COUT << " msgPublisher " << hexDump(toCbor().toBytes()) << endl;
  msgSubscriber.reflect(toCbor());
  COUT << " msgSubscriber " << hexDump(toCbor().toBytes()) << endl;
  msgPublish.reflect(toCbor());
  COUT << " msgPublish " << hexDump(toCbor().toBytes()) << endl;
}