#include <limero.h>
#include <vector>
#include <cbor11.h>
#include <broker_protocol.h>
#include <util.h>
#include <ppp_frame.h>
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
    class Subscriber : public LambdaFlow<cbor, T>, public Resource
    {

    public:
        Subscriber(int id, string key) : LambdaFlow<cbor, T>([](T &t, const cbor &data)
                                                             {
                                                                 t = data.to_array()[2];
                                                                 return true;
                                                             }),
                                         Resource(id, key){};
    };

    template <typename T>
    class Publisher : public LambdaFlow<T, cbor>, public Resource
    {

    public:
        Publisher(int id, string key) : LambdaFlow<T, cbor>([&](cbor &cb, const T &t)
                                                            {
                                                                cb = cbor::array{B_PUBLISH, Resource::id(), t};
                                                                INFO("%s : %s ", Resource::key().c_str(), cbor::debug(cb).c_str());
                                                                return true;
                                                            }),
                                        Resource(id, key){};
    };

    class Broker : public Actor
    {
    public:
        ValueFlow<cbor> incomingCbor;
        ValueFlow<cbor> outgoingCbor;
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
        _subscribers.push_back(s);
        incomingCbor >> [&](const cbor &in)
        {
            int msgType = in.to_array()[1];
            if (msgType == s->id())
                s->on(in);
        };
        incomingCbor >> s;
        return *s;
    }
};
