#ifndef _BROKER_PROTOCOL_H_
#define _BROKER_PROTOCOL_H_
enum
{
  B_CONNECT, // broker connect
  B_DISCONNECT,
  B_PUBLISH,    // id , bytes value
  B_SUBSCRIBER, // string ,id, qos,
  B_PUBLISHER,  // string ,id,  qos
  B_RESOURCE,   // string ,id
};
#endif