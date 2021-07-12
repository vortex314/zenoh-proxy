#include <ArduinoJson.h>
#include <Log.h>
#include <stdio.h>

#include <config.h>
#include <thread>
#include <unordered_map>
#include <utility>

using namespace std;

Log logger(2048);
#include <serial_session.h>
#include <tcp_session.h>

//====================================================

typedef enum {
  EV_TCP_OPEN,
  EV_TCP_CLOSE,
  EV_TCP_SEND,
  EV_TCP_RECV,
  EV_SERIAL_RXD,
  EV_SERIAL_DISCONNECTED
} EventType;

Config loadConfig(int argc, char **argv) { return Config(); };

bytes header_frame(uint8_t header, const bytes &data) {
  bytes msg;
  msg.push_back(header);
  msg.insert(msg.end(), data.begin(), data.end());
  return msg;
}

bytes cmd_frame(uint8_t header) {
  bytes msg;
  msg.push_back(header);
  return msg;
}

int main(int argc, char **argv) {

  Config config = loadConfig(argc, argv);
  Thread workerThread("worker");

  Config serialConfig = config["serial"];
  SerialSession serial(workerThread, serialConfig);

  Config tcpConfig = config["tcp"];
  TcpSession tcp(workerThread, tcpConfig);

  serial.init();
  serial.connect();

  tcp.incoming >>
      *new LambdaFlow<bytes, bytes>([&](bytes &result, const bytes &data) {
        result = ppp_frame(header_frame(TCP_RECV, data));
        return true;
      }) >>
      serial.outgoing;

  serial.incoming >> *new Sink<bytes>(3, [&](const bytes &frame) {
    uint8_t header = frame[0];
    if (header == TCP_OPEN) {
      INFO("TCP_OPEN");
      tcp.connect();
    } else if (header == TCP_CLOSE) {
      INFO("TCP_CLOSE");
      tcp.disconnect();
    } else if (header == TCP_SEND) {
      INFO("TCP_SEND %s", hexDump(frame).c_str());
      tcp.outgoing.on(bytes(&frame[1], &frame[1] + frame.size() - 1));
    }
  });

  tcp.connected >> [&](const bool &isConnected) {
    INFO(" TCP %s ", isConnected ? "CONNECTED" : "DISCONNECTED");
    if (isConnected)
      serial.outgoing.on(cmd_frame(TCP_CONNECTED));
    else
      serial.outgoing.on(cmd_frame(TCP_DISCONNECTED));
  };

  serial.connected >> [&](const bool isConnected) {
    if (!isConnected)
      tcp.disconnect();
  };

  workerThread.run();
  serial.disconnect();
  tcp.connect();
  tcp.disconnect();
}
