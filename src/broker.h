#include <limero.h>
#include <util.h>

#include <functional>

class Publisher : public Source<bytes> {
  virtual int id();
  virtual const string& name();
  virtual int publish(const bytes&);
};

class Subscriber : public Source<bytes> {
  void on(const bytes&);
};

class Broker : public Actor {
 public:
  Broker(Thread&, Config&);
  virtual ~Broker();
  virtual int init();
  virtual int connect();
  virtual int disconnect();
  virtual Publisher* publisher(string);
  virtual Subscriber* subscriber(string);
  virtual int unSubscribe(Subscriber*);
};