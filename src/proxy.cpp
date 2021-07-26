#include <ArduinoJson.h>
#include <Log.h>
#include <config.h>
#include <stdio.h>

#include <thread>
#include <unordered_map>
#include <utility>

using namespace std;

Log logger(2048);
#include <BrokerZenoh.h>
#include <CborDeserializer.h>
#include <CborSerializer.h>
#include <CborDump.h>
#include <Display.h>
#include <broker_protocol.h>
#include <Frame.h>
#include <serial_session.h>
const int MsgPublish::TYPE;
const int MsgPublisher::TYPE;
const int MsgSubscriber::TYPE;
const int MsgConnect::TYPE;
const int MsgDisconnect::TYPE;
//====================================================

const char *CMD_TO_STRING[] = {"B_CONNECT",   "B_DISCONNECT", "B_SUBSCRIBER",
                               "B_PUBLISHER", "B_PUBLISH",    "B_RESOURCE",
                               "B_QUERY"};

Config loadConfig(int argc, char **argv) { return Config(); };

//================================================================
class MsgFilter : public LambdaFlow<bytes, bytes> {
  int _msgType;
  MsgBase msgBase;
  CborDeserializer _fromCbor;

 public:
  MsgFilter(int msgType)
      : LambdaFlow<bytes, bytes>([this](bytes &out, const bytes &in) {
          if (msgBase.reflect(_fromCbor.fromBytes(in)).success() &&
              msgBase.msgType == _msgType) {
            out = in;
            return true;
          }
          return false;
        }),
        _fromCbor(1024) {
    _msgType = msgType;
  };
  static MsgFilter &nw(int msgType) { return *new MsgFilter(msgType); }
};

//==========================================================================
int main(int argc, char **argv) {
  Config config = loadConfig(argc, argv);
  Thread workerThread("worker");

  Config serialConfig = config["serial"];
  SerialSession serial(workerThread, serialConfig);

  Config brokerConfig = config["zenoh"];
  BrokerZenoh broker(workerThread, brokerConfig);
  FrameExtractor bytesToFrame;
  CborDeserializer fromCbor(1024);
  CborSerializer toCbor(1024);
  FrameGenerator toFrame;

  serial.init();
  serial.connect();
  // zSession.scout();
  broker.init();
  // CBOR de-/serialization
  serial.incoming >> bytesToFrame;
  toFrame >> serial.outgoing;

  bytesToFrame >> [&](const bytes& bs){ COUT << "RXD " << cborDump(bs) << endl ;};
  

  // filter commands from uC
  bytesToFrame >> MsgFilter::nw(B_CONNECT) >> [&](const bytes &frame) {
    MsgConnect msgConnect;
    if (msgConnect.reflect(fromCbor.fromBytes(frame)).success()) {
      int rc = broker.connect(msgConnect.clientId);
      MsgConnect msgConnectReply = {"connected"};
      toFrame.on(msgConnectReply.reflect(toCbor).toBytes());
    }
  };

  bytesToFrame >> MsgFilter::nw(B_SUBSCRIBER) >> [&](const bytes &frame) {
    MsgSubscriber msgSubscriber;
    if (msgSubscriber.reflect(fromCbor.fromBytes(frame)).success()) {
      int rc = broker.subscriber(
          msgSubscriber.id, msgSubscriber.topic, [&](int id, const bytes &bs) {
            MsgPublish msgPublish = {id, bs};
            toFrame.on(msgPublish.reflect(toCbor).toBytes());
          });
      if (rc)
        WARN(" subscriber (%s,..) = %d ", msgSubscriber.topic.c_str(), rc);
    }
  };

  bytesToFrame >> MsgFilter::nw(B_PUBLISHER) >> [&](const bytes &frame) {
    MsgPublisher msgPublisher;
    if (msgPublisher.reflect(fromCbor.fromBytes(frame)).success()) {
      int rc = broker.publisher(msgPublisher.id, msgPublisher.topic);
      if (rc) WARN("  publish (%s,..) = %d ", msgPublisher.topic.c_str(), rc);
    };
  };

  bytesToFrame >> MsgFilter::nw(B_PUBLISH) >> [&](const bytes &frame) {
    MsgPublish msgPublish;
    if (msgPublish.reflect(fromCbor.fromBytes(frame)).success()) {
      broker.publish(msgPublish.id, msgPublish.value);
    }
  };

  bytesToFrame >> MsgFilter::nw(B_PUBLISH) >> [&](const bytes &frame) {
    MsgDisconnect msgDisconnect;
    if (msgDisconnect.reflect(fromCbor.fromBytes(frame)).success()) {
      broker.disconnect();
    }
  };
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
    if (!isConnected) broker.disconnect();
  };

  workerThread.run();
}
