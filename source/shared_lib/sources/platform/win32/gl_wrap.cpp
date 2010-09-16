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
	//	Global Fcs
	// ======================================
	
	int CALLBACK EnumFontFamExProc(ENUMLOGFONTEX *lpelfe,
								   NEWTEXTMETRICEX *lpntme,
								   int FontType,
								   LPARAM lParam) {
		std::vector<std::string> *systemFontList = (std::vector<std::string> *)lParam;
		systemFontList->push_back((char *)lpelfe->elfFullName);
		return 1; // I want to get all fonts
	}
		
	void createGlFontBitmaps(uint32 &base, const string &type, int size, int width,
							 int charCount, FontMetrics &metrics) {
		// -adecw-screen-medium-r-normal--18-180-75-75-m-160-gb2312.1980-1	 this is a Chinese font
		
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
		
	}
	
	void createGlFontOutlines(uint32 &base, const string &type, int width,
							  float depth, int charCount, FontMetrics &metrics) {
		NOIMPL;
	}
		
}}//end namespace
