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

#ifndef _SHARED_GRAPHICS_GL_TEXTRENDERERGL_H_
#define _SHARED_GRAPHICS_GL_TEXTRENDERERGL_H_

#include "text_renderer.h"

namespace Shared{ namespace Graphics{ namespace Gl{

class Font2DGl;
class Font3DGl;

// =====================================================
//	class TextRenderer2DGl
// =====================================================

class TextRenderer2DGl: public TextRenderer2D{
private:
	const Font2DGl *font;
	bool rendering;

public:
	TextRenderer2DGl();

	virtual void begin(const Font2D *font);
	virtual void render(const string &text, int x, int y, bool centered);
	virtual void end();
};

// =====================================================
//	class TextRenderer3DGl
// =====================================================

class TextRenderer3DGl: public TextRenderer3D{
private:
	const Font3DGl *font;
	bool rendering;

public:
	TextRenderer3DGl();

	virtual void begin(const Font3D *font);
	virtual void render(const string &text, float x, float y, float size, bool centered);
	virtual void end();
};

}}}//end namespace

#endif
