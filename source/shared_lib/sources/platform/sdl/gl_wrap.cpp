//This file is part of Glest Shared Library (www.glest.org)
//Copyright (C) 2005 Matthias Braun <matze@braunis.de>

//You can redistribute this code and/or modify it under
//the terms of the GNU General Public License as published by the Free Software
//Foundation; either version 2 of the License, or (at your option) any later
//version.
#include "gl_wrap.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cassert>

#include "opengl.h"
#include "sdl_private.h"
#include "noimpl.h"
#include "util.h"
#include "window.h"
#include <vector>
//#include <SDL_image.h>
#include "leak_dumper.h"

using namespace Shared::Graphics::Gl;
using namespace Shared::Util;

namespace Shared{ namespace Platform{

// ======================================
//	class PlatformContextGl
// ======================================
PlatformContextGl::PlatformContextGl() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	icon = NULL;
	screen = NULL;
}

PlatformContextGl::~PlatformContextGl() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	end();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void PlatformContextGl::init(int colorBits, int depthBits, int stencilBits,bool hardware_acceleration, bool fullscreen_anti_aliasing) {
	
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	Window::setupGraphicsScreen(depthBits, stencilBits, hardware_acceleration, fullscreen_anti_aliasing);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	int flags = SDL_OPENGL;
	if(PlatformCommon::Private::shouldBeFullscreen) {
		flags |= SDL_FULLSCREEN;
		Window::setIsFullScreen(true);
	}
	else {
		Window::setIsFullScreen(false);
	}

	//flags |= SDL_HWSURFACE

	int resW = PlatformCommon::Private::ScreenWidth;
	int resH = PlatformCommon::Private::ScreenHeight;

#ifndef WIN32
	if(fileExists("megaglest.bmp")) {

		if(icon != NULL) {
			SDL_FreeSurface(icon);
			icon = NULL;
		}

		icon = SDL_LoadBMP("megaglest.bmp");
	//SDL_Surface *icon = IMG_Load("megaglest.ico");


//#if !defined(MACOSX)
	// Set Icon (must be done before any sdl_setvideomode call)
	// But don't set it on OS X, as we use a nicer external icon there.
//#if WORDS_BIGENDIAN
//	SDL_Surface* icon= SDL_CreateRGBSurfaceFrom((void*)logo,32,32,8,128,0xff000000,0x00ff0000,0x0000ff00,0);
//#else
//	SDL_Surface* icon= SDL_CreateRGBSurfaceFrom((void*)logo,32,32,32,128,0x000000ff,0x0000ff00,0x00ff0000,0xff000000);
//#endif

		//printf("In [%s::%s Line: %d] icon = %p\n",__FILE__,__FUNCTION__,__LINE__,icon);
		if(icon == NULL) {
			printf("Error: %s\n", SDL_GetError());
		}
		if(icon != NULL) {
			SDL_WM_SetIcon(icon, NULL);
		}
	}
#endif

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] about to set resolution: %d x %d, colorBits = %d.\n",__FILE__,__FUNCTION__,__LINE__,resW,resH,colorBits);

	if(screen != NULL) {
		SDL_FreeSurface(screen);
		screen = NULL;
	}

	screen = SDL_SetVideoMode(resW, resH, colorBits, flags);
	if(screen == 0) {
		std::ostringstream msg;
		msg << "Couldn't set video mode "
			<< resW << "x" << resH << " (" << colorBits
			<< "bpp " << stencilBits << " stencil "
			<< depthBits << " depth-buffer). SDL Error is: " << SDL_GetError();
		throw std::runtime_error(msg.str());
	}

	SDL_WM_GrabInput(SDL_GRAB_OFF);
}

void PlatformContextGl::end() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(icon != NULL) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		SDL_FreeSurface(icon);
		icon = NULL;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(screen != NULL) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		SDL_FreeSurface(screen);
		screen = NULL;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void PlatformContextGl::makeCurrent() {
}

void PlatformContextGl::swapBuffers() {
	SDL_GL_SwapBuffers();
}
	
	
const char *getPlatformExtensions(const PlatformContextGl *pcgl) {
		return "";
}
	
void *getGlProcAddress(const char *procName) {
		void* proc = SDL_GL_GetProcAddress(procName);
		assert(proc!=NULL);
		return proc;
}

}}//end namespace 
