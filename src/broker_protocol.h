#ifndef _BROKER_PROTOCOL_H_
#define _BROKER_PROTOCOL_H_
#include <util.h>

typedef enum { NONE=0, READ_ONLY = 1 } ReflectorMode;

typedef enum {
  B_CONNECT, // broker connect
  B_DISCONNECT,
  B_SUBSCRIBER, // TXD string ,id, qos,
  B_PUBLISHER,  // TXD string ,id,  qos
  B_PUBLISH,    // RXD,TXD id , bytes value
  B_RESOURCE,   // RXD id
} MsgType;

struct MsgBase {
  int msgType;
  template <typename Reflector> Reflector &reflect(Reflector &r) {
    r.member("msgType", msgType, "type of polymorphic message");
    return r;
  }
};

struct MsgConnect {
  string clientId;
  static const int TYPE = B_CONNECT;
  template <typename Reflector> Reflector &reflect(Reflector &r) {
    r.member("msgType", TYPE, "MsgConnect")
        .member("clientId", clientId, "client identification");
    return r;
  }
};

struct MsgDisconnect {
  static const int TYPE = B_DISCONNECT;
  template <typename Reflector> Reflector &reflect(Reflector &r) {
    r.member("msgType", TYPE, "MsgDisconnect");
    return r;
  }
};

struct MsgPublish {
  int id;
  bytes value;
  static const int TYPE = B_PUBLISH;
  template <typename Reflector> Reflector &reflect(Reflector &r) {
    r.member("msgType", TYPE, "PublishMsg")
        .member("id", id, "resource id")
        .member("value", value, "data published");
    return r;
  }
};
struct MsgPublisher {
  int id;
  string topic;
  static const int TYPE = B_PUBLISHER;
  template <typename Reflector> Reflector &reflect(Reflector &r) {
    r.member("msgType", TYPE, "MsgPublisher")
        .member("id", id, "resource id")
        .member("topic", topic, "publish topic");
    return r;
  }
};

struct MsgSubscriber {
  int id;
  string topic;
  static const int TYPE = B_SUBSCRIBER;
  template <typename Reflector> Reflector &reflect(Reflector &r) {
    r.member("msgType", TYPE, "MsgSubscriber")
        .member("id", id, "resource id")
        .member("topic", topic, "publish topic");
    return r;
  }
};
#endif