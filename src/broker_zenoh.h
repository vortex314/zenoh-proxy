#ifndef _ZENOH_SESSION_H_
#define _ZENOH_SESSION_H_
#include "broker.h"
#include "limero.h"
#include "util.h"
extern "C" {
#include "zenoh/net.h"
}
#define US_WEST "tcp/us-west.zenoh.io:7447"
#define US_EAST "tcp/us-east.zenoh.io:7447"

using namespace std;

typedef struct Message {
  string key;
  bytes data;
} Message;

typedef unordered_map<unsigned long, string> Properties;
typedef unsigned long ResourceKey;

class PublisherZenoh : public Publisher {};

class SubscriberZenoh : public Subscriber {};

class BrokerZenoh : public Broker {
  zn_session_t *_zenoh_session;
  unordered_map<string, zn_subscriber_t *> _subscribers;
  static void dataHandler(const zn_sample_t *, const void *);
  ResourceKey resource(string topic);
  vector<Message> query(string);
  int scout();
  Properties &info();

 public:
  ValueSource<bool> connected;
  ValueSource<Message> incoming;

  BrokerZenoh(Thread &, Config &);
  int init();
  int connect();
  int disconnect();
  Subscriber *subscriber(string);
  Publisher *publisher(string topic);
  int unSubscribe(Subscriber *);
};
// namespace zenoh
#endif  // _ZENOH_SESSION_h_