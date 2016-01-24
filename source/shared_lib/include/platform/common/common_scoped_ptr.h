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
//	Hack for compilers that dont support cxx11's unique_ptr which replaces auto_ptr
// =====================================================
//using namespace std;

// C++11
#if defined(HAVE_CXX11) || (__cplusplus >= 201103L) || (_MSC_VER >= 1900)

#if defined(WIN32)
#define auto_ptr unique_ptr
#else
	#define auto_ptr std::unique_ptr
#endif

#endif

#endif
