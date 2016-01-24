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
#include "model.h"
#include "texture.h"
#include "graphics_interface.h"
#include "graphics_factory.h"
#include "platform_common.h"
#include "leak_dumper.h"

using namespace Shared::Graphics::Gl;
using namespace Shared::Util;
using namespace Shared::Graphics;
using namespace Shared::PlatformCommon;

namespace Shared{ namespace Platform{

// Example values:
// DEFAULT_CHARSET (English) = 1
// GB2312_CHARSET (Chinese)  = 134
#ifdef WIN32
DWORD PlatformContextGl::charSet = DEFAULT_CHARSET;
#else
int PlatformContextGl::charSet = 1;
#endif

// ======================================
//	class PlatformContextGl
// ======================================
PlatformContextGl::PlatformContextGl() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	glcontext = NULL;
	icon = NULL;
	window = NULL;
}

PlatformContextGl::~PlatformContextGl() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	end();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

SDL_Surface * PlatformContextGl::getScreenSurface() {
	return SDL_GetWindowSurface(window);
}

void PlatformContextGl::init(int colorBits, int depthBits, int stencilBits,
		                     bool hardware_acceleration,
		                     bool fullscreen_anti_aliasing, float gammaValue) {
	
    if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	Window::setupGraphicsScreen(depthBits, stencilBits, hardware_acceleration, fullscreen_anti_aliasing);

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	//printf("In [%s::%s %d] PlatformCommon::Private::shouldBeFullscreen = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,PlatformCommon::Private::shouldBeFullscreen);

	// SDL_WINDOW_FULLSCREEN seems very broken when changing resolutions that differ from the desktop resolution
	// For now fullscreen will mean use desktop resolution
	int flags = SDL_WINDOW_OPENGL;
	if(PlatformCommon::Private::shouldBeFullscreen) {
		Window::setIsFullScreen(true);
		//flags |= SDL_WINDOW_FULLSCREEN;
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	}
	else {
		Window::setIsFullScreen(false);
	}

	//flags |= SDL_HWSURFACE

	int resW = PlatformCommon::Private::ScreenWidth;
	int resH = PlatformCommon::Private::ScreenHeight;

	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false) {

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] about to set resolution: %d x %d, colorBits = %d.\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,resW,resH,colorBits);

		if(Window::getIsFullScreen() && (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) == SDL_WINDOW_FULLSCREEN_DESKTOP) {
			//printf("#1 SDL_WINDOW_FULLSCREEN_DESKTOP\n");
			// TODO: which display? is 0 the designated primary display always?
			SDL_Rect display_rect;
			SDL_GetDisplayBounds(0, &display_rect);

			if(PlatformCommon::Private::ScreenWidth != display_rect.w ||
			   PlatformCommon::Private::ScreenHeight != display_rect.h) {
				printf("Auto Change resolution to (%d x %d) from (%d x %d)\n",display_rect.w,display_rect.h,resW,resH);
				resW = display_rect.w;
				resH = display_rect.h;
				PlatformCommon::Private::ScreenWidth = display_rect.w;
				PlatformCommon::Private::ScreenHeight = display_rect.h;
			}
		}

		int windowX = SDL_WINDOWPOS_UNDEFINED;
		int windowY = SDL_WINDOWPOS_UNDEFINED;
		string windowTitleText = "MG";
		int windowDisplayID = -1;

		if(window != NULL) {
			SDL_GetWindowPosition(window,&windowX,&windowY);
			windowDisplayID = SDL_GetWindowDisplayIndex( window );
			//printf("windowDisplayID = %d\n",windowDisplayID);
			if(Window::getIsFullScreen()) {
				windowX = SDL_WINDOWPOS_CENTERED_DISPLAY(windowDisplayID);
				windowY = SDL_WINDOWPOS_CENTERED_DISPLAY(windowDisplayID);
			}
			windowTitleText = SDL_GetWindowTitle(window);

			SDL_DestroyWindow(window);
			window = NULL;
		}

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
		//screen = SDL_CreateWindow(resW, resH, colorBits, flags);
		window = SDL_CreateWindow(windowTitleText.c_str(),windowX,windowY,resW, resH, flags);
		if(window == 0) {
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

			std::ostringstream msg;
			msg << "Couldn't set video mode "
				<< resW << "x" << resH << " (" << colorBits
				<< "bpp " << stencilBits << " stencil "
				<< depthBits << " depth-buffer). SDL Error is: " << SDL_GetError();

			if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,msg.str().c_str());
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] error [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,msg.str().c_str());

			//screen = SDL_SetVideoMode(resW, resH, i, flags);
			window = SDL_CreateWindow(windowTitleText.c_str(),windowX,windowY,resW, resH, flags);
			if(window == 0) {
				// try to switch to native desktop resolution
				window = SDL_CreateWindow(windowTitleText.c_str(),windowX,windowY,0, 0,SDL_WINDOW_FULLSCREEN_DESKTOP|flags);
			}
			if(window == 0) {
				// try to revert to 640x480
				window = SDL_CreateWindow(windowTitleText.c_str(),windowX,windowY,650, 480,SDL_WINDOW_FULLSCREEN_DESKTOP);
			}
			if(window == 0) {
				throw std::runtime_error(msg.str());
			}
		}

//		int totalDisplays = SDL_GetNumVideoDisplays();
//		windowDisplayID = SDL_GetWindowDisplayIndex( window );
//		printf("!!! totalDisplays = %d, windowDisplayID = %d\n",totalDisplays,windowDisplayID);

		//SDL_SetWindowDisplayMode(window, NULL);

		bool isNewWindow = (glcontext == NULL);
		if(isNewWindow) {
			glcontext = SDL_GL_CreateContext(window);
		}
		else {
			SDL_GL_MakeCurrent(window, glcontext);
		}

		int h;
		int w;
		SDL_GetWindowSize(window, &w, &h);

		if((w != resW || h != resH) && (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) == SDL_WINDOW_FULLSCREEN_DESKTOP) {
			//printf("#2 SDL_WINDOW_FULLSCREEN_DESKTOP\n");
			printf("#1 Change resolution mismatch get (%d x %d) desired (%d x %d)\n",w,h,resW,resH);

			PlatformCommon::Private::ScreenWidth = w;
			PlatformCommon::Private::ScreenHeight = h;

			resW = PlatformCommon::Private::ScreenWidth;
			resH = PlatformCommon::Private::ScreenHeight;

			printf("#2 Change resolution to (%d x %d)\n",resW,resH);

//			SDL_SetWindowFullscreen(window,0);
//			SDL_SetWindowSize(window, resW, resH);
//			SDL_SetWindowFullscreen(window,flags);
//			SDL_SetWindowSize(window, resW, resH);
//
//			SDL_GetWindowSize(window, &w, &h);
//			printf("#2 Change resolution mismatch get (%d x %d) desired (%d x %d)\n",w,resW,h,resH);
		}
		glViewport( 0, 0, w, h ) ;

		// There seems to be a bug where if relative mouse mouse is enabled when you create a new window,
		// the window still reports that it has input & mouse focus, but it doesn't send any events for
		// mouse motion or mouse button clicks. You can fix this by toggling relative mouse mode.
		if (SDL_GetRelativeMouseMode()) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			SDL_SetRelativeMouseMode(SDL_TRUE);
		}

#ifndef WIN32
		string mg_icon_file = "";
#if defined(CUSTOM_DATA_INSTALL_PATH_VALUE)
		if(fileExists(formatPath(TOSTRING(CUSTOM_DATA_INSTALL_PATH_VALUE)) + "megaglest.png")) {
			mg_icon_file = formatPath(TOSTRING(CUSTOM_DATA_INSTALL_PATH_VALUE)) + "megaglest.png";
		}
		else if(fileExists(formatPath(TOSTRING(CUSTOM_DATA_INSTALL_PATH_VALUE)) + "megaglest.bmp")) {
			mg_icon_file = formatPath(TOSTRING(CUSTOM_DATA_INSTALL_PATH_VALUE)) + "megaglest.bmp";
		}

#endif

		if(mg_icon_file == "" && fileExists("megaglest.png")) {
			mg_icon_file = "megaglest.png";
		}
		else if(mg_icon_file == "" && fileExists("megaglest.bmp")) {
			mg_icon_file = "megaglest.bmp";
		}
		else if(mg_icon_file == "" && fileExists("/usr/share/pixmaps/megaglest.png")) {
			mg_icon_file = "/usr/share/pixmaps/megaglest.png";
		}
		else if(mg_icon_file == "" && fileExists("/usr/share/pixmaps/megaglest.bmp")) {
			mg_icon_file = "/usr/share/pixmaps/megaglest.bmp";
		}

		if(mg_icon_file != "") {

			if(icon != NULL) {
				SDL_FreeSurface(icon);
				icon = NULL;
			}

			//printf("Loading icon [%s]\n",mg_icon_file.c_str());
			if(extractExtension(mg_icon_file) == "bmp") {
				icon = SDL_LoadBMP(mg_icon_file.c_str());
			}
			else {
				//printf("Loadng png icon\n");
				Texture2D *texture2D = GraphicsInterface::getInstance().getFactory()->newTexture2D();
				texture2D->load(mg_icon_file);
				std::pair<SDL_Surface*,unsigned char*> result = texture2D->CreateSDLSurface(true);
				icon = result.first;
				delete texture2D;
				delete [] result.second;
			}

		//SDL_Surface *icon = IMG_Load("megaglest.ico");


	//#if !defined(MACOSX)
		// Set Icon (must be done before any sdl_setvideomode call)
		// But don't set it on OS X, as we use a nicer external icon there.
	//#if WORDS_BIGENDIAN
	//	SDL_Surface* icon= SDL_CreateRGBSurfaceFrom((void*)logo,32,32,8,128,0xff000000,0x00ff0000,0x0000ff00,0);
	//#else
	//	SDL_Surface* icon= SDL_CreateRGBSurfaceFrom((void*)logo,32,32,32,128,0x000000ff,0x0000ff00,0x00ff0000,0xff000000);
	//#endif

			//printf("In [%s::%s Line: %d] icon = %p\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,icon);
			if(icon == NULL) {
				printf("Icon Load Error #1: %s\n", SDL_GetError());
			}
			if(icon != NULL) {

				//uint32 colorkey = SDL_MapRGB(icon->format, 255, 0, 255);
				//SDL_SetColorKey(icon, SDL_SRCCOLORKEY, colorkey);
				if(SDL_SetColorKey(icon, SDL_TRUE, SDL_MapRGB(icon->format, 255, 0, 255))) {
					printf("Icon Load Error #2: %s\n", SDL_GetError());
				}
				else {
					SDL_SetWindowIcon(window,icon);
				}
			}
		}
#endif

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		//SDL_WM_GrabInput(SDL_GRAB_OFF);
		//SDL_SetRelativeMouseMode(SDL_FALSE);

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d] BEFORE glewInit call\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		
		GLuint err = glewInit();

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d] AFTER glewInit call err = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,err);

		if (GLEW_OK != err) {
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

			fprintf(stderr, "Error [main]: glewInit failed: %s\n", glewGetErrorString(err));
			//return 1;
			throw std::runtime_error((char *)glewGetErrorString(err));
		}
		//fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		int bufferSize = (resW * resH * BaseColorPickEntity::COLOR_COMPONENTS);

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		BaseColorPickEntity::init(bufferSize);

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		if(gammaValue != 0.0) {
			//printf("Attempting to call SDL_SetGamma using value %f\n", gammaValue);
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			if (SDL_SetWindowBrightness(window, gammaValue) < 0) {
				printf("WARNING, SDL_SetWindowBrightness failed using value %f [%s]\n", gammaValue,SDL_GetError());
			}
		}

//        SDL_WM_GrabInput(SDL_GRAB_ON);
//        SDL_WM_GrabInput(SDL_GRAB_OFF);
		SDL_SetRelativeMouseMode(SDL_TRUE);
		SDL_SetRelativeMouseMode(SDL_FALSE);


//		if(Window::getIsFullScreen())
//			SDL_SetWindowGrab(window, SDL_TRUE);
//		else
		SDL_SetWindowGrab(window, SDL_FALSE);

//		if(Window::getIsFullScreen()) {
//			SDL_SetWindowSize(window, resW, resH);
//			//SDL_SetWindowFullscreen(window,SDL_WINDOW_FULLSCREEN);
//			SDL_SetWindowSize(window, resW, resH);
//		}

//		SDL_SetRelativeMouseMode(SDL_TRUE);
//		SDL_SetRelativeMouseMode(SDL_FALSE);
	}
}

void PlatformContextGl::end() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	if(icon != NULL) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		SDL_FreeSurface(icon);
		icon = NULL;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	if(window != NULL) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		SDL_DestroyWindow(window);
		window = NULL;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void PlatformContextGl::makeCurrent() {
}

void PlatformContextGl::swapBuffers() {
	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false) {
		 SDL_GL_SwapWindow(window);
	}
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
