#ifndef _NOIMPL_H_
#define _NOIMPL_H_

#ifndef WIN32

#define NOIMPL std::cerr << __PRETTY_FUNCTION__ << " not implemented.\n";

#else

#define NOIMPL std::cerr << __FUNCTION__ << " not implemented.\n";

#endif

#include "leak_dumper.h"

#endif

