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
class TextRenderer2DGl;
class TextRenderer3DGl;

// =====================================================
//	class TextRenderer2DGl
// =====================================================

class TextRenderer2DGl: public TextRenderer2D {
private:
	Font2DGl *font;
	bool rendering;

	//Font3DGl *font3D;
	//TextRenderer3DGl *tester;

public:
	TextRenderer2DGl();
	virtual ~TextRenderer2DGl();

	virtual void begin(Font2D *font);
	virtual void render(const string &text, float x, float y, bool centered=false, Vec3f *color=NULL);
	virtual void end();
};

// =====================================================
//	class TextRenderer3DGl
// =====================================================

class TextRenderer3DGl: public TextRenderer3D {
private:
	Font3DGl *font;
	bool rendering;
	int currentFTGLErrorCount;

	void internalRender(const string &text, float  x, float y, bool centered, Vec3f *color);
	void specialFTGLErrorCheckWorkaround(string text);

public:
	TextRenderer3DGl();
	virtual ~TextRenderer3DGl();

	virtual void begin(Font3D *font);
	virtual void render(const string &text, float x, float y, bool centered=false, Vec3f *color=NULL);
	virtual void end();
};



class TextRendererSafeWrapper {
protected:
	TextRenderer *renderer;
	Font *font;
	bool mustEnd;

public:

	TextRendererSafeWrapper(TextRenderer *renderer,Font *font) {
		mustEnd = false;
		this->renderer = renderer;
		this->font = font;
		begin();
	}
	~TextRendererSafeWrapper() {
		end();
	}

	void begin() {
		if(this->renderer != NULL) {
			if(dynamic_cast<TextRenderer2DGl *>(renderer) != NULL) {
				dynamic_cast<TextRenderer2DGl *>(renderer)->begin(dynamic_cast<Font2D *>(this->font));
				mustEnd = true;
			}
			if(dynamic_cast<TextRenderer3DGl *>(renderer) != NULL) {
				mustEnd = true;
				dynamic_cast<TextRenderer3DGl *>(renderer)->begin(dynamic_cast<Font3D *>(this->font));
			}
		}
	}
	void end() {
		if(this->renderer != NULL && mustEnd == true) {
			this->renderer->end();
			mustEnd = false;
		}
	}
};

}}}//end namespace

#endif
