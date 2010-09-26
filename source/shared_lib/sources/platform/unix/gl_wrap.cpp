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
//#include <SDL_image.h>
#include "leak_dumper.h"

using namespace Shared::Graphics::Gl;
using namespace Shared::Util;

namespace Shared{ namespace Platform{
	


// ======================================
//	Global Fcs  
// ======================================

void createGlFontBitmaps(uint32 &base, const string &type, int size, int width,
						 int charCount, FontMetrics &metrics) {
	Display* display = glXGetCurrentDisplay();
	if(display == 0) {
		throw std::runtime_error("Couldn't create font: display is 0");
	}
	XFontStruct* fontInfo = XLoadQueryFont(display, type.c_str());
	if(!fontInfo) {
		throw std::runtime_error("Font not found.");
	}

	// we need the height of 'a' which sould ~ be half ascent+descent
	metrics.setHeight(static_cast<float> 
			(fontInfo->ascent + fontInfo->descent) / 2);
	for(unsigned int i = 0; i < static_cast<unsigned int> (charCount); ++i) {
		if(i < fontInfo->min_char_or_byte2 ||
				i > fontInfo->max_char_or_byte2) {
			metrics.setWidth(i, static_cast<float>(6));
		} else {
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
