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

#ifdef USE_FTGL

#include "font_textFTGL.h"
#include <vector>
#include <algorithm>
using namespace Shared::Util;
using namespace Shared::Graphics::Gl;

#endif

#ifdef USE_FREETYPEGL

#include "font_text_freetypegl.h"

#endif

#include "util.h"
#include "leak_dumper.h"

using namespace std;
using namespace Shared::Util;

namespace Shared { namespace Graphics {

// Init statics
int Font::charCount							= 256;
std::string Font::fontTypeName 				= "Times New Roman";
bool Font::fontIsMultibyte 					= false;
bool Font::forceLegacyFonts					= false;
bool Font::fontIsRightToLeft				= false;
bool Font::forceFTGLFonts					= false;

// This value is used to scale the font text rendering
// in 3D render mode
float Font::scaleFontValue					= 0.80f;
// This value is used for centering font text vertically (height)
float Font::scaleFontValueCenterHFactor		= 4.0f;
//float Font::scaleFontValue					= 1.0;
//float Font::scaleFontValueCenterHFactor		= 4.0;

int Font::baseSize							= 3;

int Font::faceResolution					= 72;
string Font::langHeightText					= "yW";
//

void Font::resetToDefaults() {
	Font::charCount					= 256;
	Font::fontTypeName 				= "Times New Roman";
	Font::fontIsMultibyte 			= false;
	//Font::forceLegacyFonts			= false;
	Font::fontIsRightToLeft			= false;

	// This value is used to scale the font text rendering
	// in 3D render mode
	Font::scaleFontValue					= 0.80f;
	// This value is used for centering font text vertically (height)
	Font::scaleFontValueCenterHFactor		= 4.0f;
	//float Font::scaleFontValue					= 1.0;
	//float Font::scaleFontValueCenterHFactor		= 4.0;

	Font::baseSize							= 3;

	Font::faceResolution					= 72;
	Font::langHeightText					= "yW";

#if defined(WIN32)
	string newEnvValue = "MEGAGLEST_FONT=";
	_putenv(newEnvValue.c_str());
#else
	unsetenv("MEGAGLEST_FONT");
#endif
}

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
		//printf("str [%s] textHandler->Advance = %f Font::scaleFontValue = %f\n",str.c_str(),textHandler->Advance(str.c_str()),Font::scaleFontValue);
		return (textHandler->Advance(str.c_str()) * Font::scaleFontValue);
		//return (textHandler->Advance(str.c_str()));
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
		//printf("(textHandler->LineHeight(" ") = %f Font::scaleFontValue = %f\n",textHandler->LineHeight(" "),Font::scaleFontValue);
		return (textHandler->LineHeight(" ") * Font::scaleFontValue);
	}
	else {
		return height;
	}
}

// ===============================================
//	class Font
// ===============================================

Font::Font(FontTextHandlerType type) {
	inited		= false;
	this->type	= fontTypeName;
	width		= 400;
	size 		= 10;
	textHandler = NULL;

#if defined(USE_FTGL) || defined(USE_FREETYPEGL)

	if(Font::forceLegacyFonts == false) {
		try {
#if defined(USE_FREETYPEGL)
			if(Font::forceFTGLFonts == false) {
				textHandler = NULL;
				textHandler = new TextFreetypeGL(type);
			}

			else
#endif
			{
				TextFTGL::faceResolution = Font::faceResolution;
				TextFTGL::langHeightText = Font::langHeightText;

				textHandler = NULL;
				textHandler = new TextFTGL(type);
				TextFTGL::faceResolution = Font::faceResolution;
				TextFTGL::langHeightText = Font::langHeightText;
			}

			metrics.setTextHandler(this->textHandler);
		}
		catch(exception &ex) {
			SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
			textHandler = NULL;
		}
	}

#endif
}

Font::~Font() {
	if(textHandler) {
		delete textHandler;
	}
	textHandler = NULL;
}

string Font::getType() const {
	return this->type;
}

void Font::setType(string typeX11, string typeGeneric) {
	if(textHandler) {
		try {
			this->type= typeGeneric;
			textHandler->init(typeGeneric,textHandler->GetFaceSize());
			metrics.setTextHandler(this->textHandler);
		}
		catch(exception &ex) {
			SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
			textHandler = NULL;
		}
	}
	if(textHandler == NULL) {
		this->type= typeX11;
	}
}

void Font::setWidth(int width) {
	this->width= width;
}

int Font::getWidth() const	{
	return width;
}

int Font::getSize() const	{
	if(textHandler) {
		return textHandler->GetFaceSize();
	}
	else {
		return size;
	}
}
void Font::setSize(int size)	{
	if(textHandler) {
		return textHandler->SetFaceSize(size);
	}
	else {
		this->size= size;
	}
}

// ===============================================
//	class Font2D
// ===============================================

Font2D::Font2D(FontTextHandlerType type) : Font(type) {
}

// ===============================================
//	class Font3D
// ===============================================

Font3D::Font3D(FontTextHandlerType type) : Font(type) {
	depth= 10.f;
}

}}//end namespace
