#include <ArduinoJson.h>
#include <Log.h>
#include <stdio.h>

#include <config.h>
#include <thread>
#include <unordered_map>
#include <utility>
extern "C" {
#include "zenoh-pico/net.h"
}
#include <unistd.h>

using namespace std;

Log logger(2048);

int main(int argc, char **argv) {
  int udpPort = 7447;
  string host = "localhost";
  char uriDefault[1024] = "/demo/example/zenoh-pico-pub";
  char *uri = uriDefault;
  if (argc > 1) {
    uri = argv[1];
  }
  const char *value = "Pub from pico!";
  if (argc > 2) {
    value = argv[2];
  }

  zn_properties_t *config = zn_config_default();
  if (argc > 3) {
    zn_properties_insert(config, ZN_CONFIG_PEER_KEY, z_string_make(argv[3]));
  }

  printf("Openning session...\n");
  zn_session_t *s = zn_open(config);
  if (s == 0) {
    printf("Unable to open session!\n");
    exit(-1);
  }

  // Start the receive and the session lease loop for zenoh-pico
  znp_start_read_task(s);
  znp_start_lease_task(s);

  printf("Declaring Resource '%s'", uri);
  unsigned long rid = zn_declare_resource(s, zn_rname(uri));
  printf(" => RId %lu\n", rid);
  zn_reskey_t reskey = zn_rid(rid);

  printf("Declaring Publisher on %lu\n", rid);
  zn_publisher_t *pub = zn_declare_publisher(s, reskey);
  if (pub == 0) {
    printf("Unable to declare publisher.\n");
    exit(-1);
  }

  char buf[256];
  for (int idx = 0; 1; ++idx) {
    sleep(1);
    sprintf(buf, "[%4d] %s", idx, value);
    printf("Writing Data ('%lu': '%s')...\n", rid, buf);
    zn_write(s, reskey, (const uint8_t *)buf, strlen(buf));
  }

  zn_undeclare_publisher(pub);
  zn_close(s);
}
