// ==============================================================
//	This file is part of the MegaGlest Shared Library (www.megaglest.org)
//
//	Copyright (C) 2011 Mark Vejvoda and others
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 3 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef TextFTGL_h
#define TextFTGL_h

#ifdef USE_FTGL

#include <FTGL/ftgl.h>

#include "font_text.h"

namespace Shared { namespace Graphics { namespace Gl {

/**
 * Use FTGL for rendering text in OpenGL
 */
//====================================================================
class TextFTGL : public Text
{
public:

	static int faceResolution;

	TextFTGL(FontTextHandlerType type);
	virtual ~TextFTGL();
	virtual void init(string fontName, int fontSize);

	virtual void SetFaceSize(int);
	virtual int GetFaceSize();

	virtual void Render(const char*, const int = -1);
	virtual float Advance(const char*, const int = -1);
	virtual float LineHeight(const char*, const int = -1);

	virtual void Render(const wchar_t*, const int = -1);
	virtual float Advance(const wchar_t*, const int = -1);
	virtual float LineHeight(const wchar_t* = L" ", const int = -1);

private:
	FTFont *ftFont;
	//FTGLPixmapFont *ftFont;
	const char* fontFile;

	const char* findFont(const char *firstFontToTry=NULL);

	void cleanupFont();
};

}}}//end namespace

#endif // USE_FTGL

#endif // TextFTGL_h
