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

#ifdef WIN32

#include <windows.h>

#include <GL\gl.h>
#include <GL\glu.h>
#include <glprocs.h>

#define GLEST_GLPROC(X, Y) inline X( static a= wglGetProcAddress(a); return a;) 

#else

#include <SDL.h>
#define GL_GLEXT_PROTOTYPES
#include <SDL_opengl.h>

#endif

#include <string>

#include "font.h"
#include "types.h"

using std::string;

using Shared::Graphics::FontMetrics;

namespace Shared{ namespace Platform{

// =====================================================
//	class PlatformContextGl
// =====================================================

class PlatformContextGl {
public:
	virtual ~PlatformContextGl() {}

	virtual void init(int colorBits, int depthBits, int stencilBits,bool hardware_acceleration, bool fullscreen_anti_aliasing);
	virtual void end();

	virtual void makeCurrent();
	virtual void swapBuffers();

	DeviceContextHandle getHandle() const	{ return 0; }
};

// =====================================================
//	Global Fcs
// =====================================================

// Example values:
// DEFAULT_CHARSET (English) = 1
// GB2312_CHARSET (Chinese)  = 134
#ifdef WIN32
static DWORD charSet = DEFAULT_CHARSET;
#else
static int charSet = 1;
#endif

void createGlFontBitmaps(uint32 &base, const string &type, int size, int width, int charCount, FontMetrics &metrics);
void createGlFontOutlines(uint32 &base, const string &type, int width, float depth, int charCount, FontMetrics &metrics);
const char *getPlatformExtensions(const PlatformContextGl *pcgl);
void* getGlProcAddress(const char *procName);

}}//end namespace

#endif
