#include <broker_protocol.h>
#include <limero.h>
#include <ppp_frame.h>
#include <util.h>
#include <CborSerializer.h>
#include <CborDeserializer.h>

#include <vector>
using namespace std;

namespace broker
{

  class Resource
  {
    int _id;
    string _key;

  public:
    Resource(int i, string k) : _id(i), _key(k){};
    int id() { return _id; }
    const string &key() { return _key; }
  };

  template <typename T>
  class Subscriber : public LambdaFlow<bytes, T>, public Resource
  {
  public:
    Subscriber(int id, string key)
        : LambdaFlow<bytes, T>([](T &t, const bytes &data)
                               {
                                 CborDeserializer fromCbor(100);
                                 MsgPublish msgPublish;
          if ( msgPublish.reflect(fromCbor.fromBytes(data)).success() )
            {
              
            };
          return true;
                               }),
          Resource(id, key){};
    template <typename Reflector>
    Reflector &reflect(Reflector &r)
    {
      T t;
      r.begin().member("", t, "");
    }
  };

  template <typename T>
  class Publisher : public LambdaFlow<T, bytes>, public Resource
  {
  public:
    Publisher(int id, string key)
        : LambdaFlow<T, bytes>([&](bytes &cb, const T &t)
                               {
                                 CborSerializer toCbor(100);
                                 bytes bs = toCbor.begin().member(t).end().toBytes();
                                 MsgPublish msgPublish = {Resource::id(),bs};
                                 return true;
                               }),
          Resource(id, key){};
  };

  class Broker : public Actor
  {
  public:
    ValueFlow<bytes> incomingCbor;
    ValueFlow<bytes> outgoingCbor;
    int _resourceId = 0;

    vector<Resource *> _publishers;
    vector<Resource *> _subscribers;
    string brokerSrcPrefix = "src/esp32/";
    string brokerDstPrefix = "dst/esp32/";

  public:
    Broker(Thread thr) : Actor(thr){};
    template <typename T>
    Subscriber<T> &subscriber(string);
    template <typename T>
    Publisher<T> &publisher(string);
  };

  template <typename T>
  Publisher<T> &Broker::publisher(string name)
  {
    string topic = name;
    if (!(topic.find("src/") == 0 || topic.find("dst/") == 0))
    {
      topic = brokerSrcPrefix + name;
    }
    Publisher<T> *p = new Publisher<T>(_resourceId++, topic);
    _publishers.push_back(p);
    *p >> outgoingCbor;
    INFO(" created publisher %s : %X ", name.c_str(), p);
    return *p;
  }

  template <typename T>
  Subscriber<T> &Broker::subscriber(string name)
  {
    string topic = name;
    if (!(topic.find("src/") == 0 || topic.find("dst/") == 0))
    {
      topic = brokerDstPrefix + name;
    }
    Subscriber<T> *s = new Subscriber<T>(_resourceId++, topic);
    s->lambda([&](T &t, const bytes &in)
              {
                MsgPublish msgPublish;
                MsgBase msgBase;
                CborDeserializer fromCbor(100);
                if (msgBase.reflect(fromCbor.fromBytes(in)).success() && msgBase.msgType == B_PUBLISH && msgPublish.reflect(fromCbor.fromBytes(in)).success())
                {
                    //do some with msgPublish.value
                }
                return false;
              });
    _subscribers.push_back(s);
    incomingCbor >> s;
    INFO(" created subscriber %s : %X ", name.c_str(), s);
    return *s;
  }
}; // namespace broker
