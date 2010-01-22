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

#include "context_gl.h"

#include <cassert>
#include <stdexcept>

#include "opengl.h"
#include "leak_dumper.h"

using namespace std;

namespace Shared{ namespace Graphics{ namespace Gl{

// =====================================================
//	class ContextGl
// =====================================================

void ContextGl::init(){
	pcgl.init(colorBits, depthBits, stencilBits);
}

void ContextGl::end(){
	pcgl.end();
}

void ContextGl::makeCurrent(){
	pcgl.makeCurrent();
}

void ContextGl::swapBuffers(){
	pcgl.swapBuffers();
}

}}}//end namespace
