#include <ArduinoJson.h>
#include <Log.h>
#include <Udp.h>
#include <stdio.h>

#include <config.h>
#include <thread>
#include <unordered_map>
#include <utility>
extern "C" {
#include "zenoh-pico/net.h"
}

using namespace std;

void overrideConfig(JsonObject &config, int argc, char **argv);

#define DEFAULT_CONFIG "/home/lieven/workspace/zenoh-proxy/zenoh-proxy.json"

Log logger(2048);
// Config config;
#define MAX_PORT 20

std::string logFile = "";
FILE *logFd = 0;

void myLogFunction(char *s, uint32_t length) {
  fprintf(logFd, "%s\n", s);
  fflush(logFd);
  fprintf(stdout, "%s\r\n", s);
}

void SetThreadName(std::thread *thread, const char *threadName) {}

StaticJsonDocument<20000> doc;

bool parse(JsonObject &cfg, string data) {
  DeserializationError error;

  error = deserializeJson(doc, data);
  if (error) {
    ERROR("deserializeJson() failed: %s", error.c_str());
    ::exit(-1);
  }
  return true;
}

int main(int argc, char **argv) {
  DeserializationError error;
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

  JsonObject jsonConfig;
  string sConfig = "{}";
  Sys::init();
  INFO("build : " __DATE__ " " __TIME__);
  if (argc > 1) {
    INFO(" loading config file : %s ", argv[1]);
    sConfig = loadFile(argv[1]);
  } else {
    INFO(" load default config : %s", DEFAULT_CONFIG);
    sConfig = loadFile(DEFAULT_CONFIG);
  }

  parse(jsonConfig, sConfig);

  overrideConfig(jsonConfig, argc, argv);
  if (logFile.length() > 0) {
    INFO(" logging to file %s ", logFile.c_str());
    logFd = fopen(logFile.c_str(), "w");
    if (logFd == NULL) {
      WARN(" open logfile %s failed : %d %s ", logFile.c_str(), errno,
           strerror(errno));
    } else {
      //     logger.setOutput(myLogFunction);
    }
  }
}

void overrideConfig(JsonObject &config, int argc, char **argv) {
  int opt;

  while ((opt = getopt(argc, argv, "f:m:l:v:")) != -1) {
    switch (opt) {
    case 'm':
      config["mqtt"]["host"] = optarg;
      break;
    case 'f':
      parse(config, loadFile(optarg));
      break;
    case 'v': {
      char logLevel = optarg[0];
      logger.setLogLevel(logLevel);
      break;
    }
    case 'l':
      logFile = optarg;
      break;
    default: /* '?' */
      fprintf(stderr,
              "Usage: %s [-v(TDIWE)] [-f configFile] [-l logFile] [-m "
              "mqttHost]\n",
              argv[0]);
      exit(EXIT_FAILURE);
    }
  }
}
