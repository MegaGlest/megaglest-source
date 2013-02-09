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

// ========== STATIC INITIALIZATIONS ==========

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

//bool Window::masterserverMode = false;

// ========== PUBLIC ==========

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

Window::Window()  {
	// Default to 1x1 until set by caller to avoid divide by 0
	this->w = 1;
	this->h = 1;

	for(int idx = 0; idx < mbCount; idx++) {
		lastMouseDown[idx]  = 0;
		lastMouseX[idx]		= 0;
		lastMouseY[idx]		= 0;
	}

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	assert(global_window == 0);

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

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
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
#ifdef WIN32
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	done_win32();
#endif

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	assert(global_window == this);
	global_window = 0;
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

bool Window::handleEvent() {
	string codeLocation = "a";

	SDL_Event event;
	SDL_GetMouseState(&oldX,&oldY);

	//codeLocation = "b";

	while(SDL_PollEvent(&event)) {
		try {
			codeLocation = "c";

			switch(event.type) {
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
				case SDL_MOUSEMOTION:

					//printf("In [%s::%s] Line :%d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
					codeLocation = "d";

           			setLastMouseEvent(Chrono::getCurMillis());
           			setMousePos(Vec2i(event.button.x, event.button.y));
					break;
			}

			codeLocation = "d";

			switch(event.type) {
				case SDL_QUIT:
					//printf("In [%s::%s] Line :%d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
					codeLocation = "e";
					return false;
				case SDL_MOUSEBUTTONDOWN:
					//printf("In [%s::%s] Line :%d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
					codeLocation = "f";

					if(global_window) {
						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
						global_window->handleMouseDown(event);
						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
					}
					break;
				case SDL_MOUSEBUTTONUP: {
					//printf("In [%s::%s] Line :%d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
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
					//printf("In [%s::%s] Line :%d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
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
					//printf("In [%s::%s] Line :%d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

					{

					if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] =================================== START OF SDL SDL_KEYDOWN ================================\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

					codeLocation = "i";
					Window::isKeyPressedDown = true;
//#ifdef WIN32
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf("KD mod = %d : %d\n",event.key.keysym.mod,SDL_GetModState());
					event.key.keysym.mod = SDL_GetModState();
//#endif
					keystate = event.key.keysym;

					string keyName = SDL_GetKeyName(event.key.keysym.sym);
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] Raw SDL key [%d - %c] mod [%d] unicode [%d - %c] scancode [%d] keyName [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,event.key.keysym.sym,event.key.keysym.sym,event.key.keysym.mod,event.key.keysym.unicode,event.key.keysym.unicode,event.key.keysym.scancode,keyName.c_str());
					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Raw SDL key [%d] mod [%d] unicode [%d] scancode [%d] keyName [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,event.key.keysym.sym,event.key.keysym.mod,event.key.keysym.unicode,event.key.keysym.scancode,keyName.c_str());

					/* handle ALT+Return */
					if((keyName == "return" || keyName == "enter")
							&& (event.key.keysym.mod & (KMOD_LALT | KMOD_RALT))) {
						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d] SDLK_RETURN pressed.\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
						toggleFullscreen();
					}
#ifdef WIN32
					/* handle ALT+f4 */
					if((keyName == "f4" || keyName == "F4")
							&& (event.key.keysym.mod & (KMOD_LALT | KMOD_RALT))) {
						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d] ALT-F4 pressed.\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
						return false;
					}
#endif
					if(global_window) {
						//char key = getKey(event.key.keysym,true);
						//key = tolower(key);
						//if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("******************* key [%d]\n",key);

						//event.key.keysym.mod = SDL_GetModState();
						global_window->eventKeyDown(event.key);
						global_window->eventKeyPress(event.key);

						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
					}

					if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] =================================== END OF SDL SDL_KEYDOWN ================================\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
					}
					break;

				case SDL_KEYUP:
					//printf("In [%s::%s] Line :%d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

					if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] =================================== START OF SDL SDL_KEYUP ================================\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

					codeLocation = "j";

					Window::isKeyPressedDown = false;
//#ifdef WIN32
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf("KU mod = %d : %d\n",event.key.keysym.mod,SDL_GetModState());
					event.key.keysym.mod = SDL_GetModState();
//#endif

					keystate = event.key.keysym;

					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] KEY_UP, Raw SDL key [%d] mod [%d] unicode [%d] scancode [%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,event.key.keysym.sym,event.key.keysym.mod,event.key.keysym.unicode,event.key.keysym.scancode);

					if(global_window) {
						//char key = getKey(event.key.keysym,true);
						//key = tolower(key);
						global_window->eventKeyUp(event.key);
					}

					if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] =================================== END OF SDL SDL_KEYUP ================================\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

					break;
				case SDL_ACTIVEEVENT:
				{
//					codeLocation = "k";
//					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] SDL_ACTIVEEVENT event.active.state = %d event.active. = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,event.active.state,event.active.gain);
//
//					if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] Window::isActive = %d event.active.state = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,Window::isActive,event.active.state);
//
//					// Check if the program has lost window focus
//					if ((event.active.state & SDL_APPACTIVE) == SDL_APPACTIVE) {
//						if (event.active.gain == 0) {
//							Window::isActive = false;
//						}
//						else {
//							Window::isActive = true;
//						}
//
//						if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] Window::isActive = %d \n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,Window::isActive);
//
//						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Window::isActive = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,Window::isActive);
//
//						bool willShowCursor = (!Window::isActive || (Window::lastShowMouseState == SDL_ENABLE) || Window::getUseDefaultCursorOnly());
//						showCursor(willShowCursor);
//					}
//					// Check if the program has lost window focus
//					if ((event.active.state & SDL_APPMOUSEFOCUS) != SDL_APPMOUSEFOCUS &&
//						(event.active.state & SDL_APPINPUTFOCUS) != SDL_APPINPUTFOCUS &&
//						(event.active.state & SDL_APPACTIVE) != SDL_APPACTIVE) {
//						if (event.active.gain == 0) {
//							Window::isActive = false;
//						}
//						//else if (event.active.gain == 1) {
//						else {
//							Window::isActive = true;
//						}
//
//						if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] Window::isActive = %d \n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,Window::isActive);
//
//						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Window::isActive = %d, event.active.state = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,Window::isActive,event.active.state);
//						bool willShowCursor = (!Window::isActive || (Window::lastShowMouseState == SDL_ENABLE) || Window::getUseDefaultCursorOnly());
//						showCursor(willShowCursor);
//					}
//				}

					codeLocation = "k";
					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] SDL_ACTIVEEVENT event.active.state = %d event.active. = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,event.active.state,event.active.gain);

					if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] Window::isActive = %d event.active.state = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,Window::isActive,event.active.state);

					// Check if the program has lost window focus
					if ((event.active.state & (SDL_APPACTIVE | SDL_APPINPUTFOCUS))) {
						if (event.active.gain == 0) {
							Window::isActive = false;
						}
						else {
							Window::isActive = true;
						}

						if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] Window::isActive = %d \n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,Window::isActive);

						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Window::isActive = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,Window::isActive);

						bool willShowCursor = (!Window::isActive || (Window::lastShowMouseState == SDL_ENABLE) || Window::getUseDefaultCursorOnly());
						showCursor(willShowCursor);
					}
				}
				break;
			}
		}
		catch(const char *e){
			SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] (a1) Couldn't process event: [%s] codeLocation = %s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,e,codeLocation.c_str());
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] (a1) Couldn't process event: [%s] codeLocation = %s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,e,codeLocation.c_str());
			throw megaglest_runtime_error(e);
		}
		catch(const std::runtime_error& e) {
			SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] (a2) Couldn't process event: [%s] codeLocation = %s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,e.what(),codeLocation.c_str());
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] (a2) Couldn't process event: [%s] codeLocation = %s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,e.what(),codeLocation.c_str());
			throw megaglest_runtime_error(e.what());
		}
		catch(const std::exception& e) {
			SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] (b) Couldn't process event: [%s] codeLocation = %s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,e.what(),codeLocation.c_str());
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] (b) Couldn't process event: [%s] codeLocation = %s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,e.what(),codeLocation.c_str());
		}
		catch(...) {
			SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] (c) Couldn't process event: [UNKNOWN ERROR] codeLocation = %s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,codeLocation.c_str());
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] (c) Couldn't process event: [UNKNOWN ERROR] codeLocation = %s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,codeLocation.c_str());
		}
	}

	return true;
}

void Window::revertMousePos() {
	SDL_WarpMouse(oldX, oldY);
}

Vec2i Window::getOldMousePos() {
	return Vec2i(oldX, oldY);
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
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	SDL_WM_SetCaption(text.c_str(), 0);
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
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
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
#ifdef WIN32
	ontop_win32(this->w,this->h);
#endif
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void Window::destroy() {
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	SDL_Event event;
	event.type = SDL_QUIT;
	SDL_PushEvent(&event);

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void Window::setupGraphicsScreen(int depthBits, int stencilBits, bool hardware_acceleration, bool fullscreen_anti_aliasing) {
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	static int newDepthBits   = depthBits;
	static int newStencilBits = stencilBits;
	if(depthBits >= 0)
		newDepthBits   = depthBits;
	if(stencilBits >= 0)
		newStencilBits = stencilBits;

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false) {
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		if(fullscreen_anti_aliasing == true) {
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS,1);
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2);
		}
		if(hardware_acceleration == true) {
			SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
		}

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 1);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 1);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 1);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, newStencilBits);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, newDepthBits);

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		//const SDL_VideoInfo *info = SDL_GetVideoInfo();
	#ifdef SDL_GL_SWAP_CONTROL
		if(Window::tryVSynch == true) {
			/* we want vsync for smooth scrolling */
			SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);
		}
	#endif

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		// setup LOD bias factor
		//const float lodBias = std::max(std::min( configHandler->Get("TextureLODBias", 0.0f) , 4.0f), -4.0f);
		const float lodBias = max(min(0.0f,4.0f),-4.0f);
		//if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("\n\n\n\n\n$$$$ In [%s::%s Line: %d] lodBias = %f\n\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,lodBias);
	#ifdef USE_STREFLOP
		if (streflop::fabs(static_cast<streflop::Simple>(lodBias)) > 0.01f) {
	#else
		if (fabs(lodBias) > 0.01f) {
	#endif
			glTexEnvf(GL_TEXTURE_FILTER_CONTROL,GL_TEXTURE_LOD_BIAS, lodBias );
		}
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	}
}

void Window::toggleFullscreen() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	Window::isFullScreen = !Window::isFullScreen;
#ifdef WIN32
	/* -- Portable Fullscreen Toggling --
	As of SDL 1.2.10, if width and height are both 0, SDL_SetVideoMode will use the
	width and height of the current video mode (or the desktop mode, if no mode has been set).
	Use 0 for Height, Width, and Color Depth to keep the current values. */

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	if(Window::allowAltEnterFullscreenToggle == true) {

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false) {

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

			SDL_Surface *cur_surface = SDL_GetVideoSurface();
			if(cur_surface != NULL) {
				if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
				Window::isFullScreen = !((cur_surface->flags & SDL_FULLSCREEN) == SDL_FULLSCREEN);
			}

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			SDL_Surface *sf = SDL_GetVideoSurface();

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			SDL_Surface **surface = &sf;

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

			uint32 *flags = NULL;
			//void *pixels = NULL;
			//SDL_Color *palette = NULL;
			SDL_Rect clip;
			//int ncolors = 0;
			Uint32 tmpflags = 0;
			int w = 0;
			int h = 0;
			int bpp = 0;

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			if ( (!surface) || (!(*surface)) )  // don't bother if there's no surface.
				return;

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

			tmpflags = (*surface)->flags;
			w = (*surface)->w;
			h = (*surface)->h;
			bpp = (*surface)->format->BitsPerPixel;

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n w = %d, h = %d, bpp = %d",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,w,h,bpp);

			if (flags == NULL)  // use the surface's flags.
				flags = &tmpflags;

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

			//
			if ( *flags & SDL_FULLSCREEN )
				*flags &= ~SDL_FULLSCREEN;
			//
			else
				*flags |= SDL_FULLSCREEN;

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

			SDL_GetClipRect(*surface, &clip);

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

			*surface = SDL_SetVideoMode(w, h, bpp, (*flags));

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

			if (*surface == NULL) {
				if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
				*surface = SDL_SetVideoMode(w, h, bpp, tmpflags);
			} // if

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

			SDL_SetClipRect(*surface, &clip);

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
		}
	}
	else {
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		HWND handle = GetSDLWindow();
		if(Window::isFullScreen == true) {
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d] Window::isFullScreen == true [%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,handle);
			ShowWindow(handle, SW_MAXIMIZE);
			//if(Window::getUseDefaultCursorOnly() == false) {
			//	showCursor(false);
			//}
		}
		else {
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d] Window::isFullScreen == false [%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,handle);
			ShowWindow(handle, SW_RESTORE);
			//showCursor(true);
		}

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	}

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

#else
	if(Window::allowAltEnterFullscreenToggle == true) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		if(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false) {
			SDL_Surface *cur_surface = SDL_GetVideoSurface();
			if(cur_surface != NULL) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
				SDL_WM_ToggleFullScreen(cur_surface);
			}
		}
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	}
#endif

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void Window::handleMouseDown(SDL_Event event) {
	static const Uint32 DOUBLECLICKTIME = 500;
	static const int DOUBLECLICKDELTA = 5;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	MouseButton button = getMouseButton(event.button.button);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

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

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	Uint32 ticks = SDL_GetTicks();
	int n = (int) button;

	assert(n >= 0 && n < mbCount);
	if(n >= 0 && n < mbCount) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		if(ticks - lastMouseDown[n] < DOUBLECLICKTIME
				&& abs(lastMouseX[n] - event.button.x) < DOUBLECLICKDELTA
				&& abs(lastMouseY[n] - event.button.y) < DOUBLECLICKDELTA) {

			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			eventMouseDown(event.button.x, event.button.y, button);
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			eventMouseDoubleClick(event.button.x, event.button.y, button);
		}
		else {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			eventMouseDown(event.button.x, event.button.y, button);
		}
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		lastMouseDown[n] = ticks;
		lastMouseX[n] = event.button.x;
		lastMouseY[n] = event.button.y;

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
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
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Mouse Button [%d] not handled.\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,sdlButton);

			return mbUnknown;
	}
}

bool isKeyPressed(SDLKey compareKey, SDL_KeyboardEvent input,bool modifiersAllowed) {
	vector<int> modifiersToCheck;
	if(modifiersAllowed == false) {
		modifiersToCheck.push_back(KMOD_LCTRL);
		modifiersToCheck.push_back(KMOD_RCTRL);
		modifiersToCheck.push_back(KMOD_LALT);
		modifiersToCheck.push_back(KMOD_RALT);
	}

	bool result = isKeyPressed(compareKey, input, modifiersToCheck);
	return result;
}
bool isKeyPressed(SDLKey compareKey, SDL_KeyboardEvent input,vector<int> modifiersToCheck) {
	Uint16 c = SDLK_UNKNOWN;
	//if(input.keysym.unicode > 0 && input.keysym.unicode < 0x80) {
	if(input.keysym.unicode > 0) {
		string unicodeKeyName = SDL_GetKeyName((SDLKey)input.keysym.unicode);

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] input.keysym.unicode = %d input.keysym.mod = %d input.keysym.sym = %d unicodeKeyName [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,input.keysym.unicode,input.keysym.mod,input.keysym.sym,unicodeKeyName.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] input.keysym.unicode = %d input.keysym.mod = %d input.keysym.sym = %d unicodeKeyName [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,input.keysym.unicode,input.keysym.mod,input.keysym.sym,unicodeKeyName.c_str());

		// When modifiers are pressed the unicode result is wrong
		// example CTRL-3 will give the ESCAPE vslue 27 in unicode
		if( !(input.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL)) &&
			!(input.keysym.mod & (KMOD_LALT | KMOD_RALT)) &&
			!(input.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT)) ) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

			c = input.keysym.unicode;
			//c = toupper(c);
		}
		else if((input.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT)) &&
				(input.keysym.unicode == SDLK_QUESTION ||
				 input.keysym.unicode == SDLK_AT ||
				 input.keysym.unicode == SDLK_COLON ||
				 input.keysym.unicode == SDLK_LESS ||
				 input.keysym.unicode == SDLK_GREATER ||
				 input.keysym.unicode == SDLK_CARET ||
				 input.keysym.unicode == SDLK_UNDERSCORE ||
				 input.keysym.unicode == SDLK_BACKQUOTE ||
				 input.keysym.unicode == SDLK_EXCLAIM ||
				 input.keysym.unicode == SDLK_QUOTEDBL ||
				 input.keysym.unicode == SDLK_HASH ||
				 input.keysym.unicode == SDLK_DOLLAR ||
				 input.keysym.unicode == SDLK_AMPERSAND ||
				 input.keysym.unicode == SDLK_QUOTE ||
				 input.keysym.unicode == SDLK_LEFTPAREN ||
				 input.keysym.unicode == SDLK_RIGHTPAREN ||
				 input.keysym.unicode == SDLK_ASTERISK ||
				 input.keysym.unicode == SDLK_KP_MULTIPLY ||
				 input.keysym.unicode == SDLK_PLUS ||
				 input.keysym.unicode == SDLK_COMMA ||
				 input.keysym.unicode == SDLK_MINUS ||
				 input.keysym.unicode == SDLK_PERIOD ||
				 input.keysym.unicode == SDLK_SLASH ||
				 // Need to allow Shift + # key for AZERTY style keyboards
				 (input.keysym.unicode >= SDLK_0 && input.keysym.unicode <= SDLK_9))) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

			c = input.keysym.unicode;
		}
		else if(input.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL)) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

			if( (input.keysym.unicode >= SDLK_0 && input.keysym.unicode <= SDLK_9) ||
				(input.keysym.unicode >= SDLK_KP0 && input.keysym.unicode <= SDLK_KP9)) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
				if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

				c = input.keysym.unicode;
			}
		}

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] #1 (c & 0xFF) [%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,(c & 0xFF));
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s Line: %d] #1 (c & 0xFF) [%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,(c & 0xFF));
	}
	//if(c == 0) {
	if(c <= SDLK_UNKNOWN || c >= SDLK_LAST) {
		c = input.keysym.sym;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %u] c = [%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,c);

	//c = (c & 0xFF);
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] returning key [%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,c);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] returning key [%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,c);

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
		else if(compareKey == SDLK_KP_ENTER) {
			result = (c == SDLK_RETURN);
		}
		else if(compareKey == SDLK_ASTERISK) {
			result = (c == SDLK_KP_MULTIPLY);
		}
		else if(compareKey == SDLK_KP_MULTIPLY) {
			result = (c == SDLK_ASTERISK);
		}
		else if(compareKey == SDLK_BACKSPACE) {
			result = (c == SDLK_DELETE);
		}
		else if( compareKey >= SDLK_0 && compareKey <= SDLK_9) {
			switch(compareKey) {
				case SDLK_0:
					result = (c == SDLK_KP0);
					break;
				case SDLK_1:
					result = (c == SDLK_KP1);
					break;
				case SDLK_2:
					result = (c == SDLK_KP2);
					break;
				case SDLK_3:
					result = (c == SDLK_KP3);
					break;
				case SDLK_4:
					result = (c == SDLK_KP4);
					break;
				case SDLK_5:
					result = (c == SDLK_KP5);
					break;
				case SDLK_6:
					result = (c == SDLK_KP6);
					break;
				case SDLK_7:
					result = (c == SDLK_KP7);
					break;
				case SDLK_8:
					result = (c == SDLK_KP8);
					break;
				case SDLK_9:
					result = (c == SDLK_KP9);
					break;
			}
		}
		else if(compareKey >= SDLK_KP0 && compareKey <= SDLK_KP9) {
			switch(compareKey) {
				case SDLK_KP0:
					result = (c == SDLK_0);
					break;
				case SDLK_KP1:
					result = (c == SDLK_1);
					break;
				case SDLK_KP2:
					result = (c == SDLK_2);
					break;
				case SDLK_KP3:
					result = (c == SDLK_3);
					break;
				case SDLK_KP4:
					result = (c == SDLK_4);
					break;
				case SDLK_KP5:
					result = (c == SDLK_5);
					break;
				case SDLK_KP6:
					result = (c == SDLK_6);
					break;
				case SDLK_KP7:
					result = (c == SDLK_7);
					break;
				case SDLK_KP8:
					result = (c == SDLK_8);
					break;
				case SDLK_KP9:
					result = (c == SDLK_9);
					break;
			}
		}
	}

	if(result == true) {
		//printf("input.keysym.mod = %d\n",input.keysym.mod);

		for(unsigned int i = 0; i < modifiersToCheck.size(); ++i) {
			if( (input.keysym.mod & modifiersToCheck[i])) {
				if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] result *WOULD HAVE BEEN TRUE* but is false due to: input.keysym.mod = %d modifiersToCheck[i] = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,input.keysym.mod,modifiersToCheck[i]);
				result = false;
				break;
			}
		}
	}
	string compareKeyName = SDL_GetKeyName(compareKey);
	string pressKeyName = SDL_GetKeyName((SDLKey)c);

	//printf ("In [%s::%s Line: %d] compareKey [%d - %s] pressed key [%d - %s] result = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,compareKey,compareKeyName.c_str(),c,pressKeyName.c_str(),result);

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] compareKey [%d - %s] pressed key [%d - %s] result = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,compareKey,compareKeyName.c_str(),c,pressKeyName.c_str(),result);
	//printf ("ISPRESS compareKey [%d - %s] pressed key [%d - %s] input.keysym.sym [%d] input.keysym.unicode [%d] mod = %d result = %d\n",
	//		compareKey,compareKeyName.c_str(),c,pressKeyName.c_str(),input.keysym.sym,input.keysym.unicode,input.keysym.mod,result);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] compareKey [%d - %s] pressed key [%d - %s] result = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,compareKey,compareKeyName.c_str(),c,pressKeyName.c_str(),result);

	return result;
}

wchar_t extractKeyPressedUnicode(SDL_KeyboardEvent input) {
	wchar_t c = SDLK_UNKNOWN;
	//if(input.keysym.unicode > 0 && input.keysym.unicode < 0x80) {
	if(input.keysym.unicode > 0) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] input.keysym.unicode = %d input.keysym.mod = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,input.keysym.unicode,input.keysym.mod);

		c = input.keysym.unicode;
//		if(c <= SDLK_UNKNOWN || c >= SDLK_LAST) {
//			c = SDLKey(c & 0xFF);
//		}

		//c = toupper(c);

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] #1 (c & 0xFF) [%d] c = [%lc]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,(c & 0xFF),c);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] #1 (c & 0xFF) [%d] c = [%lc]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,(c & 0xFF),c);
	}
	if(c == SDLK_UNKNOWN) {
		c = input.keysym.sym;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %u] c = [%d][%lc]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,c);

	//c = (SDLKey)(c & 0xFF);
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] returning key [%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,c);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] returning key [%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,c);

	string pressKeyName = SDL_GetKeyName((SDLKey)c);
	string inputKeyName = SDL_GetKeyName(input.keysym.sym);

	//printf ("PRESS pressed key [%d - %s] input.keysym.sym [%d] input.keysym.unicode [%d] mod = %d\n",
	//		c,pressKeyName.c_str(),input.keysym.sym,input.keysym.unicode,input.keysym.mod);

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] pressed key [%d - %s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,c,pressKeyName.c_str());
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] pressed key [%d - %s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,c,pressKeyName.c_str());

	return c;
}

vector<int> extractKeyPressedUnicodeLength(string text) {
	vector<int> result;
	unsigned int i = 0;
	for(i = 0; i < text.length();) {
		char c = text[i];
		wchar_t keyW = c;
		wchar_t textAppend[] = { keyW, 0 };

		if(textAppend) {
			wchar_t newKey = textAppend[0];
			if (newKey < 0x80) {
				result.push_back(1);
				//printf("1 char, textCharLength = %d\n",textCharLength.size());
			}
			else if (newKey < 0x800) {
				result.push_back(2);
				//printf("2 char, textCharLength = %d\n",textCharLength.size());
			}
			else {
				result.push_back(3);
				//printf("3 char, textCharLength = %d\n",textCharLength.size());
			}
			i += result[result.size()-1];
		}
	}
	return result;
}

SDLKey extractKeyPressed(SDL_KeyboardEvent input) {
	SDLKey c = SDLK_UNKNOWN;
	//if(input.keysym.unicode > 0 && input.keysym.unicode < 0x80) {
	if(input.keysym.unicode > 0) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] input.keysym.unicode = %d input.keysym.mod = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,input.keysym.unicode,input.keysym.mod);

		c = (SDLKey)input.keysym.unicode;
//		if(c <= SDLK_UNKNOWN || c >= SDLK_LAST) {
//			c = SDLKey(c & 0xFF);
//		}

		//c = toupper(c);

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] #1 (c & 0xFF) [%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,(c & 0xFF));
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] #1 (c & 0xFF) [%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,(c & 0xFF));
	}
	if(c <= SDLK_UNKNOWN || c >= SDLK_LAST) {
		c = input.keysym.sym;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %u] c = [%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,c);

	//c = (SDLKey)(c & 0xFF);
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] returning key [%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,c);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] returning key [%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,c);

	string pressKeyName = SDL_GetKeyName((SDLKey)c);
	string inputKeyName = SDL_GetKeyName(input.keysym.sym);

	//printf ("PRESS pressed key [%d - %s] input.keysym.sym [%d] input.keysym.unicode [%d] mod = %d\n",
	//		c,pressKeyName.c_str(),input.keysym.sym,input.keysym.unicode,input.keysym.mod);

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] pressed key [%d - %s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,c,pressKeyName.c_str());
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] pressed key [%d - %s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,c,pressKeyName.c_str());

	return c;
}

bool isAllowedInputTextKey(wchar_t &key) {
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

	string inputKeyName = SDL_GetKeyName((SDLKey)key);
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] pressed key [%d - %s] result = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,key,inputKeyName.c_str(),result);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] pressed key [%d - %s] result = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,key,inputKeyName.c_str(),result);

	return result;
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

	string inputKeyName = SDL_GetKeyName(key);
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] pressed key [%d - %s] result = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,key,inputKeyName.c_str(),result);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] pressed key [%d - %s] result = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,key,inputKeyName.c_str(),result);

	return result;
}

bool Window::isKeyStateModPressed(int mod) {
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("isKeyStateModPressed mod = %d, keystate.mod = %d, keystate.mod & mod = %d\n",mod,keystate.mod,(keystate.mod & mod));

	if(keystate.mod & mod) {
		return true;
	}
	return false;
}

wchar_t Window::extractLastKeyPressed() {
	wchar_t c = SDLK_UNKNOWN;
	//if(input.keysym.unicode > 0 && input.keysym.unicode < 0x80) {
	if(keystate.unicode > 0) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] input.keysym.unicode = %d input.keysym.mod = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,keystate.unicode,keystate.mod);

		c = keystate.unicode;
//		if(c <= SDLK_UNKNOWN || c >= SDLK_LAST) {
//			c = SDLKey(c & 0xFF);
//		}

		//c = toupper(c);

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] #1 (c & 0xFF) [%d] c = [%lc]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,(c & 0xFF),c);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] #1 (c & 0xFF) [%d] c = [%lc]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,(c & 0xFF),c);
	}
	if(c == SDLK_UNKNOWN) {
		c = keystate.sym;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %u] c = [%d][%lc]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,c);

	//c = (SDLKey)(c & 0xFF);
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] returning key [%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,c);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] returning key [%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,c);

	string pressKeyName = SDL_GetKeyName((SDLKey)c);
	string inputKeyName = SDL_GetKeyName(keystate.sym);

	//printf ("PRESS pressed key [%d - %s] input.keysym.sym [%d] input.keysym.unicode [%d] mod = %d\n",
	//		c,pressKeyName.c_str(),input.keysym.sym,input.keysym.unicode,input.keysym.mod);

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] pressed key [%d - %s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,c,pressKeyName.c_str());
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] pressed key [%d - %s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,c,pressKeyName.c_str());

	return c;
}



}}//end namespace
