#include <limero.h>
#include <vector>
#include <cbor11.h>
#include <broker_protocol.h>
#include <util.h>
#include <ppp_frame.h>
using namespace std;

namespace broker
{
    template <typename T>
    class Subscriber : public LambdaFlow<cbor, T>
    {
        int _id;
        string _key;

    public:
        Subscriber(int id, string key) : LambdaFlow<cbor, T>([](T &t, const cbor &data)
                                                             {
                                                                 t = data.to_array()[2];
                                                                 return true;
                                                             }),
                                         _id(id), _key(key){};
    };

    template <typename T>
    class Publisher : public LambdaFlow<T, cbor>
    {

        int _id;
        string _key;

    public:
        Publisher(int id, string key) : LambdaFlow<T, cbor>([&](cbor &cb, const T &t)
                                                            {
                                                                cb = cbor::array{B_PUBLISH, _id, t};
                                                                return true;
                                                            }),
                                        _id(id), _key(key){};
        int id() { return _id; }
    };

    class Broker : public Actor
    {
    public:
        QueueFlow<cbor> incomingCbor;
        QueueFlow<cbor> outgoingCbor;

        vector<Sink<cbor> *> _publishers;
        vector<Source<cbor> *> _subscribers;

    public:
        Broker(Thread thr):Actor(thr),incomingCbor(10),outgoingCbor(10){};
        template <typename T>
        Subscriber<T> &subscriber(string);
        template <typename T>
        Publisher<T> &publisher(string);
    };

    template <typename T>
    Publisher<T> &Broker::publisher(string name)
    {
        Publisher<T> *p = new Publisher<T>(_publishers.size(), name);
        //        _publishers.push_back(p);
        *p >> outgoingCbor;
        return *p;
    }

    template <typename T>
    Subscriber<T> &Broker::subscriber(string name)
    {
        Subscriber<T> *s = new Subscriber<T>(_subscribers.size(), name);
        /*        _subscribers.push_back(s);
        incoming >> [&](cbor &out, const cbor &in)
        {
            out = in;
            return in.to_array()[1] == s->id();
        } >> s;*/
        incomingCbor >> s;
        return *s;
    }
};
/*
void ffffffffffffffff()
{
    Thread worker("worker");
    broker::Broker broker(worker);
    broker.subscriber<uint64_t>("src/global/system/utc") >> broker.publisher<uint64_t>("system/time");
}
*/