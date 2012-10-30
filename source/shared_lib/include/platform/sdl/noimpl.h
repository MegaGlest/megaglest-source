// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _NOIMPL_H_
#define _NOIMPL_H_

#ifndef WIN32

#define NOIMPL std::cerr << __PRETTY_FUNCTION__ << " not implemented.\n";

#else

#define NOIMPL std::cerr << __FUNCTION__ << " not implemented.\n";

#endif

#include "leak_dumper.h"

#endif

