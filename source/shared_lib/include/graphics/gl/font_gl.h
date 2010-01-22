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

#ifndef _SHARED_GRAPHICS_GL_FONTGL_H_
#define _SHARED_GRAPHICS_GL_FONTGL_H_

#include "font.h"
#include "opengl.h"

namespace Shared{ namespace Graphics{ namespace Gl{

// =====================================================
//	class FontGl
// =====================================================

class FontGl{
protected:
	GLuint handle;

public:
	GLuint getHandle() const				{return handle;}
};

// =====================================================
//	class Font2DGl
//
///	OpenGL bitmap font
// =====================================================

class Font2DGl: public Font2D, public FontGl{
public:
	virtual void init();
	virtual void end();
};

// =====================================================
//	class Font3DGl
//
///	OpenGL outline font
// =====================================================

class Font3DGl: public Font3D, public FontGl{
public:
	virtual void init();
	virtual void end();
};

}}}//end namespace

#endif
