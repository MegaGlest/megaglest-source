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

#ifndef _SHARED_PLATFORM_WINDOWGL_H_
#define _SHARED_PLATFORM_WINDOWGL_H_

#include "context_gl.h"
#include "window.h"

using Shared::Graphics::Gl::ContextGl;

namespace Shared{ namespace Platform{

// =====================================================
//	class WindowGl
// =====================================================

class WindowGl: public Window{
protected:
	ContextGl context;

public:
	void initGl(int colorBits, int depthBits, int stencilBits);
	void makeCurrentGl();
	void swapBuffersGl();
};

}}//end namespace

#endif
