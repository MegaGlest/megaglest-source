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

namespace Shared{ namespace Graphics{ namespace Gl{

using namespace Platform;

// =====================================================
//	class Font2DGl
// =====================================================

void Font2DGl::init(){
	assertGl();

	if(!inited){
		handle= glGenLists(charCount);
		createGlFontBitmaps(handle, type, size, width, charCount, metrics);
		inited= true;
	}

	assertGl();
}

void Font2DGl::end(){
	assertGl();

	if(inited){
		assert(glIsList(handle));
		glDeleteLists(handle, 1);
		inited= false;
	}

	assertGl();
}

// =====================================================
//	class Font3DGl
// =====================================================

void Font3DGl::init(){
	assertGl();

	if(!inited){
		handle= glGenLists(charCount);
		createGlFontOutlines(handle, type, width, depth, charCount, metrics);
		inited= true;
	}

	assertGl();
}

void Font3DGl::end(){
	assertGl();

	if(inited){
		assert(glIsList(handle));
		glDeleteLists(handle, 1);
	}

	assertGl();
}

}}}//end namespace
