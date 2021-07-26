
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

  serialRxd >> _bytesToFrame;
  _bytesToFrame >> [&](const bytes &bs)
  { INFO("RXD %s", cborDump(bs).c_str()); };

  outgoingCbor >> _toFrame;
  //  outgoingCbor.async(thread());

  _toFrame >> [&](const bytes &bs)
  { _serial.write(bs.data(), bs.size()); };

  keepAliveTimer >> [&](const TimerMsg &tm)
  {
    INFO(" connected : %s ", connected() ? "true" : "false");
    if (connected())
      uptimePub->on(Sys::millis());
    else
    {
      MsgConnect msgConnect = {brokerSrcPrefix};
      _toFrame.on(msgConnect.reflect(_toCbor).toBytes());
    }
  };
  _bytesToFrame >> filter<bytes>([&](const bytes &in) -> bool
                                 {
                                   INFO("filter %s", cborDump(in).c_str());
                                   MsgBase msgBase;
                                   INFO(" success : %s msgType : %d ", msgBase.reflect(_fromCbor.fromBytes(in)).success() ? "true" : "false", msgBase.msgType );
                                   return (msgBase.reflect(_fromCbor.fromBytes(in)).success() && msgBase.msgType == B_CONNECT);
                                 }) >>
      [&](const bytes &in)
  {
    connected = true;
    INFO(" subscribers : %d publishers : %d ", _subscribers.size(),
         _publishers.size());
    for (auto sub : _subscribers)
    {
      MsgSubscriber msgSubscriber = {sub->id(), sub->key()};
      _toFrame.on(msgSubscriber.reflect(_toCbor).toBytes());
    }
    for (auto pub : _publishers)
    {
      MsgPublisher msgPublisher = {pub->id(), pub->key()};
      _toFrame.on(msgPublisher.reflect(_toCbor).toBytes());
    }
  };

  *uptimeSub >> [&](const uint64_t &t)
  {
    latencyPub->on(t - Sys::millis());
    _loopbackReceived = Sys::millis();
  };

  connectTimer >> [&](const TimerMsg &tm)
  {
    uint64_t timeSinceLoopback = Sys::millis() - _loopbackReceived;
    if (timeSinceLoopback > 3000)
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
  //  INFO(" data : %d ", data.size());
  brk->serialRxd.emit(data);
}
