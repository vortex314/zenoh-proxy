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
#include <serial_session.h>
//#include <zenoh_session.h>

//====================================================

const char *CMD_TO_STRING[] = {"B_CONNECT",   "B_DISCONNECT", "B_SUBSCRIBER",
                               "B_PUBLISHER", "B_PUBLISH",    "B_RESOURCE",
                               "B_QUERY"};

Config loadConfig(int argc, char **argv) { return Config(); };
//=================================================================
class BytesToCbor : public LambdaFlow<bytes, cbor> {
public:
  BytesToCbor()
      : LambdaFlow<bytes, cbor>([](cbor &msg, const bytes &data) {
          msg = cbor::decode(data);
          int msgType = msg.to_array()[0];
          INFO(" PROXY RXD %s :  %s", CMD_TO_STRING[msgType],
               cbor::debug(msg).c_str());
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
      toStdout(_inputFrame);
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
          int msgType = in.to_array()[0];
          INFO(" PROXY TXD %s :  %s", CMD_TO_STRING[msgType],
               cbor::debug(in).c_str());
          out = ppp_frame(cbor::encode(in));
          return true;
        }){};
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

  // filter commands from uC
  frameToCbor >> CborFilter::nw(B_CONNECT) >> [&](const cbor &param) {
    int rc = broker.connect();
    toFrame.on(cbor::array{B_CONNECT, rc});
  };

  frameToCbor >> CborFilter::nw(B_SUBSCRIBER) >> [&](const cbor &param) {
    int id = param.to_array()[2];
    string key = param.to_array()[1];
    int rc = broker.subscriber(id, key, [&](const bytes &bs) {
      toFrame.on(cbor::array{B_PUBLISH, id, bs});
    });
    if (rc)
      WARN(" subscriber (%s,..) = %d ", key.c_str(), rc);
  };

  frameToCbor >> CborFilter::nw(B_PUBLISHER) >> [&](const cbor &param) {
    int id = param.to_array()[2];
    string key = param.to_array()[1];
    int rc = broker.publisher(id, key);
    if (rc)
      WARN("  publish (%s,..) = %d ", key.c_str(), rc);
  };

  frameToCbor >> CborFilter::nw(B_PUBLISH) >> [&](const cbor &param) {
    int id = param.to_array()[1];
    bytes data = param.to_array()[2];
    int rc = broker.publish(id, data);
  };

  frameToCbor >> CborFilter::nw(B_DISCONNECT) >>
      [&](const cbor &param) { broker.disconnect(); };
  /*
    frameToCbor >> CborFilter::nw(B_RESOURCE) >> [&](const cbor &param) {
      INFO("Z_RESOURCE");
      string resource = param.to_array()[1];
      zenoh::ResourceKey key = zSession.resource(resource);
      toFrame.on(cbor::array{Z_RESOURCE, resource, key});
    };*/
  /*
    frameToCbor >> CborFilter::nw(Z_QUERY) >> [&](const cbor &param) {
      INFO("Z_RESOURCE");
      string uri = param.to_array()[1];
      auto result = zSession.query(uri);
      for (auto res : result) {
        toFrame.on(cbor::array{Z_QUERY, res.key, res.data});
      }
    };*/

  serial.connected >> [&](const bool isConnected) {
    if (!isConnected)
      broker.disconnect();
  };

  workerThread.run();
}
