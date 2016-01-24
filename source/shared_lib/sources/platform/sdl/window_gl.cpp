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

WindowGl::WindowGl() : Window() {
}
WindowGl::WindowGl(SDL_Window *sdlWindow) : Window(sdlWindow) {
}
WindowGl::~WindowGl() {
}

int WindowGl::getScreenWidth() {
	return PlatformCommon::Private::ScreenWidth;

}
int WindowGl::getScreenHeight() {
	return PlatformCommon::Private::ScreenHeight;
}

void WindowGl::setGamma(SDL_Window *window,float gammaValue) {
	//SDL_SetGamma(gammaValue, gammaValue, gammaValue);
	//SDL_SetWindowGammaRamp(getSDLWindow(), gammaValue, gammaValue, gammaValue);
	gammaValue = clamp(gammaValue, 0.1f, 10.0f);

	Uint16 red_ramp[256];
	Uint16 green_ramp[256];
	Uint16 blue_ramp[256];

	SDL_CalculateGammaRamp(gammaValue, red_ramp);
	SDL_memcpy(green_ramp, red_ramp, sizeof(red_ramp));
	SDL_memcpy(blue_ramp, red_ramp, sizeof(red_ramp));

	SDL_SetWindowGammaRamp(window, red_ramp, green_ramp, blue_ramp);
}
void WindowGl::setGamma(float gammaValue) {
	context.setGammaValue(gammaValue);
	WindowGl::setGamma(getSDLWindow(),gammaValue);
}

SDL_Window * WindowGl::getScreenWindow() {
	return context.getPlatformContextGlPtr()->getScreenWindow();
}
SDL_Surface * WindowGl::getScreenSurface() {
	return context.getPlatformContextGlPtr()->getScreenSurface();
}

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
	setSDLWindow(context.getPlatformContextGlPtr()->getScreenWindow());
}

void WindowGl::makeCurrentGl() {
	GraphicsInterface::getInstance().setCurrentContext(&context);
	context.makeCurrent();
}

void WindowGl::swapBuffersGl(){
	context.swapBuffers();
}

void WindowGl::eventToggleFullScreen(bool isFullscreen) {
	Window::eventToggleFullScreen(isFullscreen);

	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false) {
		//SDL_Surface *cur_surface = SDL_GetVideoSurface();
		if(getScreenWindow() != NULL) {
			if(getIsFullScreen()){
				SDL_SetWindowFullscreen(getScreenWindow(),SDL_WINDOW_FULLSCREEN_DESKTOP);
			}
			else {
				SDL_SetWindowFullscreen(getScreenWindow(),0);
			}
		}

		if(isFullscreen) {
			changeVideoModeFullScreen(isFullscreen);
			ChangeVideoMode(true, getScreenWidth(), getScreenHeight(),
					true,context.getColorBits(), context.getDepthBits(), context.getStencilBits(),
					context.getHardware_acceleration(),context.getFullscreen_anti_aliasing(),
					context.getGammaValue());

		}
		else {
			changeVideoModeFullScreen(false);
			ChangeVideoMode(true, getDesiredScreenWidth(), getDesiredScreenHeight(),
					false,context.getColorBits(), context.getDepthBits(), context.getStencilBits(),
					context.getHardware_acceleration(),context.getFullscreen_anti_aliasing(),
					context.getGammaValue());
		}
	}
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
		if (SDL_GetWindowWMInfo(Window::getSDLWindow(),&info) == -1) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s %d] SDL_GetWMInfo #1 failed\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			return false;
		}

		// get device context handle
		tempDC = GetDC( info.info.win.window );

		// create temporary context
		tempRC = wglCreateContext( tempDC );
		if (tempRC == NULL) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s %d] wglCreateContext failed\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			return false;
		}

		// share resources to temporary context
		SetLastError(0);
		//if (!wglShareLists(info.info.win.hglrc, tempRC)) {
		//	if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s %d] wglShareLists #1 failed\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		//	return false;
		//}
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
		if (SDL_GetWindowWMInfo(Window::getSDLWindow(),&info) == -1) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s %d] SDL_GetWMInfo #2 failed\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			return false;
		}

		// share resources to new SDL-created context
		//if (!wglShareLists(tempRC, info.hglrc)) {
		//	if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s %d] wglShareLists #2 failed\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		//	return false;
		//}

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
