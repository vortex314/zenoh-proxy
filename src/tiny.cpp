#include <Log.h>
#include <broker_protocol.h>
#include <reflect_to_cbor.h>
#include <reflect_to_display.h>
#include <util.h>

#include <assert>
#include <iostream>
using namespace std;
const int MsgPublish::TYPE;
const int MsgPublisher::TYPE;
const int MsgSubscriber::TYPE;
const int MsgConnect::TYPE;
const int MsgDisconnect::TYPE;

Log logger(1024);

int main(int argc, char **argv) {
  cout << "Start " << argv[0] << endl;
  CborSerializer toCbor(1024);
  Display toDisplay;
  bytes data = {0x1, 0x2, 0x3};

  MsgPublish msgPublish = {1, data};
  MsgPublisher msgPublisher = {2, "system/uptime"};
  MsgSubscriber msgSubscriber = {3, "system/*"};
  MsgConnect msgConnect = {"src/esp32/"};

  msgConnect.reflect(toCbor);
  COUT << " msgConnect " << hexDump(toCbor.toBytes()) << endl;

  msgPublisher.reflect(toCbor);
  COUT << " msgPublisher " << hexDump(toCbor.toBytes()) << endl;

  msgSubscriber.reflect(toCbor);
  COUT << " msgSubscriber " << hexDump(toCbor.toBytes()) << endl;

  COUT << " msgPublish " << hexDump(msgPublish.reflect(toCbor).toBytes())
       << endl;
  COUT << " msgPublish " << msgPublish.reflect(toDisplay).toString() << endl;
}