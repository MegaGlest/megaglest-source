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
#include "window_gl.h"

#include "gl_wrap.h"
#include "graphics_interface.h"
#include "leak_dumper.h"

using namespace Shared::Graphics;

namespace Shared{ namespace Platform{

// =====================================================
//	class WindowGl
// =====================================================

void WindowGl::initGl(int colorBits, int depthBits, int stencilBits,bool hardware_acceleration, bool fullscreen_anti_aliasing){
	context.setColorBits(colorBits);
	context.setDepthBits(depthBits);
	context.setStencilBits(stencilBits);
	context.setHardware_acceleration(hardware_acceleration);
	context.setFullscreen_anti_aliasing(fullscreen_anti_aliasing);
	
	context.init();
}

void WindowGl::makeCurrentGl() {
	GraphicsInterface::getInstance().setCurrentContext(&context);
	context.makeCurrent();
}

void WindowGl::swapBuffersGl(){
	context.swapBuffers();
}

}}//end namespace
