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
#include "serial.h"
#include "util.h"
#include <limero.h>
#include <ppp_frame.h>
#include <serial_protocol.h>

/*------------------ Interfaces and sockets ------------------*/
char *_zn_select_scout_iface();
struct sockaddr_in *_zn_make_socket_address(const char *addr, int port);
_zn_socket_result_t _zn_create_udp_socket(const char *addr, int port,
                                          int timeout_usec);

_zn_socket_result_t _zn_open_tx_session(const char *locator);
void _zn_close_tx_session(_zn_socket_t sock);

/*------------------ Datagram ------------------*/
int _zn_send_dgram_to(_zn_socket_t sock, const _z_wbuf_t *wbf,
                      const struct sockaddr *dest, socklen_t salen);
int _zn_recv_dgram_from(_zn_socket_t sock, _z_zbuf_t *rbf,
                        struct sockaddr *from, socklen_t *salen);
/*------------------ Receive ------------------*/
int _zn_recv_bytes(_zn_socket_t sock, uint8_t *ptr, size_t len);
int _zn_recv_zbuf(_zn_socket_t sock, _z_zbuf_t *rbf);
/*------------------ Send ------------------*/
int _zn_send_wbuf(_zn_socket_t sock, const _z_wbuf_t *wbf);
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
//         iov[0].iov_base = ((unsigned char *)iov[i].iov_base) + (sent - idx
//         - iov[i].iov_len); j = 1; while (i < iovcnt)
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