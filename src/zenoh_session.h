#ifndef _ZENOH_SESSION_H_
#define _ZENOH_SESSION_H_
#include "limero.h"
#include "util.h"
extern "C" {
#include "zenoh/net.h"
}
#define US_WEST "tcp/us-west.zenoh.io:7447"
#define US_EAST "tcp/us-east.zenoh.io:7447"

using namespace std;
namespace zenoh {

typedef struct Message {
  string key;
  bytes data;
} Message;

typedef unordered_map<unsigned long, string> Properties;
typedef unsigned long ResourceKey;

class Session : public Actor {
  zn_session_t *_zenoh_session;
  vector<zn_subscriber_t *> _subscribers;

public:
  ValueSource<bool> connected;
  ValueSource<Message> incoming;

  Session(Thread &, Config &);
  int scout();
  int open(Properties &);
  void close();
  Properties &info();
  int subscribe(string);
  static void dataHandler(const zn_sample_t *, const void *);
  int publish(string topic, bytes &);
  ResourceKey resource(string topic);
};

class Resource {};
};     // namespace zenoh
#endif // _ZENOH_SESSION_h_