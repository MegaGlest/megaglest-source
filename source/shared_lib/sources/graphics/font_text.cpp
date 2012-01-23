// ==============================================================
//	This file is part of the MegaGlest Shared Library (www.megaglest.org)
//
//	Copyright (C) 2011 Mark Vejvoda and others
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================
#include "font_text.h"

using namespace std;

std::string Text::DEFAULT_FONT_PATH = "";

//====================================================================
Text::Text(FontTextHandlerType type) {
	this->type = type;
}
Text::~Text() {}
void  Text::init(string fontName, string fontFamilyName, int fontSize) {}
void  Text::Render(const char*, const int) {}
float Text::Advance(const char*, const int) {return 0;}
float Text::LineHeight(const char*, const int) {return 0;}
void  Text::Render(const wchar_t*, const int) {}
float Text::Advance(const wchar_t*, const int) {return 0;}
float Text::LineHeight(const wchar_t*, const int) {return 0;}
void  Text::SetFaceSize(int) {}
int   Text::GetFaceSize() {return 0;}
