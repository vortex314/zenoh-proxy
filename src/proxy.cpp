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
#include <CborDump.h>
#include <CborSerializer.h>
#include <Display.h>
#include <Frame.h>
#include <broker_protocol.h>
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
StaticJsonDocument<10240> doc;

Config loadConfig(int argc, char **argv) {
  Config cfg = doc.to<JsonObject>();
  if (argc > 1) cfg["serial"]["port"] = argv[1];
  if (argc > 2) cfg["serial"]["baudrate"] = atoi(argv[2]);
  string sCfg;
  serializeJson(doc, sCfg);
  INFO(" config : %s ", sCfg.c_str());
  return cfg;
};

//================================================================
class MsgFilter : public LambdaFlow<bytes, bytes> {
  int _msgType;
  MsgBase msgBase;
  CborDeserializer _fromCbor;

 public:
  MsgFilter(int msgType)
      : LambdaFlow<bytes, bytes>([this](bytes &out, const bytes &in) {
          //         INFO(" filter on msgType : %d in %s ",
          //         _msgType,cborDump(in).c_str());
          if (msgBase.reflect(_fromCbor.fromBytes(in)).success() &&
              msgBase.msgType == _msgType) {
            //            INFO(" found msgType : %d  ", msgBase.msgType);
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
  FrameToBytes frameToBytes;
  ValueFlow<bytes> toSerialMsg;
  ValueFlow<bytes> fromSerialMsg;

  serial.init();
  serial.connect();
  // zSession.scout();
  broker.init();
  // CBOR de-/serialization
  serial.incoming >> bytesToFrame >> fromSerialMsg;
  toSerialMsg >> frameToBytes >> serial.outgoing;

  toSerialMsg >> [&](const bytes &bs) { INFO("TXD %s", cborDump(bs).c_str()); };
  fromSerialMsg >>
      [&](const bytes &bs) { INFO("RXD %s", cborDump(bs).c_str()); };

  // filter commands from uC
  fromSerialMsg  >> MsgFilter::nw(B_CONNECT) >> [&](const bytes &frame) {
    MsgConnect msgConnect;
    if (msgConnect.reflect(fromCbor.fromBytes(frame)).success()) {
      int rc = broker.connect(msgConnect.clientId);
      MsgConnect msgConnectReply = {"connected"};
      toSerialMsg.on(msgConnectReply.reflect(toCbor).toBytes());
    }
  };

  fromSerialMsg >> MsgFilter::nw(B_SUBSCRIBER) >> [&](const bytes &frame) {
    MsgSubscriber msgSubscriber;
    if (msgSubscriber.reflect(fromCbor.fromBytes(frame)).success()) {
      int rc = broker.subscriber(
          msgSubscriber.id, msgSubscriber.topic, [&](int id, const bytes &bs) {
            MsgPublish msgPublish = {id, bs};
            toSerialMsg.on(msgPublish.reflect(toCbor).toBytes());
          });
      if (rc)
        WARN(" subscriber (%s,..) = %d ", msgSubscriber.topic.c_str(), rc);
    }
  };

  fromSerialMsg >> MsgFilter::nw(B_PUBLISHER) >> [&](const bytes &frame) {
    MsgPublisher msgPublisher;
    if (msgPublisher.reflect(fromCbor.fromBytes(frame)).success()) {
      int rc = broker.publisher(msgPublisher.id, msgPublisher.topic);
      if (rc) WARN("  publish (%s,..) = %d ", msgPublisher.topic.c_str(), rc);
    };
  };

  fromSerialMsg >> MsgFilter::nw(B_PUBLISH) >> [&](const bytes &frame) {
    MsgPublish msgPublish;
    if (msgPublish.reflect(fromCbor.fromBytes(frame)).success()) {
      broker.publish(msgPublish.id, msgPublish.value);
    }
  };

  fromSerialMsg >> MsgFilter::nw(B_DISCONNECT) >> [&](const bytes &frame) {
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
      frameToBytes.on(cbor::array{Z_RESOURCE, resource, key});
    };*/
  /*
    frameToCbor >> CborFilter::nw(Z_QUERY) >> [&](const cbor &param) {
      INFO("Z_RESOURCE");
      string uri = param.to_array()[1];
      auto result = zSession.query(uri);
      for (auto res : result) {
        frameToBytes.on(cbor::array{Z_QUERY, res.key, res.data});
      }
    };*/

  serial.connected >> [&](const bool isConnected) {
    if (!isConnected) broker.disconnect();
  };

  workerThread.run();
}
