#define JSON_NOEXCEPTION
#include "json.hpp"
#include "util.h"
#include <Log.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;
using namespace nlohmann;


#include <broker_protocol.h>
#include <reflectors.h>
const int MsgPublish::TYPE;
const int MsgPublisher::TYPE;
const int MsgSubscriber::TYPE;


Log logger(1024);

int main(int argc, char **argv) {
  cout << "Start " << argv[0] << endl;
  CborSerializer toCbor;
  CborDeserializer fromCbor;
  Display toDisplay;
  bytes data = {0x1, 0x2, 0x3};
  data = json::to_cbor({"hello world", 1.2, 10, true});
  MsgPublish msgPublish = {1, data};
  MsgPublisher msgPublisher = {2, "system/uptime"};
  MsgSubscriber msgSubscriber = {3, "system/*"};
  COUT << "msgPublish :" << msgPublish.reflect(toDisplay()).toString() << endl;

  COUT << "msgPublisher :" << msgPublisher.reflect(toDisplay()).toString()
       << endl;

  COUT << "msgSubscriber :" << msgSubscriber.reflect(toDisplay()).toString()
       << endl;

  MsgSubscriber msgS;
  msgS.reflect(fromCbor.fromBytes(msgSubscriber.reflect(toCbor()).toBytes()));

  MsgBase msgBase;
  bytes result = msgPublish.reflect(toCbor()).toBytes();
  json js = json::from_cbor(result);
  COUT << hexDump(result) << endl;
  COUT << js << endl;
  COUT << "result size " << result.size() << endl;
  if (msgBase.reflect(fromCbor.fromBytes(result)).success()) {
    COUT << "msgBase:{" << msgBase.reflect(toDisplay()).toString() << "}"
         << endl;
    if (msgBase.msgType == MsgPublish::TYPE) {
      COUT << " parsing as MsgPublish " << endl;
      if (msgPublish.reflect(fromCbor.fromBytes(result)).success())
        COUT << "msgPublish : {" << msgPublish.reflect(toDisplay()).toString()
             << "}" << endl;
      COUT << json::from_cbor(msgPublish.value) << endl;
    }
  }
}