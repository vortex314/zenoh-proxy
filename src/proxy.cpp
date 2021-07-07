#include <ArduinoJson.h>
#include <Log.h>
#include <Udp.h>
#include <stdio.h>

#include <config.h>
#include <thread>
#include <unordered_map>
#include <utility>

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

  JsonObject config;
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

  parse(config,sConfig);

  overrideConfig(config, argc, argv);
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

  Udp udp;
  udp.port(udpPort);
  udp.init();
  UdpMsg udpMsg;
  udp.receive(udpMsg);
  in_addr_t fromAddress = udpMsg.srcIp;
  uint16_t fromPort = udpMsg._srcPort;
}

void overrideConfig(JsonObject &config, int argc, char **argv) {
  int opt;

  while ((opt = getopt(argc, argv, "f:m:l:v:")) != -1) {
    switch (opt) {
    case 'm':
      config["mqtt"]["host"] = optarg;
      break;
    case 'f':
      parse(config,loadFile(optarg));
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
