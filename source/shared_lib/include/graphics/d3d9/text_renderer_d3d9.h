// ==============================================================
//	This file is part of MegaGlest Shared Library (www.megaglest.org)
//
//	Copyright (C) 2012 Mark Vejvoda, Titus Tscharntke
//	The Megaglest Team, under GNU GPL v3.0
// ==============================================================

#ifndef _SHARED_D3D9_TEXTRENDERERD3D9_H_
#define _SHARED_D3D9_TEXTRENDERERD3D9_H_

#include "text_renderer.h"

namespace Shared{ namespace Graphics{ namespace D3d9{

// ===============================
//	class TextRenderer2DD3d9  
// ===============================

class TextRenderer2DD3d9: public TextRenderer2D{
private:
	const Font *font;
	Vec4f color;

public:
	virtual void begin(const Font2D *font);
	virtual void render(const string &text, int x, int y, bool centered= false);
	virtual void end();
};

}}}//end namespace

#endif
