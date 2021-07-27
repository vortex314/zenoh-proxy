#include <CborDeserializer.h>
#include <CborDump.h>
#include <CborSerializer.h>
#include <Display.h>
#include <Log.h>
#include <assert.h>
#include <broker_protocol.h>
#include <util.h>

#include <iostream>
using namespace std;
const int MsgPublish::TYPE;
const int MsgPublisher::TYPE;
const int MsgSubscriber::TYPE;
const int MsgConnect::TYPE;
const int MsgDisconnect::TYPE;

Log logger(1024);

#include <gtest/gtest.h>

TEST(MsgPublisher, BasicAssertions) {
  MsgPublisher msgPublisher = {2, "system/uptime"};
  CborSerializer toCbor(1024);
  msgPublisher.reflect(toCbor);
  COUT << " msgPublisher " << hexDump(toCbor.toBytes()) << endl;
  EXPECT_EQ(toCbor.toBytes().size(), 18);
  byte data[] = {0x9F, 0x03, 0x02, 0x6D, 0x73, 0x79, 0x73, 0x74, 0x65,
                 0x6D, 0x2F, 0x75, 0x70, 0x74, 0x69, 0x6D, 0x65, 0xFF};
  EXPECT_EQ(toCbor.toBytes(), bytes(data, data + sizeof(data)));
}

TEST(MsgSubscriber, BasicAssertions) {
  MsgSubscriber msgSubscriber = {3, "system/*"};
  CborSerializer toCbor(100);
  bytes bs = msgSubscriber.reflect(toCbor).toBytes();
  INFO("%s", cborDump(bs).c_str());
  CborDeserializer fromCbor(100);
  MsgSubscriber msgSubscriber2;
  msgSubscriber2.reflect(fromCbor.fromBytes(bs));
  EXPECT_EQ(msgSubscriber.TYPE, msgSubscriber2.TYPE);
  EXPECT_EQ(msgSubscriber.id, msgSubscriber2.id);
  EXPECT_EQ(msgSubscriber.topic, msgSubscriber2.topic);
}

TEST(MsgPublish, BasicAssertions) {
  string s = "i/am/alive";
  bytes data = bytes(s.c_str(), s.c_str() + s.size());
  MsgPublish msgPublish = {3, data};
  CborSerializer toCbor(100);
  bytes bs = msgPublish.reflect(toCbor).toBytes();
  INFO("%s", cborDump(bs).c_str());
  CborDeserializer fromCbor(100);
  MsgPublish msgPublish2;
  msgPublish2.reflect(fromCbor.fromBytes(bs));
  EXPECT_EQ(msgPublish.TYPE, msgPublish2.TYPE);
  EXPECT_EQ(msgPublish.id, msgPublish2.id);
  EXPECT_EQ(msgPublish.value, msgPublish2.value);
}

int func(int argc, char **argv) {
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
  return 0;
}