#include <ArduinoJson.h>
#include <Log.h>
#include <config.h>
#include <stdio.h>

#include <thread>
#include <unordered_map>
#include <utility>

using namespace std;

Log logger(2048);
#include <broker_protocol.h>
#include <broker_zenoh.h>
#include <cbor11.h>
#include <ppp_frame.h>
#include <serial_protocol.h>
#include <serial_session.h>
#include <zenoh_session.h>

//====================================================

const char *CMD_TO_STRING[] = {"B_CONNECT", "B_DISCONNECT", "Z_SUBSCRIBE",
                               "Z_PUBLISH", "Z_QUERY",      "Z_QUERYABLE",
                               "Z_RESOURCE"};

Config loadConfig(int argc, char **argv) { return Config(); };
//=================================================================
class BytesToCbor : public LambdaFlow<bytes, cbor> {
 public:
  BytesToCbor()
      : LambdaFlow<bytes, cbor>([](cbor &msg, const bytes &data) {
          msg = cbor::decode(data);
          cbor cb = cbor::decode(msg.to_array()[2]);
          INFO(" msg %s => %s", cbor::debug(msg).c_str(),
               cbor::debug(cb).c_str());
          return msg.is_array();
        }){};
};
//================================================================
class CborFilter : public LambdaFlow<cbor, cbor> {
  int _msgType;

 public:
  CborFilter(int msgType)
      : LambdaFlow<cbor, cbor>([this](cbor &out, const cbor &in) {
          out = in;
          return in.to_array()[0] == cbor(_msgType);
        }) {
    _msgType = msgType;
  };
  static CborFilter &nw(int msgType) { return *new CborFilter(msgType); }
};
//================================================================
class FrameExtractor : public Flow<bytes, bytes> {
  bytes _inputFrame;
  bytes _cleanData;
  uint64_t _lastFrameFlag;
  uint32_t _frameTimeout = 1000;

 public:
  FrameExtractor() : Flow<bytes, bytes>() {}
  void on(const bytes &bs) { handleRxd(bs); }
  void toStdout(const bytes &bs) {
    if (bs.size()) {
      //  cout << hexDump(bs) << endl;
      fwrite("\033[32m", 5, 1, stdout);
      fwrite(bs.data(), bs.size(), 1, stdout);
      fwrite("\033[39m", 5, 1, stdout);
    }
  }

  bool handleFrame(bytes &bs) {
    if (bs.size() == 0) return false;
    if (ppp_deframe(_cleanData, bs)) {
      emit(_cleanData);
      return true;
    } else {
      toStdout(bs);
      return false;
    }
  }

  void handleRxd(const bytes &bs) {
    for (uint8_t b : bs) {
      if (b == PPP_FLAG_CHAR) {
        _lastFrameFlag = Sys::millis();
        handleFrame(_inputFrame);
        _inputFrame.clear();
      } else {
        _inputFrame.push_back(b);
      }
    }
    if ((Sys::millis() - _lastFrameFlag) > _frameTimeout) {
      //   cout << " skip  bytes " << hexDump(bs) << endl;
      //   cout << " frame data drop " << hexDump(frameData) << flush;
      toStdout(bs);
      _inputFrame.clear();
    }
  }
  void request(){};
};
//=========================================================================
class FrameGenerator : public LambdaFlow<cbor, bytes> {
 public:
  FrameGenerator()
      : LambdaFlow<cbor, bytes>([&](bytes &out, const cbor &in) {
          out = ppp_frame(cbor::encode(in));
          return true;
        }){};
};
//=========================================================================

class SerialMock : public Actor, public Invoker {
  vector<cbor> testScenario = {
      cbor::array{B_CONNECT},  //
      cbor::array{B_SUBSCRIBER, "/mock/alive", 0},
      cbor::array{B_SUBSCRIBER, "/@/router/**", 1},
      cbor::array{B_PUBLISHER, "src/esp32/system/uptime", 1},
      cbor::array{B_PUBLISH, "/mock/alive", cbor::encode(Sys::millis())},
      cbor::array{B_RESOURCE, "/demo/aaa", 0},
      cbor::array{B_RESOURCE, "demo/aaa", 0},
      cbor::array{B_RESOURCE, "/demo/aaa", 0},
      cbor::array{B_DISCONNECT}  //
  };
  int counter = 0;
  Serial _serial;
  bytes _rxdBuffer;

 public:
  ValueFlow<bytes> outgoing;
  ValueSource<bytes> incoming;
  ValueSource<bool> connected;
  BytesToCbor frameToCbor;
  TimerSource ticker;
  FrameExtractor bytesToFrame;
  FrameGenerator toFrame;
  SerialMock(Thread &thr, Config &cfg)
      : Actor(thr), ticker(thr, 100, true, "ticker") {
    _serial.port("/dev/ttyUSB1");
    _serial.baudrate(115200);
    _serial.init();
    _serial.connect();
    thread().addReadInvoker(_serial.fd(), this);

    incoming >> bytesToFrame >> frameToCbor;
    frameToCbor >> [&](const cbor &cb) {
      INFO(" client received %s ", cbor::debug(cb).c_str());
    };

    outgoing >> [&](const bytes &frame) {
      INFO(" MOCK TXD %s ", hexDump(frame).c_str());
      _serial.txd(frame);
    };
    ticker >> [&](const TimerMsg &tm) {
      if (counter < testScenario.size()) {
        outgoing = ppp_frame(cbor::encode(testScenario[counter]));
        counter++;
      } else {
        counter = 0;
      }
    };
    thread().enqueue(this);
  }
  void invoke() {
    INFO(" reading data");
    int rc = _serial.rxd(_rxdBuffer);
    if (rc == 0) {                   // read ok
      if (_rxdBuffer.size() == 0) {  // but no data
        WARN(" 0 data ");
      } else {
        incoming = _rxdBuffer;
      }
    }
  }
  void init(){};
  void connect(){};
  void disconnect(){};
};
//==========================================================================
int main(int argc, char **argv) {
  Config config = loadConfig(argc, argv);
  Thread workerThread("worker");

  Config serialConfig = config["serial"];
  SerialSession serial(workerThread, serialConfig);

  Config brokerConfig = config["zenoh"];
  BrokerZenoh broker(workerThread, brokerConfig);

  //  Config mockConfig = config["mock"];
  //  SerialMock mock(workerThread, mockConfig);

  BytesToCbor frameToCbor;
  FrameExtractor bytesToFrame;
  FrameGenerator toFrame;

  serial.init();
  serial.connect();
  // zSession.scout();
  broker.init();
  // CBOR de-/serialization
  toFrame >> serial.outgoing;
  serial.incoming >> bytesToFrame >> frameToCbor;
  // ZENOH feedbacks
  zSession.incoming >> [&](const zenoh::Message &msg) {
    toFrame.on(cbor::array{Z_PUBLISH, msg.key, msg.data});
  };
  // filter commands from uC
  frameToCbor >> CborFilter::nw(B_CONNECT) >> [&](const cbor &param) {
    INFO("B_CONNECT");
    int rc = zSession.open();
    toFrame.on(cbor::array{B_CONNECT, rc});
  };

  frameToCbor >> CborFilter::nw(B_SUBSCRIBER) >> [&](const cbor &param) {
    INFO("B_SUBSCRIBER");
    string resource = param.to_array()[1];
    int rc = zSession.subscribe(resource);
    if (rc) WARN(" zenoh subscribe (%s,..) = %d ", resource.c_str(), rc);
  };

  frameToCbor >> CborFilter::nw(B_PUBLISHER) >> [&](const cbor &param) {
    INFO("B_PUBLISHER");
    string resource = param.to_array()[1];
    bytes data = param.to_array()[2];
    int rc = zSession.publish(resource, data);
    broker.publisher(resource) if (rc)
        WARN(" zenoh publish (%s,..) = %d ", resource.c_str(), rc);
  };

  frameToCbor >> CborFilter::nw(B_DISCONNECT) >> [&](const cbor &param) {
    INFO("B_DISCONNECT");
    zSession.close();
  };

  frameToCbor >> CborFilter::nw(B_RESOURCE) >> [&](const cbor &param) {
    INFO("Z_RESOURCE");
    string resource = param.to_array()[1];
    zenoh::ResourceKey key = zSession.resource(resource);
    toFrame.on(cbor::array{Z_RESOURCE, resource, key});
  };

  frameToCbor >> CborFilter::nw(Z_QUERY) >> [&](const cbor &param) {
    INFO("Z_RESOURCE");
    string uri = param.to_array()[1];
    auto result = zSession.query(uri);
    for (auto res : result) {
      toFrame.on(cbor::array{Z_QUERY, res.key, res.data});
    }
  };

  serial.connected >> [&](const bool isConnected) {
    if (!isConnected) zSession.close();
  };

  workerThread.run();
}
