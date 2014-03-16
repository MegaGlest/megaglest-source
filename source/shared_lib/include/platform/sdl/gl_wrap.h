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
#ifndef _SHARED_PLATFORM_GLWRAP_H_
#define _SHARED_PLATFORM_GLWRAP_H_

#include <GL/glew.h>

#ifndef WIN32

#define GL_GLEXT_PROTOTYPES

#endif

#include <string>
#include "font.h"
#include "data_types.h"
#include <SDL.h>

#include "leak_dumper.h"

using std::string;

using Shared::Graphics::FontMetrics;

namespace Shared{ namespace Platform{

// =====================================================
//	class PlatformContextGl
// =====================================================

class PlatformContextRendererInterface {
public:
	virtual void setupRenderer(int width, int height) = 0;
	virtual void renderAllContexts() = 0;
};

class PlatformContextWindowInterface {
public:
	virtual void setupGraphicsScreen(int depthBits=-1, int stencilBits=-1, bool hardware_acceleration=false, bool fullscreen_anti_aliasing=false) = 0;
	virtual void setIsFullScreen(bool value) = 0;
};

class PlatformContextGl {
protected:
	SDL_Surface *icon;
	SDL_Surface *screen;

	static PlatformContextRendererInterface *renderer;
	static PlatformContextWindowInterface *window;

public:
	// Example values:
	// DEFAULT_CHARSET (English) = 1
	// GB2312_CHARSET (Chinese)  = 134
	#ifdef WIN32
	static DWORD charSet;
	#else
	static int charSet;
	#endif
	
public:
	PlatformContextGl();
	virtual ~PlatformContextGl();

	static void setRenderer(PlatformContextRendererInterface *rendererObj) { PlatformContextGl::renderer = rendererObj; }
	static void setWindow(PlatformContextWindowInterface *windowObj) { PlatformContextGl::window = windowObj; }

	virtual void init(int colorBits, int depthBits, int stencilBits,
			bool hardware_acceleration, bool fullscreen_anti_aliasing,
			float gammaValue);
	virtual void end();

	virtual void makeCurrent();
	virtual void swapBuffers();

	SDL_Surface * getScreen() { return screen; }

	DeviceContextHandle getHandle() const	{ return 0; }
};

// =====================================================
//	Global Fcs
// =====================================================

#if defined(__APPLE__)
void createGlFontBitmaps(uint32 &base, const string &type, int size, int width, int charCount, FontMetrics &metrics);
void createGlFontOutlines(uint32 &base, const string &type, int width, float depth, int charCount, FontMetrics &metrics);
#else
void createGlFontBitmaps(uint32 &base, const string &type, int size, int width, int charCount, FontMetrics &metrics);
void createGlFontOutlines(uint32 &base, const string &type, int width, float depth, int charCount, FontMetrics &metrics);
#endif

//const char *getPlatformExtensions(const PlatformContextGl *pcgl);
//void* getGlProcAddress(const char *procName);

}}//end namespace

#endif
