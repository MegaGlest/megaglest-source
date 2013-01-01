// ==============================================================
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

#ifndef _SHARED_PLATFORM_WINDOW_H_
#define _SHARED_PLATFORM_WINDOW_H_

#include <map>
#include <string>
#include <SDL.h>
#include <cassert>
#include "data_types.h"
#include "vec.h"
#include <vector>
#include "leak_dumper.h"

using std::map;
using std::vector;
using std::string;
using Shared::Graphics::Vec2i;

#if SDL_VERSION_ATLEAST(2,0,0)

typedef SDL_Keysym SDL_keysym;
typedef SDL_Keycode SDLKey;

#endif

namespace Shared{ namespace Platform{

class Timer;
class PlatformContextGl;

enum MouseButton {
	mbUnknown,
	mbLeft,
	mbCenter,
	mbRight,
	mbWheelUp,
	mbWheelDown,
	mbButtonX1,
	mbButtonX2,

	mbCount
};

enum SizeState{
	ssMaximized,
	ssMinimized,
	ssRestored
};

class MouseState {
private:
	bool states[mbCount];


public:
	MouseState() {
		clear();
	}
	//MouseState(const MouseState &);
	//MouseState &operator=(const MouseState &);
	void clear() { memset(this, 0, sizeof(MouseState)); }

	bool get(MouseButton b) const {
		if(b > 0 && b < mbCount) {
			return states[b];
		}
		return false;
	}

	void set(MouseButton b, bool state) {
		if(b > 0 && b < mbCount) {
			states[b] = state;
		}
	}
};

enum WindowStyle{
	wsFullscreen,
	wsWindowedFixed,
	wsWindowedResizable
};

// =====================================================
//	class Window
// =====================================================

class Window {
private:
	Uint32 lastMouseDown[mbCount];
	int lastMouseX[mbCount];
	int lastMouseY[mbCount];

    static int64 lastMouseEvent;	/** for use in mouse hover calculations */
    static MouseState mouseState;
    static Vec2i mousePos;
    static bool isKeyPressedDown;
	static bool isFullScreen;
	static SDL_keysym keystate;
	static bool tryVSynch;

    static void setLastMouseEvent(int64 lastMouseEvent)	{Window::lastMouseEvent = lastMouseEvent;}
    static int64 getLastMouseEvent() 				    {return Window::lastMouseEvent;}

    static const MouseState &getMouseState() 				    {return Window::mouseState;}
    static void setMouseState(MouseButton b, bool state)		{Window::mouseState.set(b, state);}

    static const Vec2i &getMousePos() 					        {return Window::mousePos;}
    static void setMousePos(const Vec2i &mousePos)				{Window::mousePos = mousePos;}

    static void setKeystate(SDL_keysym state)					{ keystate = state; }

    //static bool masterserverMode;

protected:
	int w, h;
	static bool isActive;
	static bool no2DMouseRendering;
	static bool allowAltEnterFullscreenToggle;
	static int lastShowMouseState;

public:
	static bool handleEvent();
	static void revertMousePos();
	static bool isKeyDown() { return isKeyPressedDown; }
	static void setupGraphicsScreen(int depthBits=-1, int stencilBits=-1, bool hardware_acceleration=false, bool fullscreen_anti_aliasing=false);
	static const bool getIsFullScreen() { return isFullScreen; }
	static void setIsFullScreen(bool value) { isFullScreen = value; }
	//static SDL_keysym getKeystate() { return keystate; }
	static bool isKeyStateModPressed(int mod);
	static wchar_t extractLastKeyPressed();

	Window();
	virtual ~Window();

	virtual bool ChangeVideoMode(bool preserveContext,int resWidth, int resHeight,
			bool fullscreenWindow, int colorBits, int depthBits, int stencilBits,
            bool hardware_acceleration, bool fullscreen_anti_aliasing,
            float gammaValue) = 0;
	//static void setMasterserverMode(bool value) { Window::masterserverMode = value;}
	//static bool getMasterserverMode() { return Window::masterserverMode;}

	static bool getTryVSynch() { return tryVSynch; }
	static void setTryVSynch(bool value) { tryVSynch = value; }

	WindowHandle getHandle()	{return 0;}
	string getText();
	int getX()					{ return 0; }
	int getY()					{ return 0; }
	int getW()					{ return w; }
	int getH()					{ return h; }

	//component state
	int getClientW()			{ return getW(); }
	int getClientH()			{ return getH(); }
	float getAspect();

	//object state
	void setText(string text);
	void setStyle(WindowStyle windowStyle);
	void setSize(int w, int h);
	void setPos(int x, int y);
	void setEnabled(bool enabled);
	void setVisible(bool visible);

	//misc
	void create();
	void destroy();
	void minimize();

	static void setUseDefaultCursorOnly(bool value) { no2DMouseRendering = value; }
	static bool getUseDefaultCursorOnly() { return no2DMouseRendering; }

	static void setAllowAltEnterFullscreenToggle(bool value) { allowAltEnterFullscreenToggle = value; }
	static bool getAllowAltEnterFullscreenToggle() { return allowAltEnterFullscreenToggle; }

	static char getRawKey(SDL_keysym keysym);

protected:
	virtual void eventCreate(){}
	virtual void eventMouseDown(int x, int y, MouseButton mouseButton){}
	virtual void eventMouseUp(int x, int y, MouseButton mouseButton){}
	virtual void eventMouseMove(int x, int y, const MouseState* mouseState){}
	virtual void eventMouseDoubleClick(int x, int y, MouseButton mouseButton){}
	virtual void eventMouseWheel(int x, int y, int zDelta) {}
	virtual void eventKeyDown(SDL_KeyboardEvent key) {}
	virtual void eventKeyUp(SDL_KeyboardEvent key) {}
	virtual void eventKeyPress(SDL_KeyboardEvent c) {}
	virtual void eventResize() {};
	virtual void eventPaint() {}
	virtual void eventTimer(int timerId) {}
	virtual void eventActivate(bool activated) {};
	virtual void eventResize(SizeState sizeState) {};
	virtual void eventMenu(int menuId) {}
	virtual void eventClose() {};
	virtual void eventDestroy() {};

private:
	/// needed to detect double clicks
	void handleMouseDown(SDL_Event event);

	static MouseButton getMouseButton(int sdlButton);
	//static char getKey(SDL_keysym keysym, bool skipSpecialKeys=false);
	//static char getNormalKey(SDL_keysym keysym,bool skipSpecialKeys=false);
	static void toggleFullscreen();
};

bool isKeyPressed(SDLKey compareKey, SDL_KeyboardEvent input, vector<int> modifiersToCheck);
bool isKeyPressed(SDLKey compareKey, SDL_KeyboardEvent input, bool modifiersAllowed=true);

SDLKey extractKeyPressed(SDL_KeyboardEvent input);
bool isAllowedInputTextKey(SDLKey key);

wchar_t extractKeyPressedUnicode(SDL_KeyboardEvent input);
vector<int> extractKeyPressedUnicodeLength(string text);
bool isAllowedInputTextKey(wchar_t &key);

}}//end namespace

#endif
