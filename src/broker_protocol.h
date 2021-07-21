#ifndef _BROKER_PROTOCOL_H_
#define _BROKER_PROTOCOL_H_
enum {
  B_CONNECT,  // broker connect
  B_DISCONNECT,
  B_SUBSCRIBER,  // TXD string ,id, qos,
  B_PUBLISHER,   // TXD string ,id,  qos
  B_PUBLISH,     // RXD,TXD id , bytes value
  B_RESOURCE,    // RXD id
};
#endif