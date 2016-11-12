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
SDL_Window *Window::sdlWindow = 0;
int64 Window::lastMouseEvent = 0;	/** for use in mouse hover calculations */
Vec2i Window::mousePos;
MouseState Window::mouseState;
bool Window::isKeyPressedDown = false;
bool Window::isFullScreen = false;
SDL_keysym Window::keystate;
int64 Window::lastToggle = -1000;

bool Window::isActive = false;
#ifdef WIN32
bool Window::allowAltEnterFullscreenToggle = false;
#else
bool Window::allowAltEnterFullscreenToggle = true;
#endif
int Window::lastShowMouseState = 0;

bool Window::tryVSynch = false;

map<wchar_t,bool> Window::mapAllowedKeys;

//bool Window::masterserverMode = false;

// ========== PUBLIC ==========

#ifdef WIN32

static HWND GetSDLWindow()
{
    SDL_SysWMinfo   info;
    SDL_VERSION(&info.version);
    if (SDL_GetWindowWMInfo(Window::getSDLWindow(),&info) == -1)
        return NULL;
    return info.info.win.window;
}

#endif

static bool isUnprintableChar(SDL_keysym key, SDL_Keymod mod) {
    switch (key.sym) {
        // We want to allow some, which are handled specially
		case SDLK_RETURN:
		case SDLK_TAB:
		case SDLK_BACKSPACE:
		case SDLK_DELETE:
		case SDLK_HOME:
		case SDLK_END:
		case SDLK_LEFT:
		case SDLK_RIGHT:
		case SDLK_UP:
		case SDLK_DOWN:
		case SDLK_PAGEUP:
		case SDLK_PAGEDOWN:
			return true;
		default:// do nothing
			break;
    }
		// U+0000 to U+001F are control characters
		/* Don't post text events for unprintable characters */

    if(StartsWith(SDL_GetKeyName(key.sym),"SDLK_KP")){
    	return false;
    }
	if (key.sym > 127) {
		return true;
	}
	if(key.sym < 0x20) {
		return true;
	}

	//printf("isUnprintableChar returns false for [%d]\n",key.sym);
	return false;

}

Window::Window()  {
	this->sdlWindow=0;
	// Default to 1x1 until set by caller to avoid divide by 0
	//this->w = 1;
	//this->h = 1;

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

Window::Window(SDL_Window *sdlWindow)  {
	this->sdlWindow=sdlWindow;
	// Default to 1x1 until set by caller to avoid divide by 0
	//this->w = 1;
	//this->h = 1;

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

void Window::setSDLWindow(SDL_Window *window) {
	Window::sdlWindow = window;
}
SDL_Window *Window::getSDLWindow() {
	return Window::sdlWindow;
}

bool Window::handleEvent() {
	string codeLocation = "a";

	SDL_Event event;
	SDL_GetMouseState(&oldX,&oldY);

	//codeLocation = "b";

	SDL_StartTextInput();
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
						global_window->handleMouseDown(event);
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
				case SDL_MOUSEWHEEL: {
					//printf("In [%s::%s] Line :%d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
					codeLocation = "g2";
					if(global_window) {
						global_window->handleMouseWheel(event);
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
				case SDL_TEXTINPUT: {
				//case SDL_TEXTEDITING:
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] =================================== START OF SDL SDL_TEXTINPUT ================================\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
					codeLocation = "i";
					Window::isKeyPressedDown = true;
//#ifdef WIN32
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf("KD mod = %d : %d\n",event.key.keysym.mod,SDL_GetModState());
					event.key.keysym.mod = SDL_GetModState();
//#endif

					string keyName = SDL_GetKeyName(event.text.text[0]);
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] Raw SDL key [%d - %c] mod [%d] scancode [%d] keyName [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,event.key.keysym.sym,event.key.keysym.sym,event.key.keysym.mod,event.key.keysym.scancode,keyName.c_str());
					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Raw SDL key [%d] mod [%d] scancode [%d] keyName [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,event.key.keysym.sym,event.key.keysym.mod,event.key.keysym.scancode,keyName.c_str());

#ifdef WIN32
					/* handle ALT+f4 */
					if((keyName == "f4" || keyName == "F4")
							&& (event.key.keysym.mod & (KMOD_LALT | KMOD_RALT))) {
						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d] ALT-F4 pressed.\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
						return false;
					}
#endif
					if(global_window) {
						if(global_window->eventTextInput(event.text.text) == false) {
							event.key.keysym.sym = event.text.text[0];
							global_window->eventKeyDown(event.key);
							global_window->eventKeyPress(event.key);
						}
						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
					}
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] =================================== END OF SDL SDL_TEXTINPUT ================================\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
					break;
				}
				case SDL_KEYDOWN: {
					//printf("In SDL_KEYDOWN\n");
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] =================================== START OF SDL SDL_KEYDOWN ================================\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
					keystate = event.key.keysym;
					bool keyDownConsumed=false;
					if(global_window) {
						keyDownConsumed=global_window->eventSdlKeyDown(event.key);
						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
						switch (event.key.keysym.sym) {
							case SDLK_v: {
								if (event.key.keysym.mod & KMOD_CTRL) {
									/* Ctrl-V, paste form clipbord */
									char *text = SDL_GetClipboardText();
									if (*text) {
										printf("Clipboard text: %s\n", text);
										if(global_window->eventTextInput(text) == true) {
											keyDownConsumed=true;
										}
									} else {
										printf("Clipboard text is empty\n");
									}
									SDL_free(text);
								}
								break;
							}
						default:
							break;
						}
					}

//					// Stop keys which would be handled twice ( one time as text input, one time as key down )
					SDL_Keymod mod = SDL_GetModState();
					if (!isUnprintableChar(event.key.keysym,mod)) {
						//printf("In SDL_KEYDOWN key SKIP [%d]\n",event.key.keysym.sym);
						break;
					}
					codeLocation = "i";
					Window::isKeyPressedDown = true;
//#ifdef WIN32
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf("KD mod = %d : %d\n",event.key.keysym.mod,SDL_GetModState());
					event.key.keysym.mod = SDL_GetModState();
//#endif
					string keyName = SDL_GetKeyName(event.key.keysym.sym);
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] Raw SDL key [%d - %c] mod [%d] scancode [%d] keyName [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,event.key.keysym.sym,event.key.keysym.sym,event.key.keysym.mod,event.key.keysym.scancode,keyName.c_str());
					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Raw SDL key [%d] mod [%d] scancode [%d] keyName [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,event.key.keysym.sym,event.key.keysym.mod,event.key.keysym.scancode,keyName.c_str());

					//printf("In SDL_KEYDOWN key [%d] keyName [%s] mod: %d\n",event.key.keysym.sym,keyName.c_str(),event.key.keysym.mod);

					// handle ALT+Return
					if(  (keyName == "Return" || keyName == "Enter")
							&& (event.key.keysym.mod & (KMOD_LALT | KMOD_RALT))) {
						if(event.key.repeat!=0) break;
						if (Chrono::getCurMillis() - getLastToggle() > 100) {
							toggleFullscreen();
							setLastToggle(Chrono::getCurMillis());
						};
						keyDownConsumed=true;
					}
#ifdef WIN32
					// handle ALT+f4
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
						if(!keyDownConsumed){
							global_window->eventKeyDown(event.key);
							global_window->eventKeyPress(event.key);
						}

						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
					}

					if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] =================================== END OF SDL SDL_KEYDOWN ================================\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
					break;
				}
				case SDL_KEYUP:{
					//printf("In [%s::%s] Line :%d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

					if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] =================================== START OF SDL SDL_KEYUP ================================\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

					codeLocation = "j";

					Window::isKeyPressedDown = false;
//#ifdef WIN32
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf("KU mod = %d : %d\n",event.key.keysym.mod,SDL_GetModState());
					event.key.keysym.mod = SDL_GetModState();
//#endif

					keystate = event.key.keysym;

					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] KEY_UP, Raw SDL key [%d] mod [%d] scancode [%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,event.key.keysym.sym,event.key.keysym.mod,event.key.keysym.scancode);
					//printf("In [%s::%s Line: %d] KEY_UP, Raw SDL key [%d] mod [%d] scancode [%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,event.key.keysym.sym,event.key.keysym.mod,event.key.keysym.scancode);

					if(global_window) {
						global_window->eventKeyUp(event.key);
					}

					// here is the problem, we have with too many key up events:
//					string keyName = SDL_GetKeyName(event.key.keysym.sym);
//					if(  (keyName == "Return" || keyName == "Enter")){
//						setLastToggle(-1000);
//					}

					if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] =================================== END OF SDL SDL_KEYUP ================================\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

					break;
				}
				case SDL_WINDOWEVENT:
				{
					codeLocation = "k";
//					if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] SDL_ACTIVEEVENT event.active.state = %d event.active. = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,event.active.state,event.active.gain);
//
//					if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] Window::isActive = %d event.active.state = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,Window::isActive,event.active.state);
//
//					// Check if the program has lost window focus
//					if ((event.active.state & (SDL_APPACTIVE | SDL_APPINPUTFOCUS))) {
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

					//printf("In SDL_WINDOWEVENT, event.window.event: %d\n",event.window.event);

					/*
					switch(event.window.event) {
						case SDL_WINDOWEVENT_ENTER:
							printf("In SDL_WINDOWEVENT_ENTER\n");
							showCursor(true);
							break;
						case SDL_WINDOWEVENT_LEAVE:
							printf("In SDL_WINDOWEVENT_LEAVE\n");
							showCursor(false);
							break;
						case SDL_WINDOWEVENT_FOCUS_GAINED:
							printf("SDL_WINDOWEVENT_FOCUS_GAINED\n");
							showCursor(true);
							break;
						case SDL_WINDOWEVENT_FOCUS_LOST:
							printf("SDL_WINDOWEVENT_FOCUS_LOST\n");
							showCursor(false);
							break;
					}
					*/
					//showCursor(false);

					if(global_window) {
						global_window->eventWindowEvent(event.window);
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
	SDL_StopTextInput();

	return true;
}

void Window::revertMousePos() {
	SDL_WarpMouseInWindow(sdlWindow,oldX, oldY);
}

Vec2i Window::getOldMousePos() {
	return Vec2i(oldX, oldY);
}

string Window::getText() {
	const char* c = 0;
	//SDL_WM_GetCaption(&c, 0);
	c=SDL_GetWindowTitle(sdlWindow);

	return string(c);
}

float Window::getAspect() {
	return static_cast<float>(getClientH())/getClientW();
}

void Window::setText(string text) {
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	//SDL_WM_SetCaption(text.c_str(), 0);
	SDL_SetWindowTitle(sdlWindow,text.c_str());
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void Window::setSize(int w, int h) {
	//this->w = w;
	//this->h = h;
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
	ontop_win32(this->getScreenWidth(),this->getScreenHeight());
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
			//SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);
			SDL_GL_SetSwapInterval(1);
		}
	#endif

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		// setup LOD bias factor
		//const float lodBias = std::max(std::min( configHandler->Get("TextureLODBias", 0.0f) , 4.0f), -4.0f);
		const float lodBias = max(min(0.0f,4.0f),-4.0f);
		//if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("\n\n\n\n\n$$$$ In [%s::%s Line: %d] lodBias = %f\n\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,lodBias);
		if (std::fabs(lodBias) > 0.01f) {
			glTexEnvf(GL_TEXTURE_FILTER_CONTROL,GL_TEXTURE_LOD_BIAS, lodBias );
		}
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	}
}

void Window::toggleFullscreen() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	Window::isFullScreen = !Window::isFullScreen;

	if(global_window) {
		global_window->eventToggleFullScreen(Window::isFullScreen);
	}
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
}

void Window::handleMouseWheel(SDL_Event event) {
	int x;
	int y;

	if (event.type != SDL_MOUSEWHEEL) {
		return;
	}

	if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	SDL_GetMouseState(&x, &y);

	//	// windows implementation uses 120 for the resolution of a standard mouse
	//	// wheel notch.  However, newer mice have finer resolutions.  I dunno if SDL
	//	// handles those, but for now we're going to say that each mouse wheel
	//	// movement is 120.
	eventMouseWheel(x, y, event.wheel.y * 120);
	return;
}

void Window::handleMouseDown(SDL_Event event) {
	static const Uint32 DOUBLECLICKTIME = 500;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

	MouseButton button = getMouseButton(event.button.button);

	Uint32 ticks = SDL_GetTicks();
	int n = (int) button;

	assert(n >= 0 && n < mbCount);
	if(n >= 0 && n < mbCount) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);

		static const int DOUBLECLICKDELTA = 5;
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
		default:
			//throw std::runtime_error("Mouse Button > 3 not handled.");
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Mouse Button [%d] not handled.\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,sdlButton);

			return mbUnknown;
	}
}

wchar_t Window::convertStringtoSDLKey(const string &value) {
	wchar_t result = SDLK_UNKNOWN;

	if(value.length() >= 1) {
		if(value.length() == 3 && value[0] == '\'' && value[2] == '\'') {
			result = (SDL_Keycode)value[1];
		}
		else {
			bool foundKey = false;
			if(value.length() > 1) {
				for(int i = SDLK_UNKNOWN; i < SDL_NUM_SCANCODES; ++i) {
					SDL_Keycode key = static_cast<SDL_Keycode>(i);
					string keyName = SDL_GetKeyName(key);
					if(value == keyName) {
						result = key;
						foundKey = true;
						break;
					}
				}
			}

			if(foundKey == false) {
				result = (SDL_Keycode)value[0];
			}
		}
	}
	else {
		string sError = "Unsupported key name: [" + value + "]";
		throw megaglest_runtime_error(sError.c_str());
	}

	// Because SDL is based on lower Ascii
	//result = tolower(result);
	return result;
}

void Window::addAllowedKeys(string keyList) {
	clearAllowedKeys();

	if(keyList.empty() == false) {
		vector<string> keys;
		Tokenize(keyList,keys,",");
		if(keys.empty() == false) {
			for(unsigned int keyIndex = 0; keyIndex < keys.size(); ++keyIndex) {
				string key = trim(keys[keyIndex]);

				wchar_t sdl_key = convertStringtoSDLKey(key);
				if(sdl_key != SDLK_UNKNOWN) {
					mapAllowedKeys[sdl_key] = true;
				}

				if(SystemFlags::VERBOSE_MODE_ENABLED)  printf("key: %d [%s] IS ALLOWED\n",sdl_key, key.c_str());
				//printf("key: %d [%s] IS ALLOWED\n",sdl_key, key.c_str());
			}
		}
	}
}
void Window::clearAllowedKeys() {
	mapAllowedKeys.clear();
}

bool Window::isAllowedKey(wchar_t key) {
	map<wchar_t,bool>::const_iterator iterFind = mapAllowedKeys.find(key);
	bool result =(iterFind != mapAllowedKeys.end());

	if(SystemFlags::VERBOSE_MODE_ENABLED) {
		string keyName = SDL_GetKeyName((SDL_Keycode)key);
		printf("key: %d [%s] allowed result: %d\n",key,keyName.c_str(),result);
	}

	return result;
}

bool isKeyPressed(SDL_Keycode compareKey, SDL_KeyboardEvent input,bool modifiersAllowed) {
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
bool isKeyPressed(SDL_Keycode compareKey, SDL_KeyboardEvent input,vector<int> modifiersToCheck) {
	//Uint16 c = SDLK_UNKNOWN;
	SDL_Keycode c = SDLK_UNKNOWN;
	//if(input.keysym.unicode > 0 && input.keysym.unicode < 0x80) {
	if(input.keysym.sym > 0) {
		c = input.keysym.sym;
	}

	//printf("START isKeyPressed input = %d compare = %d mod = %d\n",c,compareKey,input.keysym.mod);


//	if(compareKey == SDLK_QUESTION && (c == SDLK_SLASH && (input.keysym.mod & (KMOD_SHIFT)))) {
//		return true;
//	}
//	else if(compareKey == SDLK_SLASH && (c == SDLK_SLASH && (input.keysym.mod & (KMOD_SHIFT)))) {
//		return false;
//	}

////		string unicodeKeyName = SDL_GetKeyName((SDLKey)input.keysym.unicode);
////
////		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] input.keysym.unicode = %d input.keysym.mod = %d input.keysym.sym = %d unicodeKeyName [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,input.keysym.unicode,input.keysym.mod,input.keysym.sym,unicodeKeyName.c_str());
////		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] input.keysym.unicode = %d input.keysym.mod = %d input.keysym.sym = %d unicodeKeyName [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,input.keysym.unicode,input.keysym.mod,input.keysym.sym,unicodeKeyName.c_str());
//
//		// When modifiers are pressed the unicode result is wrong
//		// example CTRL-3 will give the ESCAPE vslue 27 in unicode
//		if( !(input.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL)) &&
//			!(input.keysym.mod & (KMOD_LALT | KMOD_RALT)) &&
//			!(input.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT)) ) {
//			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
//			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
//
//			c = input.keysym.sym;
//			//c = toupper(c);
//		}
//		else if((input.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT)) &&
//				(input.keysym.sym == SDLK_QUESTION ||
//				 input.keysym.sym == SDLK_AT ||
//				 input.keysym.sym == SDLK_COLON ||
//				 input.keysym.sym == SDLK_LESS ||
//				 input.keysym.sym == SDLK_GREATER ||
//				 input.keysym.sym == SDLK_CARET ||
//				 input.keysym.sym == SDLK_UNDERSCORE ||
//				 input.keysym.sym == SDLK_BACKQUOTE ||
//				 input.keysym.sym == SDLK_EXCLAIM ||
//				 input.keysym.sym == SDLK_QUOTEDBL ||
//				 input.keysym.sym == SDLK_HASH ||
//				 input.keysym.sym == SDLK_DOLLAR ||
//				 input.keysym.sym == SDLK_AMPERSAND ||
//				 input.keysym.sym == SDLK_QUOTE ||
//				 input.keysym.sym == SDLK_LEFTPAREN ||
//				 input.keysym.sym == SDLK_RIGHTPAREN ||
//				 input.keysym.sym == SDLK_ASTERISK ||
//				 input.keysym.sym == SDLK_KP_MULTIPLY ||
//				 input.keysym.sym == SDLK_PLUS ||
//				 input.keysym.sym == SDLK_COMMA ||
//				 input.keysym.sym == SDLK_MINUS ||
//				 input.keysym.sym == SDLK_PERIOD ||
//				 input.keysym.sym == SDLK_SLASH ||
//				 // Need to allow Shift + # key for AZERTY style keyboards
//				 input.keysym.sym == SDLK_0 ||
//				 input.keysym.sym == SDLK_1 ||
//				 input.keysym.sym == SDLK_2 ||
//				 input.keysym.sym == SDLK_3 ||
//				 input.keysym.sym == SDLK_4 ||
//				 input.keysym.sym == SDLK_5 ||
//				 input.keysym.sym == SDLK_6 ||
//				 input.keysym.sym == SDLK_7 ||
//				 input.keysym.sym == SDLK_8 ||
//				 input.keysym.sym == SDLK_9
//				)) {
//			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
//			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
//
//			c = input.keysym.sym;
//		}
//		else if(input.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL)) {
//			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
//			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
//
//			if( (input.keysym.sym >= SDLK_0 && input.keysym.sym <= SDLK_9) ||
//				(input.keysym.sym >= SDLK_KP_0 && input.keysym.sym <= SDLK_KP_9)) {
//				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
//				if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__);
//
//				c = input.keysym.sym;
//			}
//		}
//
//		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] #1 (c & 0xFF) [%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,(c & 0xFF));
//		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem, "In [%s::%s Line: %d] #1 (c & 0xFF) [%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,(c & 0xFF));
//	}
//	//if(c == 0) {
//	if(c <= SDLK_UNKNOWN.sym || c >= SDLK_LAST.sym) {
//		c = input.keysym.sym;
//	}

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
		compareKey = (SDL_Keycode)tolower((char)compareKey);
	}

	bool result = (c == compareKey);
	//printf("result = %d input = %d compare = %d\n",result,c,compareKey);

	if(result == false) {
		if(compareKey == SDLK_RETURN) {
			result = (c == SDLK_KP_ENTER);
		}
		else if(compareKey == SDLK_KP_ENTER) {
			result = (c == SDLK_RETURN);
		}
		else if(compareKey == SDLK_BACKSPACE) {
			result = (c == SDLK_DELETE);
		}
	}
//		else if(compareKey == SDLK_ASTERISK) {
//			result = (c == SDLK_KP_MULTIPLY);
//		}
//		else if(compareKey == SDLK_KP_MULTIPLY) {
//			result = (c == SDLK_ASTERISK);
//		}
//		else if (compareKey == SDLK_0) {
//			result = (c == SDLK_KP_0);
//		} else if (compareKey == SDLK_1) {
//			result = (c == SDLK_KP_1);
//		} else if (compareKey == SDLK_2) {
//			result = (c == SDLK_KP_2);
//		} else if (compareKey == SDLK_3) {
//			result = (c == SDLK_KP_3);
//		} else if (compareKey == SDLK_4) {
//			result = (c == SDLK_KP_4);
//		} else if (compareKey == SDLK_5) {
//			result = (c == SDLK_KP_5);
//		} else if (compareKey == SDLK_6) {
//			result = (c == SDLK_KP_6);
//		} else if (compareKey == SDLK_7) {
//			result = (c == SDLK_KP_7);
//		} else if (compareKey == SDLK_8) {
//			result = (c == SDLK_KP_8);
//		} else if (compareKey == SDLK_9) {
//			result = (c == SDLK_KP_9);
//		} else if (compareKey == SDLK_KP_0) {
//			result = (c == SDLK_0);
//		} else if (compareKey == SDLK_KP_1) {
//			result = (c == SDLK_1);
//		} else if (compareKey == SDLK_KP_2) {
//			result = (c == SDLK_2);
//		} else if (compareKey == SDLK_KP_3) {
//			result = (c == SDLK_3);
//		} else if (compareKey == SDLK_KP_4) {
//			result = (c == SDLK_4);
//		} else if (compareKey == SDLK_KP_5) {
//			result = (c == SDLK_5);
//		} else if (compareKey == SDLK_KP_6) {
//			result = (c == SDLK_6);
//		} else if (compareKey == SDLK_KP_7) {
//			result = (c == SDLK_7);
//		} else if (compareKey == SDLK_KP_8) {
//			result = (c == SDLK_8);
//		} else if (compareKey == SDLK_KP_9) {
//			result = (c == SDLK_9);
//		}
//	}

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
	string pressKeyName = SDL_GetKeyName((SDL_Keycode)c);

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
	if(input.keysym.sym > 0) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] input.keysym.sym = %d input.keysym.mod = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,input.keysym.sym,input.keysym.mod);

		c = input.keysym.sym;
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

	string pressKeyName = SDL_GetKeyName((SDL_Keycode)c);
	//string inputKeyName = SDL_GetKeyName(input.keysym.sym);

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

		if(*textAppend) {
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

SDL_Keycode extractKeyPressed(SDL_KeyboardEvent input) {
	SDL_Keycode c = SDLK_UNKNOWN;
	//if(input.keysym.unicode > 0 && input.keysym.unicode < 0x80) {
	//if(input.keysym.sym > 0) {
	//	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] input.keysym.sym = %d input.keysym.mod = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,input.keysym.sym,input.keysym.mod);

	//	c = input.keysym.sym;
//		if(c <= SDLK_UNKNOWN || c >= SDLK_LAST) {
//			c = SDLKey(c & 0xFF);
//		}

		//c = toupper(c);

		//if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] #1 (c & 0xFF) [%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,(c & 0xFF));
		//if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] #1 (c & 0xFF) [%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,(c & 0xFF));
	//}
	//if(c <= SDLK_UNKNOWN) {
	c = input.keysym.sym;
	//}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %u] c = [%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,c);

	//c = (SDLKey)(c & 0xFF);
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] returning key [%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,c);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] returning key [%d]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,c);

	string pressKeyName = SDL_GetKeyName(c);
	//string inputKeyName = SDL_GetKeyName(input.keysym.sym);

//	printf ("PRESS pressed key [%d - %s] input.keysym.sym [%d] mod = %d\n",
//			c,pressKeyName.c_str(),input.keysym.sym,input.keysym.mod);

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("In [%s::%s Line: %d] pressed key [%d - %s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,c,pressKeyName.c_str());
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] pressed key [%d - %s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,c,pressKeyName.c_str());

	return c;
}

bool isAllowedInputTextKey(wchar_t &key) {
	if(Window::isAllowedKey(key) == true) {
		return true;
	}

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
	key != SDLK_NUMLOCKCLEAR &&
	key != SDLK_CAPSLOCK &&
	key != SDLK_SCROLLLOCK &&
	key != SDLK_RSHIFT &&
	key != SDLK_LSHIFT &&
	key != SDLK_RCTRL &&
	key != SDLK_LCTRL &&
	key != SDLK_RALT &&
	key != SDLK_LALT &&
	key != SDLK_RGUI &&
	key != SDLK_LGUI &&
	key != SDLK_MODE &&
	key != SDLK_HELP &&
	key != SDLK_PRINTSCREEN &&
	key != SDLK_SYSREQ &&
	key != SDLK_PAUSE &&
	key != SDLK_MENU &&
	key != SDLK_POWER);

	string inputKeyName = SDL_GetKeyName((SDL_Keycode)key);
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] pressed key [%d - %s] result = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,key,inputKeyName.c_str(),result);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] pressed key [%d - %s] result = %d\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,key,inputKeyName.c_str(),result);

	return result;
}

bool isAllowedInputTextKey(SDL_Keycode key) {
	if(Window::isAllowedKey(key) == true) {
		return true;
	}

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
	key != SDLK_NUMLOCKCLEAR &&
	key != SDLK_CAPSLOCK &&
	key != SDLK_SCROLLLOCK &&
	key != SDLK_RSHIFT &&
	key != SDLK_LSHIFT &&
	key != SDLK_RCTRL &&
	key != SDLK_LCTRL &&
	key != SDLK_RALT &&
	key != SDLK_LALT &&
	key != SDLK_RGUI &&
	key != SDLK_LGUI &&
	key != SDLK_MODE &&
	key != SDLK_HELP &&
	key != SDLK_PRINTSCREEN &&
	key != SDLK_SYSREQ &&
	key != SDLK_PAUSE &&
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
	return keystate.sym;
}

}}//end namespace
