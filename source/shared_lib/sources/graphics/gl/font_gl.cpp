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
	//printf("In [%s::%s] Line: %d inited = %d\n",__FILE__,__FUNCTION__,__LINE__,inited);
	if(inited == false) {
		//printf("In [%s::%s] Line: %d inited = %d Font::forceLegacyFonts = %d\n",__FILE__,__FUNCTION__,__LINE__,inited,Font::forceLegacyFonts);
		if(getTextHandler() == NULL || Font::forceLegacyFonts == true) {
			assertGl();
			handle= glGenLists(charCount);
			assertGl();

			//printf("In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
			createGlFontBitmaps((Shared::Platform::uint32&)handle, type, size, width, charCount, metrics);
			assertGl();
		}
		inited= true;
	}
}

void Font2DGl::end() {
	if(inited) {
		if(getTextHandler() == NULL || Font::forceLegacyFonts == true) {
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
	//printf("In [%s::%s] Line: %d\n",__FILE__,__FUNCTION__,__LINE__);
	if(inited == false) {
		if(getTextHandler() == NULL || Font::forceLegacyFonts == true) {
			assertGl();
			handle= glGenLists(charCount);
			createGlFontOutlines((Shared::Platform::uint32&)handle, type, width, depth, charCount, metrics);
			assertGl();
		}
		inited= true;
	}
}

void Font3DGl::end() {
	if(inited) {
		if(getTextHandler() == NULL || Font::forceLegacyFonts == true) {
			assertGl();
			assert(glIsList(handle));
			glDeleteLists(handle, 1);
			assertGl();
		}
	}
}

}}}//end namespace

namespace Shared { namespace Graphics {

	using namespace Gl;
Font3D * ConvertFont2DTo3D(Font2D *font) {

	Font3D *result = new Font3DGl();
	result->setSize(font->getSize());
	result->setType("",font->getType());
	result->setWidth(font->getWidth());
	result->init();
	return result;
}

}}
