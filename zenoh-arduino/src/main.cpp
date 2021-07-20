#include <Arduino.h>
#include <BrokerSerial.h>
#include <limero.h>

#include <deque>
#define PIN_LED 2
#define PIN_BUTTON 0
#include <LedBlinker.h>
#include <Button.h>

Thread mainThread("main");
LedBlinker ledBlinkerBlue(mainThread, PIN_LED, 100);
Button button1(mainThread, PIN_BUTTON);
Poller poller(mainThread);
BrokerSerial brkr(mainThread,Serial);
Log logger(1024);

LambdaSource<uint32_t> systemHeap([]() { return ESP.getFreeHeap(); });
LambdaSource<uint64_t> systemUptime([]() { return Sys::millis(); });
LambdaSource<const char *> systemHostname([]() { return Sys::hostname(); });
LambdaSource<const char *> systemBoard([]() { return Sys::board(); });
LambdaSource<const char *> systemCpu([]() { return Sys::cpu(); });
ValueSource<const char *> systemBuild = __DATE__ " " __TIME__;

void serialEvent() {
  INFO("-");
  BrokerSerial::onRxd(&brkr);
}

void setup() {
  Serial.begin(921600);
  Serial.println("\r\n===== Starting  build " __DATE__ " " __TIME__);

  Sys::hostname(S(HOSTNAME));

  button1.init();
  ledBlinkerBlue.init();
  brkr.init();
/*
  zenoh.connected >> ledBlinkerBlue.blinkSlow;
  zenoh.connected >> poller.connected;
  zenoh.connected >> zenoh.toTopic<bool>("mqtt/connected");

  poller >> systemHeap >> zenoh.toTopic<uint32_t>("system/heap");
  poller >> systemUptime >> zenoh.toTopic<uint64_t>("system/upTime");
  poller >> systemBuild >> zenoh.toTopic<const char *>("system/build");
  poller >> systemHostname >> zenoh.toTopic<const char *>("system/hostname");
  poller >> systemBoard >> zenoh.toTopic<const char *>("system/board");
  poller >> systemCpu >> zenoh.toTopic<const char *>("system/cpu");
  poller >> button1 >> zenoh.toTopic<bool>("button/button1");*/

}

void loop() {
  mainThread.loop();
  if (Serial.available()) {
    BrokerSerial::onRxd(&brkr);
  }
}
