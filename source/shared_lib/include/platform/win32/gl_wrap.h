// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_PLATFORM_GLWRAP_H_
#define _SHARED_PLATFORM_GLWRAP_H_

#include <windows.h>

#include <gl.h>
#include <glu.h>
#include <glprocs.h>

#include <string>

#include "font.h"
#include "types.h"


#define GLEST_GLPROC(X, Y) inline X( static a= wglGetProcAddress(a); return a;) 

using std::string;

using Shared::Graphics::FontMetrics;

namespace Shared{ namespace Platform{

// =====================================================
//	class PlatformContextGl
// =====================================================

class PlatformContextGl{
protected:
	DeviceContextHandle dch;
	GlContextHandle glch;

public:
	virtual void init(int colorBits, int depthBits, int stencilBits);
	virtual void end();

	virtual void makeCurrent();
	virtual void swapBuffers();
	
	DeviceContextHandle getHandle() const	{return dch;}
};

// =====================================================
//	Global Fcs  
// =====================================================

void createGlFontBitmaps(uint32 &base, const string &type, int size, int width, int charCount, FontMetrics &metrics);
void createGlFontOutlines(uint32 &base, const string &type, int width, float depth, int charCount, FontMetrics &metrics);
const char *getPlatformExtensions(const PlatformContextGl *pcgl);
PROC getGlProcAddress(const char *procName);

}}//end namespace

#endif
