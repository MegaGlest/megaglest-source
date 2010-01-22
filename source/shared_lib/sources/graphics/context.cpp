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

#include "context.h"

#include "leak_dumper.h"

namespace Shared{ namespace Graphics{

// =====================================================
//	class Context
// =====================================================

Context::Context(){
	colorBits= 32;
	depthBits= 24;
	stencilBits= 0;
}

}}//end namespace
