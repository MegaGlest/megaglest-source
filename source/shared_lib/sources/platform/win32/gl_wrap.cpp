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

#include "gl_wrap.h"

#include <cassert>

#include <windows.h>

#include "opengl.h"
#include "leak_dumper.h"

using namespace Shared::Graphics::Gl;

namespace Shared{ namespace Platform{

// =====================================================
//	class PlatformContextGl
// =====================================================

void PlatformContextGl::init(int colorBits, int depthBits, int stencilBits){
	
	int iFormat;
	PIXELFORMATDESCRIPTOR pfd;
	BOOL err;

	//Set8087CW($133F);
	dch = GetDC(GetActiveWindow());
	assert(dch!=NULL);

	ZeroMemory(&pfd, sizeof(pfd));
	pfd.nSize= sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion= 1;
	pfd.dwFlags= PFD_GENERIC_ACCELERATED | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType= PFD_TYPE_RGBA;
	pfd.cColorBits= colorBits;
	pfd.cDepthBits= depthBits;
	pfd.iLayerType= PFD_MAIN_PLANE;
	pfd.cStencilBits= stencilBits; 

	iFormat= ChoosePixelFormat(dch, &pfd);
	assert(iFormat!=0);

	err= SetPixelFormat(dch, iFormat, &pfd);
	assert(err);    

	glch= wglCreateContext(dch);
	if(glch==NULL){
		throw runtime_error("Error initing OpenGL device context");
	}

	makeCurrent();
}

void PlatformContextGl::end(){
	int makeCurrentError= wglDeleteContext(glch);
	assert(makeCurrentError);
}

void PlatformContextGl::makeCurrent(){
	int makeCurrentError= wglMakeCurrent(dch, glch);
	assert(makeCurrentError);
}

void PlatformContextGl::swapBuffers(){
	int swapErr= SwapBuffers(dch);
	assert(swapErr);
}

// ======================================
//	Global Fcs  
// ======================================

void createGlFontBitmaps(uint32 &base, const string &type, int size, int width, int charCount, FontMetrics &metrics){
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
}

void createGlFontOutlines(uint32 &base, const string &type, int width, float depth, int charCount, FontMetrics &metrics){
	HFONT font= CreateFont(
		10, 0, 0, 0, width, 0, FALSE, FALSE, ANSI_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, 
		DEFAULT_PITCH, type.c_str());

	assert(font!=NULL);

	GLYPHMETRICSFLOAT *glyphMetrics= new GLYPHMETRICSFLOAT[charCount];

	HDC dc= wglGetCurrentDC();
	SelectObject(dc, font);
	BOOL err= wglUseFontOutlines(dc, 0, charCount, base, 1000, depth, WGL_FONT_POLYGONS, glyphMetrics);
		
	//load metrics
	metrics.setHeight(glyphMetrics['a'].gmfBlackBoxY);
	for(int i=0; i<charCount; ++i){
		metrics.setWidth(i, glyphMetrics[i].gmfCellIncX);
	}

	DeleteObject(font);
	delete [] glyphMetrics;

	assert(err);
}

const char *getPlatformExtensions(const PlatformContextGl *pcgl){
	typedef const char* (WINAPI * PROCTYPE) (HDC hdc);
	PROCTYPE proc= reinterpret_cast<PROCTYPE>(getGlProcAddress("wglGetExtensionsStringARB"));
	return proc==NULL? "": proc(pcgl->getHandle());
}

PROC getGlProcAddress(const char *procName){
	PROC proc= wglGetProcAddress(procName);
	assert(proc!=NULL);
	return proc;
}

}}//end namespace 
