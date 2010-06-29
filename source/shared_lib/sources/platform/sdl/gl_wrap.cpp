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
#include <vector>
//#include <SDL_image.h>
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
	if(PlatformCommon::Private::shouldBeFullscreen) {
		flags |= SDL_FULLSCREEN;
		Window::setIsFullScreen(true);
	}
	else {
		Window::setIsFullScreen(false);
	}

	int resW = PlatformCommon::Private::ScreenWidth;
	int resH = PlatformCommon::Private::ScreenHeight;

#ifndef WIN32
	if(fileExists("megaglest.bmp")) {
		SDL_Surface *icon = SDL_LoadBMP("megaglest.bmp");
	//SDL_Surface *icon = IMG_Load("megaglest.ico");


//#if !defined(MACOSX)
	// Set Icon (must be done before any sdl_setvideomode call)
	// But don't set it on OS X, as we use a nicer external icon there.
//#if WORDS_BIGENDIAN
//	SDL_Surface* icon= SDL_CreateRGBSurfaceFrom((void*)logo,32,32,8,128,0xff000000,0x00ff0000,0x0000ff00,0);
//#else
//	SDL_Surface* icon= SDL_CreateRGBSurfaceFrom((void*)logo,32,32,32,128,0x000000ff,0x0000ff00,0x00ff0000,0xff000000);
//#endif

		printf("In [%s::%s Line: %d] icon = %p\n",__FILE__,__FUNCTION__,__LINE__,icon);
		if(icon == NULL) {
			printf("Error: %s\n", SDL_GetError());
		}
		if(icon != NULL) {
			SDL_WM_SetIcon(icon, NULL);
		}
	}
#endif

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

	SDL_WM_GrabInput(SDL_GRAB_OFF);
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

#ifdef WIN32

int CALLBACK EnumFontFamExProc(ENUMLOGFONTEX *lpelfe,
                               NEWTEXTMETRICEX *lpntme,
                               int FontType,
                               LPARAM lParam) {
	std::vector<std::string> *systemFontList = (std::vector<std::string> *)lParam;
	systemFontList->push_back((char *)lpelfe->elfFullName);
	return 1; // I want to get all fonts
}

#endif

void createGlFontBitmaps(uint32 &base, const string &type, int size, int width,
						 int charCount, FontMetrics &metrics) {
// -adecw-screen-medium-r-normal--18-180-75-75-m-160-gb2312.1980-1	 this is a Chinese font

#ifdef X11_AVAILABLE
	Display* display = glXGetCurrentDisplay();
	if(display == 0) {
		throw std::runtime_error("Couldn't create font: display is 0");
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] trying to load font %s\n",__FILE__,__FUNCTION__,__LINE__,type.c_str());

	XFontStruct* fontInfo = XLoadQueryFont(display, type.c_str());
	if(!fontInfo) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] CANNOT load font %s, falling back to default\n",__FILE__,__FUNCTION__,__LINE__,type.c_str());
		fontInfo = XLoadQueryFont(display, "fixed");
		if(!fontInfo) {
			throw std::runtime_error("Font not found: " + type);
		}
	}

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	// we need the height of 'a' which sould ~ be half ascent+descent
	float height = (static_cast<float>(fontInfo->ascent + fontInfo->descent) / 2);
	if(height <= 0) {
		height = static_cast<float>(6);
	}
	metrics.setHeight(height);

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] height = %f\n",__FILE__,__FUNCTION__,__LINE__,height);

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	for(unsigned int i = 0; i < static_cast<unsigned int> (charCount); ++i) {

		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		if(i < fontInfo->min_char_or_byte2 || i > fontInfo->max_char_or_byte2) {

			float width = static_cast<float>(6);
			//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] setting size = %f\n",__FILE__,__FUNCTION__,__LINE__,width);

			metrics.setWidth(i, width);
		} else {
			//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			int p = i - fontInfo->min_char_or_byte2;
			//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] p = %d fontInfo->per_char = %p\n",__FILE__,__FUNCTION__,__LINE__,p,fontInfo->per_char);

			if(fontInfo->per_char == NULL) {
				//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] type = [%s] p = %d fontInfo->per_char = %p\n",__FILE__,__FUNCTION__,__LINE__,type.c_str(),p,fontInfo->per_char);

				XCharStruct *charinfo = &(fontInfo->min_bounds);
				//int charWidth = charinfo->rbearing - charinfo->lbearing;
			    //int charHeight = charinfo->ascent + charinfo->descent;
			    //int spanLength = (charWidth + 7) / 8;

				if(charinfo != NULL) {
					//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] type = [%s] charinfo->width = %d\n",__FILE__,__FUNCTION__,__LINE__,type.c_str(),charinfo->width);
					metrics.setWidth(i, static_cast<float> (charinfo->width));
				}
				else {
					//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] type = [%s] using size 6\n",__FILE__,__FUNCTION__,__LINE__,type.c_str());

					metrics.setWidth(i, static_cast<float>(6));
				}
			}
			else {
				float width = static_cast<float>(fontInfo->per_char[p].width); //( fontInfo->per_char[p].rbearing - fontInfo->per_char[p].lbearing);
				if(width <= 0) {
					width = static_cast<float>(6);
				}
				//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] type = [%s] using size = %f\n",__FILE__,__FUNCTION__,__LINE__,type.c_str(),width);
				metrics.setWidth(i, width);
			}
		}
		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	glXUseXFont(fontInfo->fid, 0, charCount, base);

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	GLenum glerror = ::glGetError();

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] glerror = %d\n",__FILE__,__FUNCTION__,__LINE__,glerror);

	XFreeFont(display, fontInfo);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
#else
    // we badly need a solution portable to more than just glx
	//NOIMPL;

	std::string useRealFontName = type;
    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] trying to load useRealFontName [%s], size = %d, width = %d\n",__FILE__,__FUNCTION__,__LINE__,useRealFontName.c_str(),size,width);

	static std::vector<std::string> systemFontList;
	if(systemFontList.size() == 0) {
		LOGFONT lf;
		//POSITION pos;
		//lf.lfCharSet = ANSI_CHARSET;
		lf.lfCharSet = (BYTE)charSet;
		lf.lfFaceName[0]='\0';

		HDC hDC = wglGetCurrentDC();
		::EnumFontFamiliesEx(hDC, 
						  &lf, 
						  (FONTENUMPROC) EnumFontFamExProc, 
						  (LPARAM) &systemFontList, 0);

		for(unsigned int idx = 0; idx < systemFontList.size(); ++idx) {
			string &fontName = systemFontList[idx];
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] found system font [%s]\n",__FILE__,__FUNCTION__,__LINE__,fontName.c_str());
		}
	}
	else {
		for(unsigned int idx = 0; idx < systemFontList.size(); ++idx) {
			string &fontName = systemFontList[idx];
			//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] checking font [%s]\n",__FILE__,__FUNCTION__,__LINE__,fontName.c_str());

			if(_stricmp(useRealFontName.c_str(),fontName.c_str()) != 0) {
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] switch font name from [%s] to [%s]\n",__FILE__,__FUNCTION__,__LINE__,useRealFontName.c_str(),fontName.c_str());

				useRealFontName = fontName;
				break;
			}
		}
	}

	HFONT font= CreateFont(
		size, 0, 0, 0, width, FALSE, FALSE, FALSE, charSet,
		OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
		DEFAULT_PITCH| (useRealFontName.c_str() ? FF_DONTCARE:FF_SWISS), useRealFontName.c_str());

	assert(font!=NULL);

	HDC dc= wglGetCurrentDC();
	SelectObject(dc, font);
	BOOL err= wglUseFontBitmaps(dc, 0, charCount, base);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] wglUseFontBitmaps returned = %d, charCount = %d, base = %d\n",__FILE__,__FUNCTION__,__LINE__,err,charCount,base);
		
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
	
	//MAT2 mat2 = {{0,1},{0,0},{0,0},{0,1}};


	//metrics
	GLYPHMETRICS glyphMetrics;
	int errorCode= GetGlyphOutline(dc, 'a', GGO_METRICS, &glyphMetrics, 0, NULL, &mat2);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] GetGlyphOutline returned = %d\n",__FILE__,__FUNCTION__,__LINE__,errorCode);

	if(errorCode!=GDI_ERROR){
		metrics.setHeight(static_cast<float>(glyphMetrics.gmBlackBoxY));
	}
	for(int i=0; i<charCount; ++i){
		int errorCode= GetGlyphOutline(dc, i, GGO_METRICS, &glyphMetrics, 0, NULL, &mat2);
		if(errorCode!=GDI_ERROR){
			metrics.setWidth(i, static_cast<float>(glyphMetrics.gmCellIncX));
		}
		else {
			//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] GetGlyphOutline returned = %d for i = %d\n",__FILE__,__FUNCTION__,__LINE__,errorCode,i);
			metrics.setWidth(i, static_cast<float>(6));
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
