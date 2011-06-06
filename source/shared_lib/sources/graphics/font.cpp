// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2007 Martio Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "font.h"
#include <stdexcept>
#include "conversion.h"

#include "font_text.h"
#ifdef USE_FTGL
#include "font_textFTGL.h"
#include <vector>
#include <algorithm>
//#include "string_utils.h"
using namespace Shared::Util;
using namespace Shared::Graphics::Gl;
#endif

#include "leak_dumper.h"

using namespace std;
using namespace Shared::Util;

namespace Shared{ namespace Graphics{

// Init statics
int Font::charCount				= 256;
std::string Font::fontTypeName 	= "Times New Roman";
bool Font::fontIsMultibyte 		= false;
//

// =====================================================
//	class FontMetrics
// =====================================================

FontMetrics::FontMetrics(Text *textHandler) {
	this->textHandler 	= textHandler;
	this->widths		= new float[Font::charCount];
	this->height		= 0;

	for(int i=0; i < Font::charCount; ++i) {
		widths[i]= 0;
	}
}

FontMetrics::~FontMetrics() {
	delete [] widths;
	widths = NULL;
}

void FontMetrics::setTextHandler(Text *textHandler) {
	this->textHandler = textHandler;
}

Text * FontMetrics::getTextHandler() {
	return this->textHandler;
}

float FontMetrics::getTextWidth(const string &str) {
	if(textHandler != NULL) {
		return textHandler->Advance(str.c_str());
	}
	else {
		float width= 0.f;
		for(unsigned int i=0; i< str.size() && (int)i < Font::charCount; ++i){
			if(str[i] >= Font::charCount) {
				string sError = "str[i] >= Font::charCount, [" + str + "] i = " + intToStr(i);
				throw runtime_error(sError);
			}
			//Treat 2 byte characters as spaces
			if(str[i] < 0) {
				width+= (widths[97]); // This is the letter a which is a normal wide character and good to use for spacing
				//i++;
			}
			else {
				width+= widths[str[i]];
			}
		}
		return width;
	}
}

float FontMetrics::getHeight() const {
	if(textHandler != NULL) {
		return textHandler->LineHeight(" ");
	}
	else {
		return height;
	}
}

// ===============================================
//	class Font
// ===============================================

Font::Font() {
	inited		= false;
	type		= fontTypeName;
	width		= 400;
	textHandler = NULL;

#ifdef USE_FTGL
	textHandler = new TextFTGL();
	metrics.setTextHandler(this->textHandler);
#endif
}

Font::~Font() {
	if(textHandler) {
		delete textHandler;
	}
	textHandler = NULL;
}

void Font::setType(string typeX11, string typeGeneric) {
	if(textHandler) {
		textHandler->init(typeGeneric,textHandler->GetFaceSize());
	}
	else {
		this->type= type;
	}
}

void Font::setWidth(int width) {
	this->width= width;
}

int Font::getWidth() const	{
	return width;
}

// ===============================================
//	class Font2D
// ===============================================

Font2D::Font2D() {
	size = 10;
}

int Font2D::getSize() const	{
	if(textHandler) {
		return textHandler->GetFaceSize();
	}
	else {
		return size;
	}
}
void Font2D::setSize(int size)	{
	if(textHandler) {
		return textHandler->SetFaceSize(size);
	}
	else {
		this->size= size;
	}
}

// ===============================================
//	class Font3D
// ===============================================

Font3D::Font3D() {
	depth= 10.f;
}

}}//end namespace
