#include <Frame.h>
#include <CborDump.h>
#include <iostream>
//================================================================

FrameExtractor::FrameExtractor() : Flow<bytes, bytes>() {}
void FrameExtractor::on(const bytes &bs) { handleRxd(bs); }
void FrameExtractor::toStdout(const bytes &bs) {
  if (bs.size()) {
    //  cout << hexDump(bs) << endl;
    fwrite("\033[32m", 5, 1, stdout);
    fwrite(bs.data(), bs.size(), 1, stdout);
    fwrite("\033[39m", 5, 1, stdout);
  }
}

bool FrameExtractor::handleFrame(bytes &bs) {
  if (bs.size() == 0) return false;
  if (ppp_deframe(_cleanData, bs)) {
    emit(_cleanData);
    return true;
  } else {
    toStdout(bs);
    return false;
  }
}

void FrameExtractor::handleRxd(const bytes &bs) {
  for (uint8_t b : bs) {
    if (b == PPP_FLAG_CHAR) {
      _lastFrameFlag = Sys::millis();
      handleFrame(_inputFrame);
      _inputFrame.clear();
    } else {
      _inputFrame.push_back(b);
    }
  }
  if ((Sys::millis() - _lastFrameFlag) > _frameTimeout) {
    //   cout << " skip  bytes " << hexDump(bs) << endl;
    //   cout << " frame data drop " << hexDump(frameData) << flush;
    toStdout(_inputFrame);
    _inputFrame.clear();
  }
}
void FrameExtractor::request(){};


FrameToBytes::FrameToBytes()
    : LambdaFlow<bytes, bytes>([&](bytes &out, const bytes &in) {
        out = ppp_frame(in);
        return true;
      }){};
