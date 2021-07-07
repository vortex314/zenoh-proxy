#ifndef __FRAME_H_
#define __FRAME_H_
#include <memory.h>
#include <stdint.h>

#include <thread>
using namespace std;
#include <Log.h>
#include <vector>

#define PPP_FCS_SIZE 2
// PPP special characters
#define PPP_MASK_CHAR 0x20
#define PPP_ESC_CHAR 0x7D
#define PPP_FLAG_CHAR 0x7E

typedef unsigned char byte;
typedef std::vector<byte> bytes;

bool ppp_frame(bytes &out, const bytes &in);
bool ppp_deframe(bytes &out, const bytes &in);

#endif