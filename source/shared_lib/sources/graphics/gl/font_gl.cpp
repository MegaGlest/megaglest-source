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

#include "font_gl.h"

#include "opengl.h"
#include "gl_wrap.h"
#include "leak_dumper.h"

namespace Shared { namespace Graphics { namespace Gl {

using namespace Platform;

// =====================================================
//	class Font2DGl
// =====================================================

string FontGl::default_fonttype = "fixed";

void Font2DGl::init() {
	if(inited == false) {
		if(getTextHandler() == NULL) {
			assertGl();
			handle= glGenLists(charCount);
			assertGl();

			createGlFontBitmaps(handle, type, size, width, charCount, metrics);
			assertGl();
		}
		inited= true;
	}
}

void Font2DGl::end() {
	if(inited) {
		if(getTextHandler() == NULL) {
			assertGl();
			//assert(glIsList(handle));
			glDeleteLists(handle, 1);
			assertGl();
		}
		inited = false;
	}
}

// =====================================================
//	class Font3DGl
// =====================================================

void Font3DGl::init() {
	if(inited == false) {
		if(getTextHandler() == NULL) {
			assertGl();
			handle= glGenLists(charCount);
			createGlFontOutlines(handle, type, width, depth, charCount, metrics);
			assertGl();
		}
		inited= true;
	}
}

void Font3DGl::end() {
	if(inited) {
		if(getTextHandler() == NULL) {
			assertGl();
			assert(glIsList(handle));
			glDeleteLists(handle, 1);
			assertGl();
		}
	}
}

}}}//end namespace
