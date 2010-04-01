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

#include <SDL.h>
#ifdef X11_AVAILABLE
#include <GL/glx.h>
#endif

#include "opengl.h"
#include "sdl_private.h"
#include "noimpl.h"
#include "util.h"
#include "window.h"

#include "leak_dumper.h"

using namespace Shared::Graphics::Gl;
using namespace Shared::Util;

namespace Shared{ namespace Platform{

// ======================================
//	class PlatformContextGl
// ======================================

void PlatformContextGl::init(int colorBits, int depthBits, int stencilBits) {
	
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	Window::setupGraphicsScreen(depthBits, stencilBits);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	int flags = SDL_OPENGL;
	if(Private::shouldBeFullscreen) {
		flags |= SDL_FULLSCREEN;
		Window::setIsFullScreen(true);
	}
	else {
		Window::setIsFullScreen(false);
	}

	int resW = Private::ScreenWidth;
	int resH = Private::ScreenHeight;

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] about to set resolution: %d x %d, colorBits = %d.\n",__FILE__,__FUNCTION__,__LINE__,resW,resH,colorBits);

	SDL_Surface* screen = SDL_SetVideoMode(resW, resH, colorBits, flags);
	if(screen == 0) {
		std::ostringstream msg;
		msg << "Couldn't set video mode "
			<< resW << "x" << resH << " (" << colorBits
			<< "bpp " << stencilBits << " stencil "
			<< depthBits << " depth-buffer). SDL Error is: " << SDL_GetError();
		throw std::runtime_error(msg.str());
	}
}

void PlatformContextGl::end() {
}

void PlatformContextGl::makeCurrent() {
}

void PlatformContextGl::swapBuffers() {
	SDL_GL_SwapBuffers();
}

// ======================================
//	Global Fcs
// ======================================

void createGlFontBitmaps(uint32 &base, const string &type, int size, int width,
						 int charCount, FontMetrics &metrics) {
#ifdef X11_AVAILABLE
	Display* display = glXGetCurrentDisplay();
	if(display == 0) {
		throw std::runtime_error("Couldn't create font: display is 0");
	}
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] trying to load font %s\n",__FILE__,__FUNCTION__,__LINE__,type.c_str());
	XFontStruct* fontInfo = XLoadQueryFont(display, type.c_str());
	if(!fontInfo) {
		throw std::runtime_error("Font not found.");
	}

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	// we need the height of 'a' which sould ~ be half ascent+descent
	metrics.setHeight(static_cast<float>
			(fontInfo->ascent + fontInfo->descent) / 2);

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	for(unsigned int i = 0; i < static_cast<unsigned int> (charCount); ++i) {

		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		if(i < fontInfo->min_char_or_byte2 ||
				i > fontInfo->max_char_or_byte2) {
			//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			metrics.setWidth(i, static_cast<float>(6));
		} else {
			//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			int p = i - fontInfo->min_char_or_byte2;
			//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] p = %d fontInfo->per_char = %p\n",__FILE__,__FUNCTION__,__LINE__,p,fontInfo->per_char);
			if(fontInfo->per_char == NULL) {
				XCharStruct *charinfo = &(fontInfo->min_bounds);
				int charWidth = charinfo->rbearing - charinfo->lbearing;
			    int charHeight = charinfo->ascent + charinfo->descent;
			    int spanLength = (charWidth + 7) / 8;

			    metrics.setWidth(i, static_cast<float> (charWidth));
			}
			else {
				metrics.setWidth(i, static_cast<float> (
						fontInfo->per_char[p].rbearing
						- fontInfo->per_char[p].lbearing));
			}
		}
		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	glXUseXFont(fontInfo->fid, 0, charCount, base);

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	XFreeFont(display, fontInfo);

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
#else
    // we badly need a solution portable to more than just glx
	//NOIMPL;
	HFONT font= CreateFont(
		size, 0, 0, 0, width, 0, FALSE, FALSE, ANSI_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, 
		DEFAULT_PITCH, type.c_str());

	assert(font!=NULL);

	HDC dc= wglGetCurrentDC();
	SelectObject(dc, font);
	BOOL err= wglUseFontBitmaps(dc, 0, charCount, base);
		
	FIXED one;
	one.value= 1;
	one.fract= 0;

	FIXED zero;
	zero.value= 0;
	zero.fract= 0;

	MAT2 mat2;
	mat2.eM11= one;
	mat2.eM12= zero;
	mat2.eM21= zero;
	mat2.eM22= one;

	//metrics
	GLYPHMETRICS glyphMetrics;
	int errorCode= GetGlyphOutline(dc, 'a', GGO_METRICS, &glyphMetrics, 0, NULL, &mat2);
	if(errorCode!=GDI_ERROR){
		metrics.setHeight(static_cast<float>(glyphMetrics.gmBlackBoxY));
	}
	for(int i=0; i<charCount; ++i){
		int errorCode= GetGlyphOutline(dc, i, GGO_METRICS, &glyphMetrics, 0, NULL, &mat2);
		if(errorCode!=GDI_ERROR){
			metrics.setWidth(i, static_cast<float>(glyphMetrics.gmCellIncX));
		}
	}

	DeleteObject(font);

	assert(err);

#endif
}

void createGlFontOutlines(uint32 &base, const string &type, int width,
						  float depth, int charCount, FontMetrics &metrics) {
	NOIMPL;
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
