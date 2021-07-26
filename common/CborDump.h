#ifndef C4459101_F076_4820_A58D_E11FB9A9C0B8
#define C4459101_F076_4820_A58D_E11FB9A9C0B8
#include <cbor.h>
#include <util.h>
#include <Log.h>
#include <sstream>
using namespace std;


extern CborError dumpCborRecursive(stringstream& ss, CborValue *it,
                                          int nestingLevel);
string cborDump(const bytes &);

#endif /* C4459101_F076_4820_A58D_E11FB9A9C0B8 */
