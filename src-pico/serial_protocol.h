#ifndef _SERIAL_PROTOCOL_H_
#define _SERIAL_PROTOCOL_H_
typedef enum {
  TCP_OPEN = 'O',
  TCP_CLOSE = 'C',
  TCP_SEND = 'S',
  TCP_RECV = 'R',
  TCP_CONNECTED = 'c',
  TCP_DISCONNECTED = 'd'
} MsgType;

typedef enum {
  Z_OPEN,
  Z_CLOSE,
  Z_SUBSCRIBE,
  Z_PUBLISH,
  Z_QUERY,
  Z_QUERYABLE,
  Z_RESOURCE
} CMD;


#endif