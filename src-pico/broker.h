#include <util.h>

#include <unordered_map>
#include <vector>
using namespace std;
class Broker {
  virtual int publish(const string&, const bytes&);
  virtual int subscribe(const string&);
  static Broker& create(Config&);
  virtual int unPublish(const string&);
  virtual int unSubscribe(const string&);
};

class PublisherProxy {
  string name;
  uint32_t id;
};

class BrokerProxy {
  unordered_map<uint32_t, string> _publishers;
  unordered_map<string, uint32_t> _subscribers;
  void onPublish(uint32_t, bytes);
  void onSubscribe(string);
};