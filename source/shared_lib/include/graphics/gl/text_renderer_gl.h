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

#ifndef _SHARED_GRAPHICS_GL_TEXTRENDERERGL_H_
#define _SHARED_GRAPHICS_GL_TEXTRENDERERGL_H_

#include "text_renderer.h"
#include "leak_dumper.h"

namespace Shared { namespace Graphics { namespace Gl {

class Font2DGl;
class Font3DGl;
//#ifdef USE_FTGL
//	class TextFTGL;
//#endif


// =====================================================
//	class TextRenderer2DGl
// =====================================================

class TextRenderer2DGl: public TextRenderer2D {
private:
	Font2DGl *font;
	bool rendering;

//#ifdef USE_FTGL
//	TextFTGL *fontFTGL;
//#endif

public:
	TextRenderer2DGl();
	virtual ~TextRenderer2DGl();

	virtual void begin(Font2D *font);
	virtual void render(const string &text, int x, int y, bool centered, Vec3f *color=NULL);
	virtual void end();
};

// =====================================================
//	class TextRenderer3DGl
// =====================================================

class TextRenderer3DGl: public TextRenderer3D{
private:
	Font3DGl *font;
	bool rendering;

//#ifdef USE_FTGL
//	TextFTGL *fontFTGL;
//#endif

public:
	TextRenderer3DGl();
	virtual ~TextRenderer3DGl();

	virtual void begin(Font3D *font);
	virtual void render(const string &text, float x, float y, float size, bool centered);
	virtual void end();
};

}}}//end namespace

#endif
