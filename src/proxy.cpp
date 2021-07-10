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

int main(int argc, char **argv) {

  Config config = loadConfig(argc, argv);
  Thread workerThread("worker");

  Config serialConfig = config["serial"];
  SerialSession serial(workerThread, serialConfig);

  Config tcpConfig = config["tcp"];
  TcpSession tcp(workerThread, tcpConfig);

  serial.init();
  serial.connect();
  serial.disconnect();
  tcp.connect();
  tcp.disconnect();

  return 0;

  tcp.incoming >> serial.outgoing;
  serial.incoming >> tcp.outgoing;
  tcp.disconnected >> [&](const bool& state){
    serial.sendEvent(TCP_DISCONNECTED);
  };
  serial.command >> [&](const TcpCommand &cmd) {
    if (cmd == CMD_OPEN)
      tcp.connect();
    if (cmd == CMD_CLOSE)
      tcp.disconnect();
  };
  workerThread.run();
}
