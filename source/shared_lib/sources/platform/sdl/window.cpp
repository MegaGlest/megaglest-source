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
unsigned int Window::lastMouseEvent = 0;	/** for use in mouse hover calculations */
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
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	string codeLocation = "a";

	SDL_Event event;
	SDL_GetMouseState(&oldX,&oldY);

	codeLocation = "b";
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	while(SDL_PollEvent(&event)) {
		try {
			//printf("START [%d]\n",event.type);
			//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
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

			//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
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
						SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
						global_window->handleMouseDown(event);
						SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
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

					setMouseState(mbLeft, event.motion.state & SDL_BUTTON_LMASK);
					setMouseState(mbRight, event.motion.state & SDL_BUTTON_RMASK);
					setMouseState(mbCenter, event.motion.state & SDL_BUTTON_MMASK);

					if(global_window) {
						global_window->eventMouseMove(event.motion.x, event.motion.y, &getMouseState()); //&ms);
					}
					break;
				}
				case SDL_KEYDOWN:
					//printf("In [%s::%s] Line :%d\n",__FILE__,__FUNCTION__,__LINE__);

					codeLocation = "i";
					Window::isKeyPressedDown = true;
					keystate = event.key.keysym;

					SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Raw SDL key [%d] mod [%d] unicode [%d] scancode [%d]\n",__FILE__,__FUNCTION__,__LINE__,event.key.keysym.sym,event.key.keysym.mod,event.key.keysym.unicode,event.key.keysym.scancode);

					/* handle ALT+Return */
					if(event.key.keysym.sym == SDLK_RETURN
							&& (event.key.keysym.mod & (KMOD_LALT | KMOD_RALT))) {
						SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d] SDLK_RETURN pressed.\n",__FILE__,__FUNCTION__,__LINE__);
						toggleFullscreen();
					}
					if(global_window) {
						global_window->eventKeyDown(getKey(event.key.keysym,true));
						global_window->eventKeyPress(static_cast<char>(event.key.keysym.unicode));

						SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
					}
					break;
				case SDL_KEYUP:
					//printf("In [%s::%s] Line :%d\n",__FILE__,__FUNCTION__,__LINE__);

					codeLocation = "j";

					Window::isKeyPressedDown = false;
					keystate = event.key.keysym;

					if(global_window) {
						global_window->eventKeyUp(getKey(event.key.keysym,true));
					}
					break;
				case SDL_ACTIVEEVENT:
				{
					codeLocation = "k";
					SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] SDL_ACTIVEEVENT.\n",__FILE__,__FUNCTION__,__LINE__);

					// Check if the program has lost keyboard focus
					if (event.active.state == SDL_APPINPUTFOCUS) {
						if (event.active.gain == 0) {
							Window::isActive = false;
						}
						else if (event.active.gain == 1) {
							Window::isActive = true;
						}

						SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Window::isActive = %d\n",__FILE__,__FUNCTION__,__LINE__,Window::isActive);

						bool willShowCursor = (!Window::isActive || (Window::lastShowMouseState == SDL_ENABLE) || Window::getUseDefaultCursorOnly());
						showCursor(willShowCursor);
					}
					// Check if the program has lost window focus
					else if (event.active.state == SDL_APPACTIVE) {
						if (event.active.gain == 0) {
							Window::isActive = false;
						}
						else if (event.active.gain == 1) {
							Window::isActive = true;
						}

						SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Window::isActive = %d\n",__FILE__,__FUNCTION__,__LINE__,Window::isActive);

						bool willShowCursor = (!Window::isActive || (Window::lastShowMouseState == SDL_ENABLE) || Window::getUseDefaultCursorOnly());
						showCursor(willShowCursor);
					}
					// Check if the program has lost window focus
					else if (event.active.state == SDL_APPMOUSEFOCUS) {
						if (event.active.gain == 0) {
							Window::isActive = false;
						}
						else if (event.active.gain == 1) {
							Window::isActive = true;
						}

						SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Window::isActive = %d\n",__FILE__,__FUNCTION__,__LINE__,Window::isActive);
						bool willShowCursor = (!Window::isActive || (Window::lastShowMouseState == SDL_ENABLE) || Window::getUseDefaultCursorOnly());
						showCursor(willShowCursor);
					}
					else {
						if (event.active.gain == 0) {
							Window::isActive = false;
						}
						else if (event.active.gain == 1) {
							Window::isActive = true;
						}

						SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Window::isActive = %d, event.active.state = %d\n",__FILE__,__FUNCTION__,__LINE__,Window::isActive,event.active.state);
						bool willShowCursor = (!Window::isActive || (Window::lastShowMouseState == SDL_ENABLE) || Window::getUseDefaultCursorOnly());
						showCursor(willShowCursor);
					}
				} 
				break;
			}
		} 
		catch(const char *e){
			std::cerr << "(a1) Couldn't process event: " << e << " codelocation = " << codeLocation << "\n";
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d] (a1) Couldn't process event: [%s] codeLocation = %s\n",__FILE__,__FUNCTION__,__LINE__,e,codeLocation.c_str());
			throw runtime_error(e);
		}
		catch(const std::runtime_error& e) {
			std::cerr << "(a2) Couldn't process event: " << e.what() << " codelocation = " << codeLocation << "\n";
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d] (a2) Couldn't process event: [%s] codeLocation = %s\n",__FILE__,__FUNCTION__,__LINE__,e.what(),codeLocation.c_str());
			throw runtime_error(e.what());
		}
		catch(const std::exception& e) {
			std::cerr << "(b) Couldn't process event: " << e.what() << " codelocation = " << codeLocation << "\n";
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d] (b) Couldn't process event: [%s] codeLocation = %s\n",__FILE__,__FUNCTION__,__LINE__,e.what(),codeLocation.c_str());
		}
		catch(...) {
			std::cerr << "(c) Couldn't process event: [UNKNOWN ERROR] " << " codelocation = " << codeLocation << "\n";
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d] (c) Couldn't process event: [UNKNOWN ERROR] codeLocation = %s\n",__FILE__,__FUNCTION__,__LINE__,codeLocation.c_str());
		}

		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}

    //printf("END [%d]\n",event.type);
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

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
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 16);
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
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

	Window::isFullScreen = !Window::isFullScreen;
#ifdef WIN32
	/* -- Portable Fullscreen Toggling --
	As of SDL 1.2.10, if width and height are both 0, SDL_SetVideoMode will use the
	width and height of the current video mode (or the desktop mode, if no mode has been set).
	Use 0 for Height, Width, and Color Depth to keep the current values. */

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

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

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

		tmpflags = (*surface)->flags;
		w = (*surface)->w;
		h = (*surface)->h;
		bpp = (*surface)->format->BitsPerPixel;

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n w = %d, h = %d, bpp = %d",__FILE__,__FUNCTION__,__LINE__,w,h,bpp);

		if (flags == NULL)  // use the surface's flags.
			flags = &tmpflags;

		//
		if ( *flags & SDL_FULLSCREEN )
			*flags &= ~SDL_FULLSCREEN;
		//
		else
			*flags |= SDL_FULLSCREEN;

		SDL_GetClipRect(*surface, &clip);

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

		*surface = SDL_SetVideoMode(w, h, bpp, (*flags));

		if (*surface == NULL) {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
			*surface = SDL_SetVideoMode(w, h, bpp, tmpflags);
		} // if

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);

		SDL_SetClipRect(*surface, &clip);
	}
	else {
		HWND handle = GetSDLWindow();
		if(Window::isFullScreen == true) {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d] Window::isFullScreen == true [%d]\n",__FILE__,__FUNCTION__,__LINE__,handle);
			ShowWindow(handle, SW_MAXIMIZE);
			//if(Window::getUseDefaultCursorOnly() == false) {
			//	showCursor(false);
			//}
		}
		else {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d] Window::isFullScreen == false [%d]\n",__FILE__,__FUNCTION__,__LINE__,handle);
			ShowWindow(handle, SW_RESTORE);
			//showCursor(true);
		}
	}
	
#else
	if(Window::allowAltEnterFullscreenToggle == true) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
		SDL_Surface *cur_surface = SDL_GetVideoSurface();
		if(cur_surface != NULL) {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
			SDL_WM_ToggleFullScreen(cur_surface);
		}
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}
#endif

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void Window::handleMouseDown(SDL_Event event) {
	static const Uint32 DOUBLECLICKTIME = 500;
	static const int DOUBLECLICKDELTA = 5;

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	MouseButton button = getMouseButton(event.button.button);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

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

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	Uint32 ticks = SDL_GetTicks();
	int n = (int) button;

	assert(n >= 0 && n < mbCount);
	if(n >= 0 && n < mbCount) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		if(ticks - lastMouseDown[n] < DOUBLECLICKTIME
				&& abs(lastMouseX[n] - event.button.x) < DOUBLECLICKDELTA
				&& abs(lastMouseY[n] - event.button.y) < DOUBLECLICKDELTA) {

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			eventMouseDown(event.button.x, event.button.y, button);
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			eventMouseDoubleClick(event.button.x, event.button.y, button);
		}
		else {
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			eventMouseDown(event.button.x, event.button.y, button);
		}
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		lastMouseDown[n] = ticks;
		lastMouseX[n] = event.button.x;
		lastMouseY[n] = event.button.y;

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
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
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Mouse Button [%d] not handled.\n",__FILE__,__FUNCTION__,__LINE__,sdlButton);

			return mbUnknown;
	}
}

char Window::getKey(SDL_keysym keysym,bool skipSpecialKeys) {
	if(skipSpecialKeys == false) {
		switch(keysym.sym) {
			case SDLK_LALT:
			case SDLK_RALT:
				return vkAlt;
			case SDLK_LCTRL:
			case SDLK_RCTRL:
				return vkControl;
			case SDLK_LSHIFT:
			case SDLK_RSHIFT:
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
	switch(keysym.sym) {
		case SDLK_PLUS:
		case SDLK_KP_PLUS:
			return vkAdd;
		case SDLK_MINUS:
		case SDLK_KP_MINUS:
			return vkSubtract;
		case SDLK_ESCAPE:
			return vkEscape;
		case SDLK_UP:
			return vkUp;
		case SDLK_LEFT:
			return vkLeft;
		case SDLK_RIGHT:
			return vkRight;
		case SDLK_DOWN:
			return vkDown;
		case SDLK_RETURN:
		case SDLK_KP_ENTER:
			return vkReturn;
		case SDLK_TAB:
			return vkTab;
		case SDLK_BACKSPACE:
			return vkBack;
		case SDLK_0:
			return '0';
		case SDLK_1:
			return '1';
		case SDLK_2:
			return '2';
		case SDLK_3:
			return '3';
		case SDLK_4:
			return '4';
		case SDLK_5:
			return '5';
		case SDLK_6:
			return '6';
		case SDLK_7:
			return '7';
		case SDLK_8:
			return '8';
		case SDLK_9:
			return '9';
		case SDLK_QUESTION:
			return '?';
		case SDLK_a:
			return 'A';
		case SDLK_b:
			return 'B';
		case SDLK_c:
			return 'C';
		case SDLK_d:
			return 'D';
		case SDLK_e:
			return 'E';
		case SDLK_f:
			return 'F';
		case SDLK_g:
			return 'G';
		case SDLK_h:
			return 'H';
		case SDLK_i:
			return 'I';
		case SDLK_j:
			return 'J';
		case SDLK_k:
			return 'K';
		case SDLK_l:
			return 'L';
		case SDLK_m:
			return 'M';
		case SDLK_n:
			return 'N';
		case SDLK_o:
			return 'O';
		case SDLK_p:
			return 'P';
		case SDLK_q:
			return 'Q';
		case SDLK_r:
			return 'R';
		case SDLK_s:
			return 'S';
		case SDLK_t:
			return 'T';
		case SDLK_u:
			return 'U';
		case SDLK_v:
			return 'V';
		case SDLK_w:
			return 'W';
		case SDLK_x:
			return 'X';
		case SDLK_y:
			return 'Y';
		case SDLK_z:
			return 'Z';
		default:
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			Uint16 c = keysym.unicode;
			if((c & 0xFF80) == 0) {
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
				c = keysym.unicode & 0x7F;
				c = toupper(c);
			}
			if(c == 0) {
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

				if(skipSpecialKeys == true) {
					switch(keysym.sym) {
						case SDLK_LALT:
						case SDLK_RALT:
							return vkAlt;
						case SDLK_LCTRL:
						case SDLK_RCTRL:
							return vkControl;
						case SDLK_LSHIFT:
						case SDLK_RSHIFT:
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

				c = keysym.sym;
			}

			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %u] c = [%d]\n",__FILE__,__FUNCTION__,__LINE__,c);
			return (c & 0xFF);
			break;
	}

	return 0;
}

}}//end namespace
