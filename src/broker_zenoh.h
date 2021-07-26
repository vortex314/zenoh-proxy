#ifndef _ZENOH_SESSION_H_
#define _ZENOH_SESSION_H_
#include "limero.h"
#include "util.h"
extern "C" {
#include "zenoh.h"
#include "zenoh/net.h"
}
#define US_WEST "tcp/us-west.zenoh.io:7447"
#define US_EAST "tcp/us-east.zenoh.io:7447"

using namespace std;

typedef int (*SubscribeCallback)(int, bytes);
struct PubMsg {
  int id;
  bytes value;
};

struct Sub {
  int id;
  string key;
  std::function<void(int,const bytes &)> callback;
  zn_subscriber_t *zn_subscriber;
};

struct Pub {
  int id;
  string key;
  zn_reskey_t zn_reskey;
  zn_publisher_t *zn_publisher;
};

class BrokerZenoh : public Actor {
  zn_session_t *_zenoh_session;
  unordered_map<int, Sub *> _subscribers;
  unordered_map<int, Pub *> _publishers;
  static void subscribeHandler(const zn_sample_t *, const void *);
  zn_reskey_t resource(string topic);
  int scout();

public:
  ValueSource<bool> connected;

  BrokerZenoh(Thread &, Config &);
  int init();
  int connect();
  int disconnect();
  int publisher(int, string);
  int subscriber(int, string, std::function<void(int,const bytes &)>);
  int publish(int, bytes &);
  int onSubscribe(SubscribeCallback);
  int unSubscriber(int);
  vector<PubMsg> query(string);
};
// namespace zenoh
#endif // _ZENOH_SESSION_h_