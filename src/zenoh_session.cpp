
#include <zenoh_session.h>

using namespace zenoh;

void Session::dataHandler(const zn_sample_t *sample, const void *arg) {
  Session *session = (Session *)arg;

  string key(sample->key.val, sample->key.len);
  bytes data(sample->value.val, sample->value.val + sample->value.len);
  INFO(" RXD : %s : %s ", key.c_str(), hexDump(data).c_str());
  session->incoming.emit({key, data});
}

Session::Session(Thread &thr, Config &cfg) : Actor(thr) { _zenoh_session = 0; }

int Session::scout() {
  zn_properties_t *config = zn_config_default();
  z_string_t us_west = z_string_make(US_WEST);
  //  zn_properties_insert(config, ZN_CONFIG_PEER_KEY, us_west);
  zn_properties_insert(config, ZN_CONFIG_LISTENER_KEY, us_west);

  zn_hello_array_t hellos = zn_scout(ZN_ROUTER | ZN_PEER, config, 1000);
  if (hellos.len > 0) {
    for (unsigned int i = 0; i < hellos.len; ++i) {
      zn_hello_t hello = hellos.val[i];
      z_str_array_t locators = hello.locators;
      bytes pid;
      if (hello.pid.val != NULL) {
        pid = bytes(hello.pid.val, hello.pid.val + hello.pid.len);
      }
      INFO(" whatami: %s pid : %s ",
           hello.whatami == ZN_ROUTER
               ? "ZN_ROUTER"
               : hello.whatami == ZN_PEER ? "ZN_PEER" : "OTHER",
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

int Session::open(Properties &props) {
  zn_properties_t *config = zn_config_default();
  zn_properties_insert(config, ZN_CONFIG_PEER_KEY, z_string_make(US_WEST));

  if (_zenoh_session = 0)
    _zenoh_session = zn_open(config);
  if (_zenoh_session == 0)
    WARN(" zn_open() failed ");
  return _zenoh_session == 0 ? -1 : 0;
}

void Session::close() {
  for (zn_subscriber_t *sub : _subscribers) {
    zn_undeclare_subscriber(sub);
  }
  _subscribers.clear();
  if (_zenoh_session)
    zn_close(_zenoh_session);
  _zenoh_session = NULL;
}

int Session::subscribe(string resource) {
  zn_subscriber_t *sub =
      zn_declare_subscriber(_zenoh_session, zn_rname(resource.c_str()),
                            zn_subinfo_default(), dataHandler, this);
  if (sub == NULL) {
    WARN(" subscription failed for %s ", resource.c_str());
    return -1;
  }
  _subscribers.push_back(sub);
  return 0;
}

int Session::publish(string topic, bytes &bs) {
  return zn_write(_zenoh_session, zn_rname(topic.c_str()), bs.data(),
                  bs.size());
}

ResourceKey Session::resource(string topic) {
  struct zn_reskey_t reskey;
  reskey.id = 0;
  reskey.suffix = topic.c_str();
  unsigned long rc = zn_declare_resource(_zenoh_session, reskey);
  INFO(" %ld == %ld ", rc, reskey.id);
  return 0;
}