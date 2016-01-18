// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2005 Matthias Braun <matze@braunis.de>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _COMMON_SCOPED_PTR_H_
#define _COMMON_SCOPED_PTR_H_

#include <memory>


// =====================================================
//	class Thread
// =====================================================
using namespace std;

#if !defined(HAVE_CXX11) && !defined(__GXX_EXPERIMENTAL_CXX0X__)
  #define unique_ptr auto_ptr
#endif

#endif
