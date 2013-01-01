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
#include "util.h"
#include "platform_util.h"
#include "sdl_private.h"
#include "opengl.h"

#ifdef WIN32
#include "SDL_syswm.h"
#endif
#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Shared{ namespace Platform{

// =====================================================
//	class WindowGl
// =====================================================

void WindowGl::initGl(int colorBits, int depthBits, int stencilBits,
		             bool hardware_acceleration, bool fullscreen_anti_aliasing,
		             float gammaValue) {
	context.setColorBits(colorBits);
	context.setDepthBits(depthBits);
	context.setStencilBits(stencilBits);
	context.setHardware_acceleration(hardware_acceleration);
	context.setFullscreen_anti_aliasing(fullscreen_anti_aliasing);
	context.setGammaValue(gammaValue);
	
	context.init();
}

void WindowGl::makeCurrentGl() {
	GraphicsInterface::getInstance().setCurrentContext(&context);
	context.makeCurrent();
}

void WindowGl::swapBuffersGl(){
	context.swapBuffers();
}

// changes display resolution at any time
bool WindowGl::ChangeVideoMode(bool preserveContext, int resWidth, int resHeight,
		bool fullscreenWindow,
		int colorBits, int depthBits, int stencilBits, bool hardware_acceleration,
		bool fullscreen_anti_aliasing, float gammaValue) {
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d] preserveContext = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,preserveContext);

#ifdef WIN32
	SDL_SysWMinfo info;
	HDC tempDC = 0;
	HGLRC tempRC = 0;

	if(preserveContext == true) {
		// get window handle from SDL
		SDL_VERSION(&info.version);
		if (SDL_GetWMInfo(&info) == -1) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s %d] SDL_GetWMInfo #1 failed\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			return false;
		}

		// get device context handle
		tempDC = GetDC( info.window );

		// create temporary context
		tempRC = wglCreateContext( tempDC );
		if (tempRC == NULL) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s %d] wglCreateContext failed\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			return false;
		}

		// share resources to temporary context
		SetLastError(0);
		if (!wglShareLists(info.hglrc, tempRC)) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s %d] wglShareLists #1 failed\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			return false;
		}
	}
#endif

 // set video mode
 //if (!SetVideoMode())
 //	 return false;
// this->initGl(config.getInt("ColorBits"),
//			       config.getInt("DepthBits"),
//			       config.getInt("StencilBits"),
//			       config.getBool("HardwareAcceleration","false"),
//			       config.getBool("FullScreenAntiAliasing","false"),
//			       config.getFloat("GammaValue","0.0"));


	this->setStyle(fullscreenWindow ? wsWindowedFixed: wsFullscreen);
	this->setPos(0, 0);
	this->setSize(resWidth, resHeight);

	this->initGl(colorBits, depthBits, stencilBits,hardware_acceleration,
				fullscreen_anti_aliasing,gammaValue);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	this->makeCurrentGl();

#ifdef WIN32
	if(preserveContext == true) {
		// previously used structure may possibly be invalid, to be sure we get it again
		SDL_VERSION(&info.version);
		if (SDL_GetWMInfo(&info) == -1) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s %d] SDL_GetWMInfo #2 failed\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			return false;
		}

		// share resources to new SDL-created context
		if (!wglShareLists(tempRC, info.hglrc)) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s %d] wglShareLists #2 failed\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			return false;
		}

		// we no longer need our temporary context
		if (!wglDeleteContext(tempRC)) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s %d] wglDeleteContext failed\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			return false;
		}
	}
#endif

 // success
 return true;
}


}}//end namespace
