
#include <BrokerSerial.h>
#include <util.h>
const int MsgPublish::TYPE;
const int MsgPublisher::TYPE;
const int MsgSubscriber::TYPE;
const int MsgConnect::TYPE;
const int MsgDisconnect::TYPE;
//================================================================
class MsgFilter : public LambdaFlow<bytes, bytes>
{
  int _msgType;
  MsgBase msgBase;
  CborDeserializer _fromCbor;

public:
  MsgFilter(int msgType)
      : LambdaFlow<bytes, bytes>([this](bytes &out, const bytes &in)
                                 {
                                   if (msgBase.reflect(_fromCbor.fromBytes(in)).success() &&
                                       msgBase.msgType == _msgType)
                                   {
                                     out = in;
                                     return true;
                                   }
                                   return false;
                                 }),
        _fromCbor(1024)
  {
    _msgType = msgType;
  };
  static MsgFilter &nw(int msgType) { return *new MsgFilter(msgType); }
};

BrokerSerial::BrokerSerial(Thread &thr, Stream &serial)
    : Broker(thr),
      _serial(serial),
      _toCbor(100), _fromCbor(100),
      keepAliveTimer(thr, 1000, true),
      connectTimer(thr, 10000, true) {}
BrokerSerial::~BrokerSerial() {}

void BrokerSerial::init()
{
  // outgoing
  connected = false;
  uptimePub = &publisher<uint64_t>("system/uptime");
  latencyPub = &publisher<uint64_t>("system/latency");
  INFO(" %X %X ", uptimePub, latencyPub);
  uptimeSub = &subscriber<uint64_t>(brokerSrcPrefix + "system/uptime");

  outgoingFrame >> _frameToBytes;
  _frameToBytes >> [&](const bytes &bs)
  { _serial.write(bs.data(), bs.size()); };

  serialRxd >> _bytesToFrame;
  _bytesToFrame >> MsgFilter::nw(B_PUBLISH) >> incomingPublish;
  _bytesToFrame >> MsgFilter::nw(B_CONNECT) >> incomingConnect;
  _bytesToFrame >> MsgFilter::nw(B_DISCONNECT) >> incomingDisconnect;

/*  _bytesToFrame >> [&](const bytes &bs)
  { INFO("RXD %s", cborDump(bs).c_str()); };*/

  keepAliveTimer >> [&](const TimerMsg &tm)
  {
//    INFO(" connected : %s ", connected() ? "true" : "false");
    if (connected())
      uptimePub->on(Sys::millis());
    else
    {
      MsgConnect msgConnect = {brokerSrcPrefix};
      outgoingFrame.on(msgConnect.reflect(_toCbor).toBytes());
    }
  };
  incomingConnect >>
      [&](const bytes &in)
  {
    connected = true;
    INFO(" subscribers : %d publishers : %d ", _subscribers.size(),
         _publishers.size());
    for (auto sub : _subscribers)
    {
      MsgSubscriber msgSubscriber = {sub->id(), sub->key()};
      outgoingFrame.on(msgSubscriber.reflect(_toCbor).toBytes());
    }
    for (auto pub : _publishers)
    {
      MsgPublisher msgPublisher = {pub->id(), pub->key()};
      outgoingFrame.on(msgPublisher.reflect(_toCbor).toBytes());
    }
  };

  *uptimeSub >> [&](const uint64_t &t)
  {
    //    INFO(" received uptime %lu ", t);
    latencyPub->on(Sys::millis() - t);
    _loopbackReceived = Sys::millis();
  };

  connectTimer >> [&](const TimerMsg &tm)
  {
    uint64_t timeSinceLoopback = Sys::millis() - _loopbackReceived;
    if (timeSinceLoopback > 5000)
      connected = false;
  };

  _serial.setTimeout(0);
}

void BrokerSerial::onRxd(void *me)
{
  BrokerSerial *brk = (BrokerSerial *)me;
  bytes data;
  while (brk->_serial.available())
  {
    data.push_back(brk->_serial.read());
  }
  brk->serialRxd.emit(data);
}
