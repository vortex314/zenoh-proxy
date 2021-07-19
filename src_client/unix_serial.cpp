/*
 * Copyright (c) 2017, 2020 ADLINK Technology Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
 * which is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * Contributors:
 *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#include <arpa/inet.h>
#include <errno.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
extern "C" {
#include "zenoh-pico/net/private/system.h"
#include "zenoh-pico/private/logging.h"
}
#include "Log.h"
#include "util.h"

#include <ppp_frame.h>
#include <serial.h>
#include <serial_protocol.h>

using namespace std;
const char *port = "/dev/ttyUSB1";
static Serial serial;
std::deque<uint8_t> _inputStream;
std::deque<uint8_t> _eventStream;
uint64_t _lastFrameFlag;
uint64_t _frameTimeout = 2000;
bytes _inputFrame;
bytes _cleanData;
static bytes empty(0, 0);

int initSerial() {
  if (!serial.connected()) {
    serial.port(port);
    serial.baudrate(115200);
    serial.init();
    serial.connect();
  }
  return serial.fd();
}

int sendFrame(uint8_t header, const uint8_t *data, uint32_t length) {
  bytes buffer;
  buffer.push_back(header);
  for (uint32_t i = 0; i < length; i++)
    buffer.push_back(data[i]);
  bytes outFrame = ppp_frame(buffer);
  serial.txd(outFrame);
  return length;
}

bool handleFrame(bytes &bs) {

  if (bs.size() == 0)
    return false;
  if (ppp_deframe(_cleanData, bs)) {
    uint8_t event = _cleanData[0];
    if (event == TCP_RECV) {
      INFO(" TCP_RECV %s ", hexDump(_cleanData).c_str());
      for (uint8_t b : _cleanData)
        _inputStream.push_back(b);
      return true;
    } else if (event == TCP_CONNECTED) {
      INFO(" TCP_CONNECTED %s ", hexDump(_cleanData).c_str());
      _eventStream.push_back(TCP_CONNECTED);
      return true;
    } else if (event == TCP_DISCONNECTED) {
      INFO(" TCP_DISCONNECTED %s ", hexDump(_cleanData).c_str());
      _eventStream.push_back(TCP_DISCONNECTED);
      return true;
    } else {
      INFO(" UNKNOWN %s ", hexDump(_cleanData).c_str());
      return false;
    }
  } else {
    //    toStdout(bs);
    INFO(" garbage %s ", hexDump(bs).c_str());
    return false;
  }
}

void handleRxd(bytes &bs) {
  for (uint8_t b : bs) {
    if (b == PPP_FLAG_CHAR) {
      _lastFrameFlag = Sys::millis();
      handleFrame(_inputFrame);
      _inputFrame.clear();
    } else {
      _inputFrame.push_back(b);
    }
  }
  if ((Sys::millis() - _lastFrameFlag) > _frameTimeout) {
    //   cout << " skip  bytes " << hexDump(bs) << endl;
    //   cout << " frame data drop " << hexDump(frameData) << flush;
    //  toStdout(bs);
    _inputFrame.clear();
  }
}

int recvData(uint8_t *buffer, uint32_t capacity) {
  bytes rxdBuffer;
  INFO(" reading data");
  while (_inputStream.size() < capacity) {
    serial.waitForRxd(100);
    int rc = serial.rxd(rxdBuffer);
    if (rc == 0) {                 // read ok
      if (rxdBuffer.size() == 0) { // but no data
        WARN(" 0 data ");
      } else {
        INFO(" SERIAL RXD[%d]", rxdBuffer.size());
        handleRxd(rxdBuffer);
        rxdBuffer.clear();
      }
    }
  }
  for (uint32_t i = 0; i < capacity; i++) {
    buffer[i] = _inputStream.front();
    _inputStream.pop_front();
  }
  return capacity;
}

//===============================

void dump(const char *prefix, const uint8_t *data, uint32_t length) {
  INFO("%s %s", prefix, hexDump(bytes(data, data + length)).c_str());
  INFO("%s %s", prefix, charDump(bytes(data, data + length)).c_str());
}

/*------------------ Interfaces and sockets ------------------*/
char *_zn_select_scout_iface() {
  static char unknown[256] = "UNKNOWN";
  ERROR("_zn_select_scout_iface() should not be called ! ");
  return unknown;
}

struct sockaddr_in *_zn_make_socket_address(const char *addr, int port) {
  INFO(" make_socket_addr(%s,%d) should not be called ", addr, port);
  return 0;
}

_zn_socket_result_t _zn_create_udp_socket(const char *addr, int port,
                                          int timeout_usec) {
  WARN("create_udp_socket(%s,%d,%d) should not be called ", addr, port,
       timeout_usec);
  _zn_socket_result_t r;

  r.tag = _z_res_t_ERR;
  r.value.error = r.value.socket;
  r.value.socket = -1;
  return r;
}

_zn_socket_result_t _zn_open_tx_session(const char *locator) {
  initSerial();
  sendFrame(TCP_OPEN, 0, 0);
  INFO("open_tx_session(%s)", locator);
  _zn_socket_result_t r;
  r.tag = _z_res_t_OK;
  return r;
}

void _zn_close_tx_session(_zn_socket_t sock) {
  INFO("close_tx_session(%d)", sock);
  sendFrame(TCP_CLOSE, 0, 0);
}

/*------------------ Datagram ------------------*/
int _zn_send_dgram_to(_zn_socket_t sock, const _z_wbuf_t *wbf,
                      const struct sockaddr *dest, socklen_t salen) {
  WARN("send_dgram_to(%d) should not be called !", sock);
  return -1;
}

int _zn_recv_dgram_from(_zn_socket_t sock, _z_zbuf_t *rbf,
                        struct sockaddr *from, socklen_t *salen) {
  WARN("recv_dgram_from(%d) should not be called!", sock);
  return -1;
}

/*------------------ Receive ------------------*/
int _zn_recv_bytes(_zn_socket_t sock, uint8_t *ptr, size_t len) {
  INFO("_zn_recv_bytes(%d,,%d)", sock, len);
  int n = len;
  int rb;

  do {
    rb = recvData(ptr, n);
    INFO("%d", rb);
    if (rb == 0)
      return -1;
    n -= rb;
    ptr = ptr + (len - n);
  } while (n > 0);
  INFO("");
  return 0;
}

int _zn_recv_zbuf(_zn_socket_t sock, _z_zbuf_t *rbf) {
  int rb = recvData(_z_zbuf_get_wptr(rbf), _z_zbuf_space_left(rbf));

  if (rb > 0) {
    _z_zbuf_set_wpos(rbf, _z_zbuf_get_wpos(rbf) + rb);
  }
  return rb;
}

/*------------------ Send ------------------*/
int _zn_send_wbuf(_zn_socket_t sock, const _z_wbuf_t *wbf) {
  for (size_t i = 0; i < _z_wbuf_len_iosli(wbf); i++) {
    z_bytes_t bs = _z_iosli_to_bytes(_z_wbuf_get_iosli(wbf, i));
    int n = bs.len;
    int wb;
    do {
      _Z_DEBUG("Sending wbuf on socket...");
      dump("send()", bs.val, n);
      wb = sendFrame(TCP_SEND, bs.val, n);
      _Z_DEBUG_VA(" sent %d bytes\n", wb);
      if (wb <= 0) {
        _Z_DEBUG_VA("Error while sending data over socket [%d]\n", wb);
        return -1;
      }
      n -= wb;
      bs.val += bs.len - n;
    } while (n > 0);
  }

  return 0;
}

// size_t _zn_iovs_len(struct iovec *iov, int iovcnt)
// {
//     size_t len = 0;
//     for (int i = 0; i < iovcnt; ++i)
//         len += iov[i].iov_len;
//     return len;
// }

// int _zn_compute_remaining(struct iovec *iov, int iovcnt, size_t sent)
// {
//     size_t idx = 0;
//     int i = 0;
//     while (idx + iov[i].iov_len <= sent)
//     {
//         idx += sent;
//         i += 1;
//     }
//     int j = 0;
//     if (idx + iov[i].iov_len > sent)
//     {
//         iov[0].iov_base = ((unsigned char *)iov[i].iov_base) + (sent - idx -
//         iov[i].iov_len); j = 1; while (i < iovcnt)
//         {
//             iov[j] = iov[i];
//             j++;
//             i++;
//         }
//     }
//     return j;
// }

// int _zn_send_iovec(_zn_socket_t sock, struct iovec *iov, int iovcnt)
// {
//     int len = 0;
//     for (int i = 0; i < iovcnt; ++i)
//         len += iov[i].iov_len;

//     int n = writev(sock, iov, iovcnt);
//     _Z_DEBUG_VA("z_send_iovec sent %d of %d bytes \n", n, len);
//     while (n < len)
//     {
//         iovcnt = _zn_compute_remaining(iov, iovcnt, n);
//         len = _zn_iovs_len(iov, iovcnt);
//         n = writev(sock, iov, iovcnt);
//         _Z_DEBUG_VA("z_send_iovec sent %d of %d bytes \n", n, len);
//         if (n < 0)
//             return -1;
//     }
//     return 0;
// }
