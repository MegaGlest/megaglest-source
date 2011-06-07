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

#ifndef Text_h
#define Text_h

#include <string>

using std::string;
/**
 * Base class upon which all text rendering calls are made.
 */
//====================================================================
class Text
{
public:

	static std::string DEFAULT_FONT_PATH;

	Text();
	virtual ~Text();

	virtual void init(string fontName, int fontSize);
	virtual void SetFaceSize(int);
	virtual int GetFaceSize();

	virtual void Render(const char*, const int = -1);
	virtual float Advance(const char*, const int = -1);
	virtual float LineHeight(const char* = " ", const int = -1);

	virtual void Render(const wchar_t*, const int = -1);
	virtual float Advance(const wchar_t*, const int = -1);
	virtual float LineHeight(const wchar_t* = L" ", const int = -1);

};

#endif // Text_h
