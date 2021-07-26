#ifndef BEA3A05D_E4F4_41BB_A864_310EE1D37C62
#define BEA3A05D_E4F4_41BB_A864_310EE1D37C62

#include <limero.h>
#include <util.h>
typedef int (*SubscribeCallback)(int, bytes);

class BrokerAbstract {
 public:
  ValueSource<bool> connected;
  virtual int init() = 0;
  virtual int connect(string) = 0;
  virtual int disconnect() = 0;
  virtual int publisher(int, string) = 0;
  virtual int subscriber(int, string,
                         std::function<void(int, const bytes &)>) = 0;
  virtual int publish(int, bytes &) = 0;
  virtual int unSubscribe(int) = 0;
};

#endif /* BEA3A05D_E4F4_41BB_A864_310EE1D37C62 */
