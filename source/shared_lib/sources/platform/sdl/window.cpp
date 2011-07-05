//This file is part of Glest Shared Library (www.glest.org)
//Copyright (C) 2005 Matthias Braun <matze@braunis.de>

//You can redistribute this code and/or modify it under
//the terms of the GNU General Public License as published by the Free Software
//Foundation; either version 2 of the License, or (at your option) any later
//version.

#include "window.h"

#include <iostream>
#include <stdexcept>
#include <cassert>
#include <cctype>

#include "conversion.h"
#include "platform_util.h"
#include "sdl_private.h"
#include "noimpl.h"
#include "util.h"
#include "opengl.h"

#ifdef WIN32

#include "SDL_syswm.h"

#endif

#include "leak_dumper.h"

using namespace Shared::Util;
using namespace std;

namespace Shared{ namespace Platform{

// =======================================
//               WINDOW
// =======================================

// ========== STATIC INICIALIZATIONS ==========

// Matze: hack for now...
static Window* global_window = 0;
static int oldX=0,oldY=0;
int64 Window::lastMouseEvent = 0;	/** for use in mouse hover calculations */
Vec2i Window::mousePos;
MouseState Window::mouseState;
bool Window::isKeyPressedDown = false;
bool Window::isFullScreen = false;
SDL_keysym Window::keystate;

bool Window::isActive = false;
bool Window::no2DMouseRendering = false;
#ifdef WIN32
bool Window::allowAltEnterFullscreenToggle = false;
#else
bool Window::allowAltEnterFullscreenToggle = true;
#endif
int Window::lastShowMouseState = 0;

bool Window::tryVSynch = false;

// ========== PUBLIC ==========

Window::Window()  {
	for(int idx = 0; idx < mbCount; idx++) {
		lastMouseDown[idx] = 0;
	}

	assert(global_window == 0);
	global_window = this;
	Window::isActive = true;

	lastMouseEvent = 0;
	mousePos = Vec2i(0);
	mouseState.clear();

#ifdef WIN32
	init_win32();
#endif
}

Window::~Window() {
#ifdef WIN32
	done_win32();
#endif

	assert(global_window == this);
	global_window = 0;
}

bool Window::handleEvent() {
	string codeLocation = "a";

	SDL_Event event;
	SDL_GetMouseState(&oldX,&oldY);

	codeLocation = "b";

	while(SDL_PollEvent(&event)) {
		try {
			codeLocation = "c";

			switch(event.type) {
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
				case SDL_MOUSEMOTION:

					//printf("In [%s::%s] Line :%d\n",__FILE__,__FUNCTION__,__LINE__);
					codeLocation = "d";

           			setLastMouseEvent(Chrono::getCurMillis());
           			setMousePos(Vec2i(event.button.x, event.button.y));
					break;
			}

			codeLocation = "d";

			switch(event.type) {
				case SDL_QUIT:
					//printf("In [%s::%s] Line :%d\n",__FILE__,__FUNCTION__,__LINE__);
					codeLocation = "e";
					return false;
				case SDL_MOUSEBUTTONDOWN:
					//printf("In [%s::%s] Line :%d\n",__FILE__,__FUNCTION__,__LINE__);
					codeLocation = "f";

					if(global_window) {
						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
						global_window->handleMouseDown(event);
						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
					}
					break;
				case SDL_MOUSEBUTTONUP: {
					//printf("In [%s::%s] Line :%d\n",__FILE__,__FUNCTION__,__LINE__);
					codeLocation = "g";
					if(global_window) {
						MouseButton b = getMouseButton(event.button.button);
						setMouseState(b, false);

						global_window->eventMouseUp(event.button.x,
							event.button.y,getMouseButton(event.button.button));
					}
					break;
				}
				case SDL_MOUSEMOTION: {
					//printf("In [%s::%s] Line :%d\n",__FILE__,__FUNCTION__,__LINE__);
					//MouseState ms;
					//ms.leftMouse = (event.motion.state & SDL_BUTTON_LMASK) != 0;
					//ms.rightMouse = (event.motion.state & SDL_BUTTON_RMASK) != 0;
					//ms.centerMouse = (event.motion.state & SDL_BUTTON_MMASK) != 0;
					codeLocation = "h";

					setMouseState(mbLeft, (event.motion.state & SDL_BUTTON_LMASK) == SDL_BUTTON_LMASK);
					setMouseState(mbRight, (event.motion.state & SDL_BUTTON_RMASK) == SDL_BUTTON_RMASK);
					setMouseState(mbCenter, (event.motion.state & SDL_BUTTON_MMASK) == SDL_BUTTON_MMASK);

					if(global_window) {
						global_window->eventMouseMove(event.motion.x, event.motion.y, &getMouseState()); //&ms);
					}
					break;
				}
				case SDL_KEYDOWN:
					//printf("In [%s::%s] Line :%d\n",__FILE__,__FUNCTION__,__LINE__);

					{

					if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] =================================== START OF SDL SDL_KEYDOWN ================================\n",__FILE__,__FUNCTION__,__LINE__);

					codeLocation = "i";
					Window::isKeyPressedDown = true;
					keystate = event.key.keysym;

					string keyName = SDL_GetKeyName(event.key.keysym.sym);
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] Raw SDL key [%d - %c] mod [%d] unicode [%d - %c] scancode [%d] keyName [%s]\n",__FILE__,__FUNCTION__,__LINE__,event.key.keysym.sym,event.key.keysym.sym,event.key.keysym.mod,event.key.keysym.unicode,event.key.keysym.unicode,event.key.keysym.scancode,keyName.c_str());
					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Raw SDL key [%d] mod [%d] unicode [%d] scancode [%d] keyName [%s]\n",__FILE__,__FUNCTION__,__LINE__,event.key.keysym.sym,event.key.keysym.mod,event.key.keysym.unicode,event.key.keysym.scancode,keyName.c_str());

					/* handle ALT+Return */
					if((keyName == "return" || keyName == "enter")
							&& (event.key.keysym.mod & (KMOD_LALT | KMOD_RALT))) {
						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d] SDLK_RETURN pressed.\n",__FILE__,__FUNCTION__,__LINE__);
						toggleFullscreen();
					}
#ifdef WIN32
					/* handle ALT+f4 */
					if((keyName == "f4" || keyName == "F4")
							&& (event.key.keysym.mod & (KMOD_LALT | KMOD_RALT))) {
						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d] ALT-F4 pressed.\n",__FILE__,__FUNCTION__,__LINE__);
						return false;
					}
#endif
					if(global_window) {
						//char key = getKey(event.key.keysym,true);
						//key = tolower(key);
						//if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("******************* key [%d]\n",key);

						global_window->eventKeyDown(event.key);
						global_window->eventKeyPress(event.key);

						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
					}

					if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] =================================== END OF SDL SDL_KEYDOWN ================================\n",__FILE__,__FUNCTION__,__LINE__);
					}
					break;

				case SDL_KEYUP:
					//printf("In [%s::%s] Line :%d\n",__FILE__,__FUNCTION__,__LINE__);

					if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] =================================== START OF SDL SDL_KEYUP ================================\n",__FILE__,__FUNCTION__,__LINE__);

					codeLocation = "j";

					Window::isKeyPressedDown = false;
					keystate = event.key.keysym;

					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] KEY_UP, Raw SDL key [%d] mod [%d] unicode [%d] scancode [%d]\n",__FILE__,__FUNCTION__,__LINE__,event.key.keysym.sym,event.key.keysym.mod,event.key.keysym.unicode,event.key.keysym.scancode);

					if(global_window) {
						//char key = getKey(event.key.keysym,true);
						//key = tolower(key);
						global_window->eventKeyUp(event.key);
					}

					if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] =================================== END OF SDL SDL_KEYUP ================================\n",__FILE__,__FUNCTION__,__LINE__);

					break;
				case SDL_ACTIVEEVENT:
				{
					codeLocation = "k";
					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] SDL_ACTIVEEVENT event.active.state = %d event.active. = %d\n",__FILE__,__FUNCTION__,__LINE__,event.active.state,event.active.gain);

					// Check if the program has lost window focus
					if ((event.active.state & SDL_APPACTIVE) == SDL_APPACTIVE) {
						if (event.active.gain == 0) {
							Window::isActive = false;
						}
						else {
							Window::isActive = true;
						}

						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Window::isActive = %d\n",__FILE__,__FUNCTION__,__LINE__,Window::isActive);

						bool willShowCursor = (!Window::isActive || (Window::lastShowMouseState == SDL_ENABLE) || Window::getUseDefaultCursorOnly());
						showCursor(willShowCursor);
					}
					// Check if the program has lost window focus
					if ((event.active.state & SDL_APPMOUSEFOCUS) != SDL_APPMOUSEFOCUS &&
						(event.active.state & SDL_APPINPUTFOCUS) != SDL_APPINPUTFOCUS &&
						(event.active.state & SDL_APPACTIVE) != SDL_APPACTIVE) {
						if (event.active.gain == 0) {
							Window::isActive = false;
						}
						//else if (event.active.gain == 1) {
						else {
							Window::isActive = true;
						}

						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Window::isActive = %d, event.active.state = %d\n",__FILE__,__FUNCTION__,__LINE__,Window::isActive,event.active.state);
						bool willShowCursor = (!Window::isActive || (Window::lastShowMouseState == SDL_ENABLE) || Window::getUseDefaultCursorOnly());
						showCursor(willShowCursor);
					}
				}
				break;
			}
		}
		catch(const char *e){
			SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] (a1) Couldn't process event: [%s] codeLocation = %s\n",__FILE__,__FUNCTION__,__LINE__,e,codeLocation.c_str());
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] (a1) Couldn't process event: [%s] codeLocation = %s\n",__FILE__,__FUNCTION__,__LINE__,e,codeLocation.c_str());
			throw runtime_error(e);
		}
		catch(const std::runtime_error& e) {
			SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] (a2) Couldn't process event: [%s] codeLocation = %s\n",__FILE__,__FUNCTION__,__LINE__,e.what(),codeLocation.c_str());
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] (a2) Couldn't process event: [%s] codeLocation = %s\n",__FILE__,__FUNCTION__,__LINE__,e.what(),codeLocation.c_str());
			throw runtime_error(e.what());
		}
		catch(const std::exception& e) {
			SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] (b) Couldn't process event: [%s] codeLocation = %s\n",__FILE__,__FUNCTION__,__LINE__,e.what(),codeLocation.c_str());
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] (b) Couldn't process event: [%s] codeLocation = %s\n",__FILE__,__FUNCTION__,__LINE__,e.what(),codeLocation.c_str());
		}
		catch(...) {
			SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] (c) Couldn't process event: [UNKNOWN ERROR] codeLocation = %s\n",__FILE__,__FUNCTION__,__LINE__,codeLocation.c_str());
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] (c) Couldn't process event: [UNKNOWN ERROR] codeLocation = %s\n",__FILE__,__FUNCTION__,__LINE__,codeLocation.c_str());
		}
	}

	return true;
}

void Window::revertMousePos() {
	SDL_WarpMouse(oldX, oldY);
}

string Window::getText() {
	char* c = 0;
	SDL_WM_GetCaption(&c, 0);

	return string(c);
}

float Window::getAspect() {
	return static_cast<float>(getClientH())/getClientW();
}

void Window::setText(string text) {
	SDL_WM_SetCaption(text.c_str(), 0);
}

void Window::setSize(int w, int h) {
	this->w = w;
	this->h = h;
	Private::ScreenWidth = w;
	Private::ScreenHeight = h;
}

void Window::setPos(int x, int y)  {
	if(x != 0 || y != 0) {
		NOIMPL;
		return;
	}
}

void Window::minimize() {
	NOIMPL;
}

void Window::setEnabled(bool enabled) {
	NOIMPL;
}

void Window::setVisible(bool visible) {
	 NOIMPL;
}

void Window::setStyle(WindowStyle windowStyle) {
	if(windowStyle == wsFullscreen)
		return;
	// NOIMPL;
}

void Window::create() {
	// nothing here
}

void Window::destroy() {
	SDL_Event event;
	event.type = SDL_QUIT;
	SDL_PushEvent(&event);
}

void Window::setupGraphicsScreen(int depthBits, int stencilBits, bool hardware_acceleration, bool fullscreen_anti_aliasing) {
	static int newDepthBits   = depthBits;
	static int newStencilBits = stencilBits;
	if(depthBits >= 0)
		newDepthBits   = depthBits;
	if(stencilBits >= 0)
		newStencilBits = stencilBits;

	if(fullscreen_anti_aliasing == true) {
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS,1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2);
	}
	if(hardware_acceleration == true) {
		SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	}
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 1);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 1);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 1);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, newStencilBits);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, newDepthBits);

	const SDL_VideoInfo *info = SDL_GetVideoInfo();
#ifdef SDL_GL_SWAP_CONTROL
    if(Window::tryVSynch == true) {
    	/* we want vsync for smooth scrolling */
    	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);
    }
#endif

	// setup LOD bias factor
	//const float lodBias = std::max(std::min( configHandler->Get("TextureLODBias", 0.0f) , 4.0f), -4.0f);
	const float lodBias = max(min(0.0f,4.0f),-4.0f);
	//if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("\n\n\n\n\n$$$$ In [%s::%s Line: %d] lodBias = %f\n\n",__FILE__,__FUNCTION__,__LINE__,lodBias);
#ifdef USE_STREFLOP
	if (streflop::fabs(lodBias) > 0.01f) {
#else
	if (fabs(lodBias) > 0.01f) {
#endif
		glTexEnvf(GL_TEXTURE_FILTER_CONTROL,GL_TEXTURE_LOD_BIAS, lodBias );
	}
}

#ifdef WIN32

static HWND GetSDLWindow()
{
    SDL_SysWMinfo   info;

    SDL_VERSION(&info.version);
    if (SDL_GetWMInfo(&info) == -1)
        return NULL;
    return info.window;
}

#endif

void Window::toggleFullscreen() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	Window::isFullScreen = !Window::isFullScreen;
#ifdef WIN32
	/* -- Portable Fullscreen Toggling --
	As of SDL 1.2.10, if width and height are both 0, SDL_SetVideoMode will use the
	width and height of the current video mode (or the desktop mode, if no mode has been set).
	Use 0 for Height, Width, and Color Depth to keep the current values. */

	if(Window::allowAltEnterFullscreenToggle == true) {
		SDL_Surface *cur_surface = SDL_GetVideoSurface();
		if(cur_surface != NULL) {
			Window::isFullScreen = !((cur_surface->flags & SDL_FULLSCREEN) == SDL_FULLSCREEN);
		}

		SDL_Surface *sf = SDL_GetVideoSurface();
		SDL_Surface **surface = &sf;
		uint32 *flags = NULL;
		void *pixels = NULL;
		SDL_Color *palette = NULL;
		SDL_Rect clip;
		int ncolors = 0;
		Uint32 tmpflags = 0;
		int w = 0;
		int h = 0;
		int bpp = 0;

		if ( (!surface) || (!(*surface)) )  // don't bother if there's no surface.
			return;

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

		tmpflags = (*surface)->flags;
		w = (*surface)->w;
		h = (*surface)->h;
		bpp = (*surface)->format->BitsPerPixel;

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n w = %d, h = %d, bpp = %d",__FILE__,__FUNCTION__,__LINE__,w,h,bpp);

		if (flags == NULL)  // use the surface's flags.
			flags = &tmpflags;

		//
		if ( *flags & SDL_FULLSCREEN )
			*flags &= ~SDL_FULLSCREEN;
		//
		else
			*flags |= SDL_FULLSCREEN;

		SDL_GetClipRect(*surface, &clip);

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

		*surface = SDL_SetVideoMode(w, h, bpp, (*flags));

		if (*surface == NULL) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
			*surface = SDL_SetVideoMode(w, h, bpp, tmpflags);
		} // if

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

		SDL_SetClipRect(*surface, &clip);
	}
	else {
		HWND handle = GetSDLWindow();
		if(Window::isFullScreen == true) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d] Window::isFullScreen == true [%d]\n",__FILE__,__FUNCTION__,__LINE__,handle);
			ShowWindow(handle, SW_MAXIMIZE);
			//if(Window::getUseDefaultCursorOnly() == false) {
			//	showCursor(false);
			//}
		}
		else {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d] Window::isFullScreen == false [%d]\n",__FILE__,__FUNCTION__,__LINE__,handle);
			ShowWindow(handle, SW_RESTORE);
			//showCursor(true);
		}
	}

#else
	if(Window::allowAltEnterFullscreenToggle == true) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
		SDL_Surface *cur_surface = SDL_GetVideoSurface();
		if(cur_surface != NULL) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
			SDL_WM_ToggleFullScreen(cur_surface);
		}
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}
#endif

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void Window::handleMouseDown(SDL_Event event) {
	static const Uint32 DOUBLECLICKTIME = 500;
	static const int DOUBLECLICKDELTA = 5;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	MouseButton button = getMouseButton(event.button.button);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	// windows implementation uses 120 for the resolution of a standard mouse
	// wheel notch.  However, newer mice have finer resolutions.  I dunno if SDL
	// handles those, but for now we're going to say that each mouse wheel
	// movement is 120.
	if(button == mbWheelUp) {
	    //printf("button == mbWheelUp\n");
		eventMouseWheel(event.button.x, event.button.y, 120);
		return;
	} else if(button == mbWheelDown) {
	    //printf("button == mbWheelDown\n");
		eventMouseWheel(event.button.x, event.button.y, -120);
		return;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	Uint32 ticks = SDL_GetTicks();
	int n = (int) button;

	assert(n >= 0 && n < mbCount);
	if(n >= 0 && n < mbCount) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		if(ticks - lastMouseDown[n] < DOUBLECLICKTIME
				&& abs(lastMouseX[n] - event.button.x) < DOUBLECLICKDELTA
				&& abs(lastMouseY[n] - event.button.y) < DOUBLECLICKDELTA) {

			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			eventMouseDown(event.button.x, event.button.y, button);
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			eventMouseDoubleClick(event.button.x, event.button.y, button);
		}
		else {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			eventMouseDown(event.button.x, event.button.y, button);
		}
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		lastMouseDown[n] = ticks;
		lastMouseX[n] = event.button.x;
		lastMouseY[n] = event.button.y;

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}
}

MouseButton Window::getMouseButton(int sdlButton) {
	switch(sdlButton) {
		case SDL_BUTTON_LEFT:
			return mbLeft;
		case SDL_BUTTON_RIGHT:
			return mbRight;
		case SDL_BUTTON_MIDDLE:
			return mbCenter;
        case SDL_BUTTON_WHEELUP:
            return mbWheelUp;
        case SDL_BUTTON_WHEELDOWN:
            return mbWheelDown;
		default:
			//throw std::runtime_error("Mouse Button > 3 not handled.");
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Mouse Button [%d] not handled.\n",__FILE__,__FUNCTION__,__LINE__,sdlButton);

			return mbUnknown;
	}
}

/*
char Window::getRawKey(SDL_keysym keysym) {
	char result = 0;
	// Because Control messes up unicode character

	if((keysym.mod & (KMOD_LCTRL | KMOD_RCTRL)) == 0) {
		//printf("keysym.unicode = %d [%d]\n",keysym.unicode,0x80);

		//Uint16 c = keysym.unicode;
		//if(c != 0 && (c & 0xFF80) == 0) {
		if(keysym.unicode > 0 && keysym.unicode < 0x80) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			result = static_cast<char>(keysym.unicode);
			//c = toupper(c);
			//result = (c & 0xFF);
			//result = c;

			//printf("result = %d\n",result);

			//if(c != 0) {
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] returning key [%d]\n",__FILE__,__FUNCTION__,__LINE__,result);
			return result;
			//}
		}
	}
	if(keysym.sym <= 255) {
		result = keysym.sym;
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] result [%d]\n",__FILE__,__FUNCTION__,__LINE__,result);
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] returning key [%d]\n",__FILE__,__FUNCTION__,__LINE__,result);

	return result;
}

char Window::getNormalKey(SDL_keysym keysym,bool skipSpecialKeys) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] keysym.sym [%d] skipSpecialKeys = %d.\n",__FILE__,__FUNCTION__,__LINE__,keysym.sym,skipSpecialKeys);

	//SDLKey unicodeKey = static_cast<SDLKey>(getRawKey(keysym));
	char c = getRawKey(keysym);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] c [%d]\n",__FILE__,__FUNCTION__,__LINE__,c);

	SDLKey unicodeKey = SDLK_UNKNOWN;
	if(c > SDLK_UNKNOWN && c < SDLK_LAST) {
		unicodeKey = static_cast<SDLKey>(c);
	}
	if(unicodeKey == SDLK_UNKNOWN) {
		unicodeKey = keysym.sym;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] unicodeKey [%d]\n",__FILE__,__FUNCTION__,__LINE__,unicodeKey);

	//string keyName = SDL_GetKeyName(keysym.sym);
	string keyName = SDL_GetKeyName(unicodeKey);
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] Raw SDL key [%d] mod [%d] unicode [%d] scancode [%d] keyName [%s]\n",__FILE__,__FUNCTION__,__LINE__,keysym.sym,keysym.mod,keysym.unicode,keysym.scancode,keyName.c_str());
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Raw SDL key [%d] mod [%d] unicode [%d] scancode [%d] keyName [%s]\n",__FILE__,__FUNCTION__,__LINE__,keysym.sym,keysym.mod,keysym.unicode,keysym.scancode,keyName.c_str());

	if(skipSpecialKeys == false) {
		if(keyName == "left alt" || keyName == "right alt") {
			return vkAlt;
		}
		else if(keyName == "left ctrl" || keyName == "right ctrl") {
			return vkControl;
		}
		else if(keyName == "left shift" || keyName == "right shift") {
			return vkShift;
		}

		if(keysym.mod & (KMOD_LALT | KMOD_RALT)) {
			return vkAlt;
		}
		else if(keysym.mod & (KMOD_LCTRL | KMOD_RCTRL)) {
			return vkControl;
		}
		else if(keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT)) {
			return vkShift;
		}
	}
	if(keyName == "up arrow" || keyName == "up") {
		return vkUp;
	}
	if(keyName == "left arrow" || keyName == "left") {
		return vkLeft;
	}
	if(keyName == "right arrow" || keyName == "right") {
		return vkRight;
	}
	if(keyName == "down arrow" || keyName == "down") {
		return vkDown;
	}
	if(keyName == "return" || keyName == "enter") {
		return vkReturn;
	}
	if(keyName == "plus sign" || keyName == "plus") {
		return vkAdd;
	}
	if(keyName == "minus sign" || keyName == "minus") {
		return vkSubtract;
	}
	//if(keyName == "escape") {
	//	return vkEscape;
	//}
	if(keyName == "escape") {
		return unicodeKey;
	}
	if(keyName == "tab") {
		return vkTab;
	}
	if(keyName == "backspace") {
		return vkBack;
	}
	if(keyName == "delete") {
		return vkDelete;
	}
	if(keyName == "print-screen") {
		return vkPrint;
	}
	if(keyName == "pause") {
		return vkPause;
	}
	if(keyName == "question mark" || keyName == "?") {
		return '?';
	}
	if(keyName == "space") {
		return ' ';
	}

	if(keyName == "f1" || keyName == "F1") {
		return vkF1;
	}
	if(keyName == "f2" || keyName == "F2") {
		return vkF2;
	}
	if(keyName == "f3" || keyName == "F3") {
		return vkF3;
	}
	if(keyName == "f4" || keyName == "F4") {
		return vkF4;
	}
	if(keyName == "f5" || keyName == "F5") {
		return vkF5;
	}
	if(keyName == "f6" || keyName == "F6") {
		return vkF6;
	}
	if(keyName == "f7" || keyName == "F7") {
		return vkF7;
	}
	if(keyName == "f8" || keyName == "F8") {
		return vkF8;
	}
	if(keyName == "f9" || keyName == "F9") {
		return vkF9;
	}
	if(keyName == "f10" || keyName == "F10") {
		return vkF10;
	}
	if(keyName == "f11" || keyName == "F11") {
		return vkF11;
	}
	if(keyName == "f12" || keyName == "F12") {
		return vkF12;
	}
	if(keyName == "0") {
		return '0';
	}
	if(keyName == "1") {
		return '1';
	}
	if(keyName == "2") {
		return '2';
	}
	if(keyName == "3") {
		return '3';
	}
	if(keyName == "4") {
		return '4';
	}
	if(keyName == "5") {
		return '5';
	}
	if(keyName == "6") {
		return '6';
	}
	if(keyName == "7") {
		return '7';
	}
	if(keyName == "8") {
		return '8';
	}
	if(keyName == "9") {
		return '9';
	}
	if(keyName == "a") {
		return 'A';
	}
	if(keyName == "b") {
		return 'B';
	}
	if(keyName == "c") {
		return 'C';
	}
	if(keyName == "d") {
		return 'D';
	}
	if(keyName == "e") {
		return 'E';
	}
	if(keyName == "f") {
		return 'F';
	}
	if(keyName == "g") {
		return 'G';
	}
	if(keyName == "h") {
		return 'H';
	}
	if(keyName == "i") {
		return 'I';
	}
	if(keyName == "j") {
		return 'J';
	}
	if(keyName == "k") {
		return 'K';
	}
	if(keyName == "l") {
		return 'L';
	}
	if(keyName == "m") {
		return 'M';
	}
	if(keyName == "n") {
		return 'N';
	}
	if(keyName == "o") {
		return 'O';
	}
	if(keyName == "p") {
		return 'P';
	}
	if(keyName == "q") {
		return 'Q';
	}
	if(keyName == "r") {
		return 'R';
	}
	if(keyName == "s") {
		return 'S';
	}
	if(keyName == "t") {
		return 'T';
	}
	if(keyName == "u") {
		return 'U';
	}
	if(keyName == "v") {
		return 'V';
	}
	if(keyName == "w") {
		return 'W';
	}
	if(keyName == "x") {
		return 'X';
	}
	if(keyName == "y") {
		return 'Y';
	}
	if(keyName == "z") {
		return 'Z';
	}

	if(unicodeKey > 0 && unicodeKey <= 255) {
		return unicodeKey;
	}
	return 0;
}

char Window::getKey(SDL_keysym keysym,bool skipSpecialKeys) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] keysym.sym [%d] skipSpecialKeys = %d.\n",__FILE__,__FUNCTION__,__LINE__,keysym.sym,skipSpecialKeys);

	string keyName = SDL_GetKeyName(keysym.sym);
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] Raw SDL key [%d] mod [%d] unicode [%d] scancode [%d] keyName [%s]\n",__FILE__,__FUNCTION__,__LINE__,keysym.sym,keysym.mod,keysym.unicode,keysym.scancode,keyName.c_str());

	char result = getNormalKey(keysym,skipSpecialKeys);
	if(result != 0) {
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] returning key [%d]\n",__FILE__,__FUNCTION__,__LINE__,result);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] returning key [%d]\n",__FILE__,__FUNCTION__,__LINE__,result);

		return result;
	}
	else {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		Uint16 c = 0;
		if(keysym.unicode > 0 && keysym.unicode < 0x80) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			c = keysym.unicode;
			//c = toupper(c);

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] #1 (c & 0xFF) [%d]\n",__FILE__,__FUNCTION__,__LINE__,(c & 0xFF));

			if(c > SDLK_UNKNOWN && c < SDLK_LAST) {
				SDL_keysym newKeysym = keysym;
				newKeysym.sym = static_cast<SDLKey>(c);

				result = getNormalKey(newKeysym,skipSpecialKeys);

				if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] returning key [%d]\n",__FILE__,__FUNCTION__,__LINE__,result);
				return result;
			}
		}
		if(c == 0) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			if(skipSpecialKeys == true) {
				switch(keysym.sym) {
					case SDLK_LALT:
					case SDLK_RALT:
						if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] returning key [%d] vkAlt\n",__FILE__,__FUNCTION__,__LINE__,vkAlt);
						return vkAlt;
					case SDLK_LCTRL:
					case SDLK_RCTRL:
						if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] returning key [%d] vkControl\n",__FILE__,__FUNCTION__,__LINE__,vkControl);
						return vkControl;
					case SDLK_LSHIFT:
					case SDLK_RSHIFT:
						if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] returning key [%d] vkShift\n",__FILE__,__FUNCTION__,__LINE__,vkShift);
						return vkShift;
				}

				if(keysym.mod & (KMOD_LALT | KMOD_RALT)) {
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] returning key [%d] vkAlt\n",__FILE__,__FUNCTION__,__LINE__,vkAlt);

					return vkAlt;
				}
				else if(keysym.mod & (KMOD_LCTRL | KMOD_RCTRL)) {
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] returning key [%d] vkControl\n",__FILE__,__FUNCTION__,__LINE__,vkControl);

					return vkControl;
				}
				else if(keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT)) {
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] returning key [%d] vkShift\n",__FILE__,__FUNCTION__,__LINE__,vkShift);

					return vkShift;
				}
			}

			if(keysym.sym <= 255) {
				c = keysym.sym;
			}
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %u] c = [%d]\n",__FILE__,__FUNCTION__,__LINE__,c);

		result = (c & 0xFF);
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] returning key [%d]\n",__FILE__,__FUNCTION__,__LINE__,result);

		return result;
	}

	result = 0;
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] returning key [%d]\n",__FILE__,__FUNCTION__,__LINE__,result);

	return result;
}
*/

bool isKeyPressed(SDLKey compareKey, SDL_KeyboardEvent input,bool modifiersAllowed) {
	Uint16 c = SDLK_UNKNOWN;
	//if(input.keysym.unicode > 0 && input.keysym.unicode < 0x80) {
	if(input.keysym.unicode > 0) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] input.keysym.unicode = %d input.keysym.mod = %d\n",__FILE__,__FUNCTION__,__LINE__,input.keysym.unicode,input.keysym.mod);

		// When modifiers are pressed the unicode result is wrong
		// example CTRL-3 will give the ESCAPE vslue 27 in unicode
		if( (input.keysym.mod & KMOD_LCTRL) != KMOD_LCTRL &&
			(input.keysym.mod & KMOD_RCTRL) != KMOD_RCTRL &&
			(input.keysym.mod & KMOD_LALT) != KMOD_LALT   &&
			(input.keysym.mod & KMOD_RALT) != KMOD_RALT   &&
			(input.keysym.mod & KMOD_LSHIFT) != KMOD_LSHIFT &&
			(input.keysym.mod & KMOD_RSHIFT) != KMOD_RSHIFT) {
			c = input.keysym.unicode;
			//c = toupper(c);
		}
		else if(c == SDLK_QUESTION &&
				(input.keysym.mod & KMOD_LSHIFT) == KMOD_LSHIFT ||
				(input.keysym.mod & KMOD_RSHIFT) == KMOD_RSHIFT) {
			c = input.keysym.unicode;
		}

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] #1 (c & 0xFF) [%d]\n",__FILE__,__FUNCTION__,__LINE__,(c & 0xFF));
	}
	//if(c == 0) {
	if(c <= SDLK_UNKNOWN || c >= SDLK_LAST) {
		c = input.keysym.sym;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %u] c = [%d]\n",__FILE__,__FUNCTION__,__LINE__,c);

	//c = (c & 0xFF);
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] returning key [%d]\n",__FILE__,__FUNCTION__,__LINE__,c);

	// SDL does NOT handle lowercase
	if(c >= 'A' && c <= 'Z') {
		c = tolower(c);
	}
	// SDL does NOT handle lowercase
	if(compareKey >= 'A' && compareKey <= 'Z') {
		compareKey = (SDLKey)tolower((char)compareKey);
	}

	bool result = (c == compareKey);
	if(result == false) {
		if(compareKey == SDLK_RETURN) {
			result = (c == SDLK_KP_ENTER);
		}
	}

	if(result == true && modifiersAllowed == false) {
		//printf("input.keysym.mod = %d\n",input.keysym.mod);

		if( (input.keysym.mod & KMOD_LCTRL) == KMOD_LCTRL ||
			(input.keysym.mod & KMOD_RCTRL) == KMOD_RCTRL ||
			(input.keysym.mod & KMOD_LALT) == KMOD_LALT   ||
			(input.keysym.mod & KMOD_RALT) == KMOD_RALT   ||
			(input.keysym.mod & KMOD_LSHIFT) == KMOD_LSHIFT ||
			(input.keysym.mod & KMOD_RSHIFT) == KMOD_RSHIFT) {
				result = false;
			}
	}
	string compareKeyName = SDL_GetKeyName(compareKey);
	string pressKeyName = SDL_GetKeyName((SDLKey)c);

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] compareKey [%d - %s] pressed key [%d - %s] result = %d\n",__FILE__,__FUNCTION__,__LINE__,compareKey,compareKeyName.c_str(),c,pressKeyName.c_str(),result);
	//printf ("ISPRESS compareKey [%d - %s] pressed key [%d - %s] input.keysym.sym [%d] input.keysym.unicode [%d] mod = %d result = %d\n",
	//		compareKey,compareKeyName.c_str(),c,pressKeyName.c_str(),input.keysym.sym,input.keysym.unicode,input.keysym.mod,result);

	return result;
}

SDLKey extractKeyPressed(SDL_KeyboardEvent input) {
	SDLKey c = SDLK_UNKNOWN;
	//if(input.keysym.unicode > 0 && input.keysym.unicode < 0x80) {
	if(input.keysym.unicode > 0) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] input.keysym.unicode = %d input.keysym.mod = %d\n",__FILE__,__FUNCTION__,__LINE__,input.keysym.unicode,input.keysym.mod);

		c = (SDLKey)input.keysym.unicode;
		//c = toupper(c);

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] #1 (c & 0xFF) [%d]\n",__FILE__,__FUNCTION__,__LINE__,(c & 0xFF));
	}
	if(c <= SDLK_UNKNOWN || c >= SDLK_LAST) {
		c = input.keysym.sym;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %u] c = [%d]\n",__FILE__,__FUNCTION__,__LINE__,c);

	//c = (SDLKey)(c & 0xFF);
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] returning key [%d]\n",__FILE__,__FUNCTION__,__LINE__,c);

	string pressKeyName = SDL_GetKeyName((SDLKey)c);
	string inputKeyName = SDL_GetKeyName(input.keysym.sym);

	//printf ("PRESS pressed key [%d - %s] input.keysym.sym [%d] input.keysym.unicode [%d] mod = %d\n",
	//		c,pressKeyName.c_str(),input.keysym.sym,input.keysym.unicode,input.keysym.mod);

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] pressed key [%d - %s]\n",__FILE__,__FUNCTION__,__LINE__,c,pressKeyName.c_str());
	//printf ("In [%s::%s Line: %d] pressed key [%d - %s] input [%d - %s] input.keysym.unicode [%d] mod = %d\n",__FILE__,__FUNCTION__,__LINE__,c,pressKeyName.c_str(),input.keysym.sym,inputKeyName.c_str(),input.keysym.unicode,input.keysym.mod);

	return c;
}

bool isAllowedInputTextKey(SDLKey key) {
	bool result = (
	key != SDLK_DELETE &&
	key != SDLK_BACKSPACE &&
	key != SDLK_TAB &&
	key != SDLK_CLEAR &&
	key != SDLK_RETURN &&
	key != SDLK_PAUSE &&
	key != SDLK_UP &&
	key != SDLK_DOWN &&
	key != SDLK_RIGHT &&
	key != SDLK_LEFT &&
	key != SDLK_INSERT &&
	key != SDLK_HOME &&
	key != SDLK_END &&
	key != SDLK_PAGEUP &&
	key != SDLK_PAGEDOWN &&
	key != SDLK_F1 &&
	key != SDLK_F2 &&
	key != SDLK_F3 &&
	key != SDLK_F4 &&
	key != SDLK_F5 &&
	key != SDLK_F6 &&
	key != SDLK_F7 &&
	key != SDLK_F8 &&
	key != SDLK_F9 &&
	key != SDLK_F10 &&
	key != SDLK_F11 &&
	key != SDLK_F12 &&
	key != SDLK_F13 &&
	key != SDLK_F14 &&
	key != SDLK_F15 &&
	key != SDLK_NUMLOCK &&
	key != SDLK_CAPSLOCK &&
	key != SDLK_SCROLLOCK &&
	key != SDLK_RSHIFT &&
	key != SDLK_LSHIFT &&
	key != SDLK_RCTRL &&
	key != SDLK_LCTRL &&
	key != SDLK_RALT &&
	key != SDLK_LALT &&
	key != SDLK_RMETA &&
	key != SDLK_LMETA &&
	key != SDLK_LSUPER &&
	key != SDLK_RSUPER &&
	key != SDLK_MODE &&
	key != SDLK_HELP &&
	key != SDLK_PRINT &&
	key != SDLK_SYSREQ &&
	key != SDLK_BREAK &&
	key != SDLK_MENU &&
	key != SDLK_POWER);

	return result;
}

}}//end namespace
