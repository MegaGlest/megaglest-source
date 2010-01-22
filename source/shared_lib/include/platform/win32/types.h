// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_PLATFORM_TYPES_H_
#define _SHARED_PLATFORM_TYPES_H_

#include <windows.h>

namespace Shared{ namespace Platform{

typedef HWND WindowHandle;
typedef HDC DeviceContextHandle;
typedef HGLRC GlContextHandle;

typedef float float32;
typedef double float64;
typedef char int8;
typedef unsigned char uint8;
typedef short int int16;
typedef unsigned short int uint16;
typedef int int32;
typedef unsigned int uint32;
typedef long long int64;

}}//end namespace

#endif
