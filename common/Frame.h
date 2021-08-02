#ifndef A4D87CFA_B381_4A7A_9FB2_67EDFB2285CB
#define A4D87CFA_B381_4A7A_9FB2_67EDFB2285CB
#include <limero.h>
#include <util.h>
#include <ppp_frame.h>
//================================================================
class FrameExtractor : public Flow<bytes, bytes> {
  bytes _inputFrame;
  bytes _cleanData;
  uint64_t _lastFrameFlag;
  uint32_t _frameTimeout = 1000;

 public:
  FrameExtractor();
  void on(const bytes &bs);
  void toStdout(const bytes &bs);
  bool handleFrame(bytes &bs);
  void handleRxd(const bytes &bs);
  void request();
};
class FrameToBytes : public LambdaFlow<bytes, bytes> {
 public:
  FrameToBytes();
};

#endif /* A4D87CFA_B381_4A7A_9FB2_67EDFB2285CB */
