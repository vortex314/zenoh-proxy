
#include <broker_zenoh.h>
#include <cbor11.h>

void BrokerZenoh::dataHandler(const zn_sample_t *sample, const void *arg) {
  BrokerZenoh *broker = (BrokerZenoh *)arg;
  string key(sample->key.val, sample->key.len);
  bytes data(sample->value.val, sample->value.val + sample->value.len);
  //    INFO(" ZENOH RXD : %s : %s ", key.c_str(),
  //        cbor::debug(cbor::decode(data)).c_str());
  broker->incomingPub.emit({1, data});
}

BrokerZenoh::BrokerZenoh(Thread &thr, Config &cfg) : Actor(thr) {
  _zenoh_session = 0;
}

int Session::scout() {
  zn_properties_t *config = zn_config_default();
  z_string_t us_west = z_string_make(US_WEST);
  //  zn_properties_insert(config, ZN_CONFIG_PEER_KEY, us_west);
  zn_properties_insert(config, ZN_CONFIG_LISTENER_KEY, us_west);

  zn_hello_array_t hellos = zn_scout(ZN_ROUTER | ZN_PEER, config, 3000);
  if (hellos.len > 0) {
    for (unsigned int i = 0; i < hellos.len; ++i) {
      zn_hello_t hello = hellos.val[i];
      z_str_array_t locators = hello.locators;
      bytes pid;
      if (hello.pid.val != NULL) {
        pid = bytes(hello.pid.val, hello.pid.val + hello.pid.len);
      }
      INFO(" whatami: %s pid : %s ",
           hello.whatami == ZN_ROUTER ? "ZN_ROUTER"
           : hello.whatami == ZN_PEER ? "ZN_PEER"
                                      : "OTHER",
           hexDump(pid, "").c_str());
      for (unsigned int i = 0; i < locators.len; i++) {
        INFO(" locator : %s ", locators.val[i]);
      }
    }
    zn_hello_array_free(hellos);
  } else {
    WARN("Did not find any zenoh process.\n");
  }
  return 0;
}

int BrokerZenoh::connect() {
  zn_properties_t *config = zn_config_default();
  //   zn_properties_insert(config, ZN_CONFIG_LISTENER_KEY,
  //   z_string_make(US_WEST));

  if (_zenoh_session == 0)  // idempotent
    _zenoh_session = zn_open(config);
  INFO("zn_open() %s.", _zenoh_session == 0 ? "failed" : "succeeded");
  return _zenoh_session == 0 ? -1 : 0;
}

void BrokerZenoh::disconnect() {
  for (auto tuple : _subscribers) {
    zn_undeclare_subscriber(tuple.second);
  }
  _subscribers.clear();
  //  if (_zenoh_session)
  //   zn_close(_zenoh_session);
  // _zenoh_session = NULL;
}

int BrokerZenoh::subscriber(string resource) {
  if (_subscribers.find(resource) == _subscribers.end()) {
    zn_subscriber_t *sub =
        zn_declare_subscriber(_zenoh_session, zn_rname(resource.c_str()),
                              zn_subinfo_default(), dataHandler, this);
    if (sub == NULL) {
      WARN(" subscription failed for %s ", resource.c_str());
      return -1;
    }
    _subscribers.emplace(resource, sub);
  }
  return 0;
}

int BrokerZenoh::publisher(int id, string key) {
  if (_publishers.find(id) == _publishers.end()) {
    zn_reskey_t reskey = resource(key);
    zn_publisher_t *pub = zn_declare_publisher(s, reskey);
    if (pub == 0) WARN(" unable to declar publisher %s", key.c_str());
    _publishers.emplace(id, pub);
  }
  return 0;
}

int BrokerZenoh::publish(int topic, bytes &bs) {
  return zn_write(_zenoh_session, zn_rname(topic.c_str()), bs.data(),
                  bs.size());
}

zn_reskey_t BrokerZenoh::resource(string topic) {
  zn_reskey_t rid =
      zn_rid(zn_declare_resource(_zenoh_session, zn_rname(topic.c_str())));
  return rid;
}

vector<Message> Session::query(string uri) {
  vector<Message> result;
  zn_reply_data_array_t replies = zn_query_collect(
      _zenoh_session, zn_rname(uri.c_str()), "", zn_query_target_default(),
      zn_query_consolidation_default());
  for (unsigned int i = 0; i < replies.len; ++i) {
    result.push_back(
        {string(replies.val[i].data.key.val, replies.val[i].data.key.len),
         bytes(replies.val[i].data.value.val,
               replies.val[i].data.value.val + replies.val[i].data.value.len)});
    printf(">> [Reply handler] received (%.*s, %.*s)\n",
           (int)replies.val[i].data.key.len, replies.val[i].data.key.val,
           (int)replies.val[i].data.value.len, replies.val[i].data.value.val);
  }
  zn_reply_data_array_free(replies);
  return result;
}