#ifndef _BROKER_PROTOCOL_H_
#define _BROKER_PROTOCOL_H_
#include <util.h>
using namespace std;
typedef enum {
  B_CONNECT,  // broker connect
  B_DISCONNECT,
  B_SUBSCRIBER,  // TXD id, string, qos,
  B_PUBLISHER,   // TXD  id, string, qos
  B_PUBLISH,     // RXD,TXD id , bytes value
  B_RESOURCE,    // RXD id
} MsgType;

struct MsgBase {
  int msgType;
  template <typename Reflector>
  Reflector &reflect(Reflector &r) {
    r.begin().member(msgType, "msgType", "type of polymorphic message").end();
    //   INFO(" looking for int %d ", msgType);
    return r;
  }
};

struct MsgConnect {
  string clientId;
  static const int TYPE = B_CONNECT;
  template <typename Reflector>
  Reflector &reflect(Reflector &r) {
    return r.begin()
        .member(TYPE, "msgType", "MsgConnect")
        .member(clientId, "clientId", "client identification")
        .end();
  }
};

struct MsgDisconnect {
  static const int TYPE = B_DISCONNECT;

  template <typename Reflector>
  Reflector &reflect(Reflector &r) {
    return r.begin().member(TYPE, "msgType", "MsgDisconnect").end();
  }
};

struct MsgPublish {
  int id;
  bytes value;

  static const int TYPE = B_PUBLISH;

  template <typename Reflector>
  Reflector &reflect(Reflector &r) {
    return r.begin()
        .member(TYPE, "msgType", "PublishMsg")
        .member(id, "id", "resource id")
        .member(value, "value", "data published")
        .end();
  }
};
struct MsgPublisher {
  int id;
  string topic;

  static const int TYPE = B_PUBLISHER;

  template <typename Reflector>
  Reflector &reflect(Reflector &r) {
    return r.begin()
        .member(TYPE, "msgType", "MsgPublisher")
        .member(id, "id", "resource id")
        .member(topic, "topic", "publish topic")
        .end();
  }
};

struct MsgSubscriber {
  int id;
  string topic;

  static const int TYPE = B_SUBSCRIBER;

  template <typename Reflector>
  Reflector &reflect(Reflector &r) {
    return r.begin()
        .member(TYPE, "msgType", "MsgSubscriber")
        .member(id, "id", "resource id")
        .member(topic, "topic", "publish topic")
        .end();
  }
};
#endif