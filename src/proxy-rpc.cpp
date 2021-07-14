#include <ArduinoJson.h>
#include <Log.h>
#include <stdio.h>

#include <config.h>
#include <thread>
#include <unordered_map>
#include <utility>

using namespace std;

Log logger(2048);
#include <cbor11.h>
#include <ppp_frame.h>
#include <serial_protocol.h>
#include <serial_session.h>
#include <zenoh_session.h>

//====================================================

const char *CMD_TO_STRING[] = {"Z_OPEN",    "Z_CLOSE", "Z_SUBSCRIBE",
                               "Z_PUBLISH", "Z_QUERY", "Z_QUERYABLE",
                               "Z_RESOURCE"};

Config loadConfig(int argc, char **argv) { return Config(); };

class BytesToCbor : public LambdaFlow<bytes, cbor> {
public:
  BytesToCbor()
      : LambdaFlow<bytes, cbor>([](cbor &msg, const bytes &data) {
          msg = cbor::decode(data);
          INFO(" msg %s", cbor::debug(msg).c_str());
          return msg.is_array();
        }){};
} serialCbor;

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
    if (bs.size() == 0)
      return false;
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
} getFrame;

class FrameGenerator : public LambdaFlow<cbor, bytes> {
public:
  FrameGenerator()
      : LambdaFlow<cbor, bytes>([&](bytes &out, const cbor &in) {
          out = ppp_frame(cbor::encode(in));
          return true;
        }){};
} toFrame;

class SerialMock : public Actor {
  vector<cbor> sequence = {
      cbor::array{Z_OPEN}, //
      cbor::array{Z_SUBSCRIBE, "/demo/aaa"},
      cbor::array{Z_SUBSCRIBE, "/@/**"},
      cbor::array{Z_PUBLISH, "/demo/aaa", cbor::binary{0x13, 0x14}},
      cbor::array{Z_RESOURCE, "/demo/aaa", 0},
      cbor::array{Z_RESOURCE, "demo/aaa", 0},
      cbor::array{Z_RESOURCE, "/demo/aaa", 0},
      cbor::array{Z_CLOSE}
  };
  int counter = 0;

public:
  ValueFlow<bytes> incoming;
  Sink<bytes> outgoing;
  ValueSource<bool> connected;
  TimerSource ticker;
  SerialMock(Thread &thread, Config &cfg)
      : Actor(thread),
        outgoing(
            10, [&](const bytes &bs) { INFO("TXD : %s", hexDump(bs).c_str()); },
            "serial.outgoing"),
        ticker(thread, 100, true, "ticker") {
    ticker >> [&](const TimerMsg &tm) {
      if (counter < sequence.size()) {
        incoming = ppp_frame(cbor::encode(sequence[counter]));
        counter++;
      }
    };
  }
  void init(){};
  void connect(){};
  void disconnect(){};
};

int main(int argc, char **argv) {

  Config config = loadConfig(argc, argv);
  Thread workerThread("worker");

  Config serialConfig = config["serial"];
  SerialMock serial(workerThread, serialConfig);

  Config zenohConfig = config["zenoh"];
  zenoh::Session zSession(workerThread, zenohConfig);
  zenoh::Properties properties;

  serial.init();
  serial.connect();
  // zSession.scout();

  toFrame >> serial.outgoing;

  serial.incoming >> getFrame >> serialCbor;

  serialCbor >> CborFilter::nw(Z_OPEN) >> [&](const cbor &param) {
    INFO("Z_OPEN");
    int rc = zSession.open(properties);
    toFrame.on(cbor::array{Z_OPEN, rc});
  };

  serialCbor >> CborFilter::nw(Z_SUBSCRIBE) >> [&](const cbor &param) {
    INFO("Z_SUBSCRIBE");
    string resource = param.to_array()[1];
    zSession.subscribe(resource);
  };

  serialCbor >> CborFilter::nw(Z_PUBLISH) >> [&](const cbor &param) {
    INFO("Z_PUBLISH");
    string resource = param.to_array()[1];
    bytes data = param.to_array()[2];
    zSession.publish(resource, data);
  };

  serialCbor >> CborFilter::nw(Z_CLOSE) >> [&](const cbor &param) {
    INFO("Z_CLOSE");
    zSession.close();
  };

  serialCbor >> CborFilter::nw(Z_RESOURCE) >> [&](const cbor &param) {
    INFO("Z_RESOURCE");
    string resource = param.to_array()[1];
    zenoh::ResourceKey key = zSession.resource(resource);
    toFrame.on(cbor::array{Z_RESOURCE, resource, key});
  };

  serial.connected >> [&](const bool isConnected) {
    if (!isConnected)
      zSession.close();
  };

  workerThread.run();
}
