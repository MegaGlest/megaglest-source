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

#ifndef _SHARED_PLATFORM_WINDOWGL_H_
#define _SHARED_PLATFORM_WINDOWGL_H_

#include "context_gl.h"
#include "window.h"
#include "leak_dumper.h"

using Shared::Graphics::Gl::ContextGl;

namespace Shared{ namespace Platform{

// =====================================================
//	class WindowGl
// =====================================================

class WindowGl: public Window{
private:
	ContextGl context;

public:
	void initGl(int colorBits, int depthBits, int stencilBits,
			    bool hardware_acceleration, bool fullscreen_anti_aliasing,
			    float gammaValue);
	void makeCurrentGl();
	void swapBuffersGl();
};

}}//end namespace

#endif
