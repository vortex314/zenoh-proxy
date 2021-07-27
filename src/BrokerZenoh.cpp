
#include <BrokerZenoh.h>

#include <CborDump.h>

void BrokerZenoh::subscribeHandler(const zn_sample_t *sample, const void *arg) {
  Sub *pSub = (Sub *)arg;
  string key(sample->key.val, sample->key.len);
  bytes data(sample->value.val, sample->value.val + sample->value.len);
  pSub->callback(pSub->id, data);
}

BrokerZenoh::BrokerZenoh(Thread &thr, Config &cfg) { _zenoh_session = 0; }

int BrokerZenoh::scout() {
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

int BrokerZenoh::init() { return 0; }

int BrokerZenoh::connect(string clientId) {
  zn_properties_t *config = zn_config_default();
  //   zn_properties_insert(config, ZN_CONFIG_LISTENER_KEY,
  //   z_string_make(US_WEST));

  if (_zenoh_session == 0)  // idempotent
    _zenoh_session = zn_open(config);
  INFO("zn_open() %s.", _zenoh_session == 0 ? "failed" : "succeeded");
  return _zenoh_session == 0 ? -1 : 0;
}

int BrokerZenoh::disconnect() {
  INFO(" disconnecting.");
  for (auto tuple : _subscribers) {
    zn_undeclare_subscriber(tuple.second->zn_subscriber);
  }
  _subscribers.clear();
  for (auto tuple : _publishers) {
    zn_undeclare_publisher(tuple.second->zn_publisher);
  }
  _publishers.clear();

  return 0;
  //  if (_zenoh_session)
  //   zn_close(_zenoh_session);
  // _zenoh_session = NULL;
}

int BrokerZenoh::subscriber(int id, string pattern,
                            std::function<void(int, const bytes &)> callback) {
  INFO(" Zenoh subscriber %d : %s ", id, pattern.c_str());
  if (_subscribers.find(id) == _subscribers.end()) {
    Sub *sub = new Sub({id, pattern, callback, 0});
    zn_subscriber_t *zsub =
        zn_declare_subscriber(_zenoh_session, zn_rname(pattern.c_str()),
                              zn_subinfo_default(), subscribeHandler, sub);
    sub->zn_subscriber = zsub;
    if (zsub == NULL) {
      WARN(" subscription failed for %s ", pattern.c_str());
      return -1;
    }
    _subscribers.emplace(id, sub);
  }
  return 0;
}

int BrokerZenoh::unSubscribe(int id) {
  auto it = _subscribers.find(id);
  if (it == _subscribers.end()) {
  } else {
    zn_undeclare_subscriber(it->second->zn_subscriber);
  }
  return 0;
}

int BrokerZenoh::publisher(int id, string key) {
  if (_publishers.find(id) == _publishers.end()) {
    INFO(" Adding Zenoh publisher %d : '%s' in %d publishers", id, key.c_str(),_publishers.size());
    zn_reskey_t reskey = resource(key);
    zn_publisher_t *pub = zn_declare_publisher(_zenoh_session, reskey);
    if (pub == 0) WARN(" unable to declare publisher '%s'", key.c_str());
    Pub *pPub = new Pub{id, key, reskey, pub};
    _publishers.emplace(id, pPub);
  }
  return 0;
}

int BrokerZenoh::publish(int id, bytes &bs) {
  auto it = _publishers.find(id);
  if (it != _publishers.end()) {
    INFO("publish %d : %s => %s ", id, it->second->key.c_str(), cborDump(bs).c_str());
    int rc =
        zn_write(_zenoh_session, it->second->zn_reskey, bs.data(), bs.size());
    if (rc) WARN("zn_write failed.");
    return rc;
  } else {
    INFO(" publish id %d unknown in %d publishers. ", id,_publishers.size());
    return ENOENT;
  }
}

zn_reskey_t BrokerZenoh::resource(string topic) {
  zn_reskey_t rid =
      zn_rid(zn_declare_resource(_zenoh_session, zn_rname(topic.c_str())));
  return rid;
}
/*
vector<PubMsg> BrokerZenoh::query(string uri) {
  vector<PubMsg> result;
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
*/