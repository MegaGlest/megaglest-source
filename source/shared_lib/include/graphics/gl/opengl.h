// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martio Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_GRAPHICS_GL_OPENGL_H_
#define _SHARED_GRAPHICS_GL_OPENGL_H_

#include <GL/glew.h>
#include <cassert>
#include <stdexcept>
#include <string>
#include "conversion.h"
#include "gl_wrap.h"
#include "leak_dumper.h"

using std::runtime_error;
using std::string;

namespace Shared{ namespace Graphics{ namespace Gl{

using Util::intToStr;

// =====================================================
//	Globals
// =====================================================

bool getVBOSupported();
void setVBOSupported(bool value);

void overrideGlExtensionSupport(const char *extensionName,bool value);
bool isGlExtensionSupported(const char *extensionName);
//bool isGlVersionSupported(int major, int minor, int release);
const char *getGlVersion();
const char *getGlRenderer();
const char *getGlVendor();
const char *getGlExtensions();
const char *getGlPlatformExtensions();
int getGlMaxLights();
int getGlMaxTextureSize();
int getGlMaxTextureUnits();
int getGlModelviewMatrixStackDepth();
int getGlProjectionMatrixStackDepth();
void checkGlExtension(const char *extensionName);

void inline _assertGl(const char *file, int line, GLenum *forceErrorNumber = NULL) {
	GLenum error = (forceErrorNumber != NULL ? *forceErrorNumber : glGetError());
	if(error != GL_NO_ERROR) {
		//if(error != GL_INVALID_ENUM) {
			const char *errorString= reinterpret_cast<const char*>(gluErrorString(error));
			char szBuf[4096]="";
			sprintf(szBuf,"OpenGL error #%d [0x%X] : [%s] at file: [%s], line: %d",error,error,errorString,file,line);
			//throw runtime_error("OpenGL error #" + intToStr(error) + " : " + string(errorString) + " at file: " + string(file) + ", line " + intToStr(line));
			throw runtime_error(szBuf);
		//}
	}

}

#ifdef NDEBUG

#define assertGl() ((void) 0);
#define assertGlWithErrorNumber(forceErrorNumber) ((void) 0);

#else

#define assertGl() _assertGl(__FILE__, __LINE__);
#define assertGlWithErrorNumber(forceErrorNumber) _assertGl(__FILE__, __LINE__, &forceErrorNumber);
	
#endif

}}}//end namespace

#endif
