
#include "util.h"
#include <Log.h>
#include <regex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
using namespace std;

typedef uint32_t Rid;

class Endpoint {
public:
  virtual void send(bytes &) = 0;
};

class SerialEndpoint : public Endpoint {
public:
  void send(bytes &);
};

bool match(string pattern, string value) {
  return regex_match(value, regex(pattern));
}

struct Subscriber {
  set<string> patterns;
  Endpoint &endpoint;
};

class Broker {
  vector<Subscriber *> subscribers;
  unordered_map<Rid, string> rid2key;
  unordered_map<Rid, bytes> rid2value;
  unordered_map<Rid, set<Subscriber *>> rid2subs;

public:
  void onPublish(Rid key, bytes &value) {
    rid2value.emplace(key, value);
    auto got = rid2subs.find(key);
    if (got != rid2subs.end()) {
      for (Subscriber *sub : got->second) {
        sub->endpoint.send(value);
      }
    }
  }

  bool isNewRid(Rid rid) { return rid2key.find(rid) == rid2key.end(); }
  void addRidKey(Rid rid, string &key) { rid2key.emplace(rid, key); }

  void onPublish(string key, bytes &value) {
    Rid rid = H(key.c_str());
    if (isNewRid(rid)) {
      addRidKey(rid, key);
      for (Subscriber *sub : subscribers) {
        for (string pattern : sub->patterns) {
          if (match(pattern, key))
            addSubscriberOnRid(rid, sub);
        }
      }
    };
    onPublish(rid, value);
  }

  void onNewSubscriber(Subscriber *sub) { subscribers.push_back(sub); }

  void onNewKey(string key) { rid2key.emplace(H(key.c_str()), key); }

  void addSubscriberOnRid(Rid rid, Subscriber *sub) {
    auto cursor = rid2subs.find(rid);
    if (cursor == rid2subs.end()) {
      set<Subscriber *> subs;
      subs.emplace(sub);
      rid2subs.emplace(rid, subs);
    } else {
      cursor->second.emplace(sub);
    }
  }

  void onSubscribe(Subscriber *sub, string pattern) {
    sub->patterns.emplace(pattern);
    unordered_map<Rid, string>::const_iterator it = rid2key.begin();
    while (it != rid2key.end()) {
      string key = it->second;
      Rid rid = it->first;
      if (match(key, pattern)) {
        addSubscriberOnRid(rid, sub);
      }
      it++;
    }
  }
};

class ConsoleEndpoint : public Endpoint {
  string _name;

public:
  ConsoleEndpoint(const char *name) : _name(name) {}
  void send(bytes &bs) { INFO("%s : %s ", _name.c_str(), hexDump(bs).c_str()); }
} console1("console1");

Log logger(1024);

int main(int argc, char **argv) {
  Broker broker;
  Subscriber sub({{}, console1});
  broker.onNewSubscriber(&sub);
  //  broker.onSubscribe(&sub, "a.*");
  broker.onSubscribe(&sub, ".a.*");
  bytes data;
  data.push_back(0xAA);
  broker.onPublish("aaa", data);
  for (uint32_t i = 0; i < 1000; i++)
    broker.onPublish(H("aaa"), data);
}