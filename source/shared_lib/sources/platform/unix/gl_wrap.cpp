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

#include <GL/glx.h>

#include "opengl.h"
#include "sdl_private.h"
#include "noimpl.h"
#include "util.h"
#include "window.h"
#include <vector>
#include "font_gl.h"
#include "leak_dumper.h"

using namespace Shared::Graphics::Gl;
using namespace Shared::Util;

namespace Shared { namespace Platform {

// ======================================
//	Global Fcs  
// ======================================

void createGlFontBitmaps(uint32 &base, const string &type, int size, int width,
						 int charCount, FontMetrics &metrics) {

	Display* display = glXGetCurrentDisplay();
	if(display == 0) {
		throw std::runtime_error("Couldn't create font: display is 0");
	}

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("About to try font [%s]\n",type.c_str());
	printf("About to try font [%s]\n",type.c_str());

	XFontStruct* fontInfo = XLoadQueryFont(display, type.c_str());
	if(fontInfo == NULL) {
		string default_font = FontGl::getDefault_fontType();

		//throw std::runtime_error("Font not found: [" + type + "]");
		SystemFlags::OutputDebug(SystemFlags::debugError,"Font not found [%s] trying to fallback to [%s]\n",type.c_str(),default_font.c_str());
		printf("Font not found [%s] trying to fallback to [%s]\n",type.c_str(),default_font.c_str());

		fontInfo = XLoadQueryFont(display, default_font.c_str());
		if(fontInfo == NULL) {
			throw std::runtime_error("Font not found: [" + type + "]");
		}
	}

	// we need the height of 'a' which sould ~ be half ascent+descent
	metrics.setHeight(static_cast<float> 
			(fontInfo->ascent + fontInfo->descent) / 2);


	int first = (fontInfo->min_byte1 << 8) + fontInfo->min_char_or_byte2;
	int last = (fontInfo->max_byte1 << 8) + fontInfo->max_char_or_byte2;
	int count = last - first + 1;

/*
	// 16-bit fonts have more than one row; indexing into
	//     per_char is trickier.
	int rows = fontInfo->max_byte1 - fontInfo->min_byte1 + 1;
	int pages = fontInfo->max_char_or_byte2 - fontInfo->min_char_or_byte2 + 1;
	int byte1, byte2, index;
	int charIndex = 0;
	int charWidth, charHeight;
	XChar2b character;


	for (int i = first; count; i++, count--) {
		bool skipToEnd = false;
		int undefined = 0;
		if (rows == 1) {
		  undefined = (fontInfo->min_char_or_byte2 > i ||
				  fontInfo->max_char_or_byte2 < i);
		}
		else {
		  byte2 = i & 0xff;
		  byte1 = i >> 8;
		  undefined = (fontInfo->min_char_or_byte2 > byte2 ||
				  fontInfo->max_char_or_byte2 < byte2 ||
				  fontInfo->min_byte1 > byte1 ||
				  fontInfo->max_byte1 < byte1);

		}
		if (undefined) {
			skipToEnd = true;
		}
		else if (fontInfo->per_char != NULL) {
		  if (rows == 1) {
			index = i - fontInfo->min_char_or_byte2;
		  }
		  else {
			byte2 = i & 0xff;
			byte1 = i >> 8;

			index =
			  (byte1 - fontInfo->min_byte1) * pages +
			  (byte2 - fontInfo->min_char_or_byte2);
		  }
		  XCharStruct *charinfo = &(fontInfo->per_char[index]);
		  charWidth = charinfo->rbearing - charinfo->lbearing;
		  charHeight = charinfo->ascent + charinfo->descent;
		  if (charWidth == 0 || charHeight == 0) {
			//if (charinfo->width != 0) {
			//}
			skipToEnd = true;
		  }

		  if(skipToEnd == false) {
			  metrics.setWidth(charIndex, static_cast<float> (fontInfo->per_char[index].width));
			  charIndex++;
		  }
		}
		if(skipToEnd == false) {
			character.byte2 = i & 255;
			character.byte1 = i >> 8;
		}
	}
	//Shared::Graphics::Font::charCount = charIndex;
*/


	//for(unsigned int i = 0; fontInfo->per_char != NULL && i < static_cast<unsigned int> (charCount); ++i) {
	for(unsigned int i = 0; fontInfo->per_char != NULL && i < static_cast<unsigned int> (count); ++i) {
		if(i < fontInfo->min_char_or_byte2 ||
				i > fontInfo->max_char_or_byte2) {
			metrics.setWidth(i, static_cast<float>(6));
		}
		else {
			int p = i - fontInfo->min_char_or_byte2;
			metrics.setWidth(i, static_cast<float> (
			fontInfo->per_char[p].width));
	//						fontInfo->per_char[p].rbearing
	//						- fontInfo->per_char[p].lbearing));
		}
	}

	glXUseXFont(fontInfo->fid, 0, charCount, base);
	XFreeFont(display, fontInfo);
}

void createGlFontOutlines(uint32 &base, const string &type, int width,
						  float depth, int charCount, FontMetrics &metrics) {
	NOIMPL;
}


}}//end namespace 
