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
using namespace Shared::Graphics::Gl;
#endif

#ifdef USE_FREETYPEGL
#include "font_text_freetypegl.h"
#endif

#ifdef HAVE_FONTCONFIG
#include <fontconfig/fontconfig.h>
#endif

#include "util.h"
#include "platform_common.h"

#include "leak_dumper.h"

using namespace std;
using namespace Shared::Util;
using namespace Shared::PlatformCommon;

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

float FontMetrics::getHeight(const string &str) const {
	if(textHandler != NULL) {
		//printf("(textHandler->LineHeight(" ") = %f Font::scaleFontValue = %f\n",textHandler->LineHeight(" "),Font::scaleFontValue);
		//return (textHandler->LineHeight(str.c_str()) * Font::scaleFontValue);
		return (textHandler->LineHeight(str.c_str()));
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
			//if(Font::forceFTGLFonts == false) {
			//	textHandler = NULL;
			//	textHandler = new TextFreetypeGL(type);
				//printf("TextFreetypeGL is ON\n");
			//}

			//else
#endif
			{
				TextFTGL::faceResolution = Font::faceResolution;
				TextFTGL::langHeightText = Font::langHeightText;

				textHandler = NULL;
				textHandler = new TextFTGL(type);
				//printf("TextFTGL is ON\n");

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

const char* findFont(const char *firstFontToTry) {
	const char* font = NULL;
	const char* path = NULL;

	#define CHECK_FONT_PATH(filename) \
	{ \
		path = filename; \
		if( !font && path && fileExists(path) == true ) \
			font = strdup(path); \
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Found font file [%s]\n",font); \
	}

	string tryFont = "";
	if(firstFontToTry) {
		tryFont = firstFontToTry;
		#ifdef WIN32
		  replaceAll(tryFont, "/", "\\");
		#endif

		CHECK_FONT_PATH(tryFont.c_str())
	}

	// Get user-specified font path
	if(getenv("MEGAGLEST_FONT") != NULL) {
		tryFont = getenv("MEGAGLEST_FONT");
		#ifdef WIN32
		  replaceAll(tryFont, "/", "\\");
		#endif

		CHECK_FONT_PATH(tryFont.c_str())
	}

	string data_path = Text::DEFAULT_FONT_PATH;
	string defaultFont = data_path + "data/core/fonts/LinBiolinum_RB.ttf";//LinBiolinum_Re-0.6.4.ttf
	tryFont = defaultFont;
	#ifdef WIN32
	  replaceAll(tryFont, "/", "\\");
	#endif
	CHECK_FONT_PATH(tryFont.c_str())

#ifdef FONT_PATH
	// Get distro-specified font path
	CHECK_FONT_PATH(FONT_PATH)
#endif

#ifdef HAVE_FONTCONFIG
	// Get default font via fontconfig
	if( !font && FcInit() )	{
		FcResult result;
		FcFontSet *fs;
		FcPattern* pat;
		FcPattern *match;

		/*
		TRANSLATORS: If using the FTGL backend, this should be the font
		name of a font that contains all the Unicode characters in use in
		your translation.
		*/
		pat = FcNameParse((FcChar8 *)"Gothic Uralic");
		FcConfigSubstitute(0, pat, FcMatchPattern);

		FcPatternDel(pat, FC_WEIGHT);
		FcPatternAddInteger(pat, FC_WEIGHT, FC_WEIGHT_BOLD);

		FcDefaultSubstitute(pat);
		fs = FcFontSetCreate();
		match = FcFontMatch(0, pat, &result);

		if (match) FcFontSetAdd(fs, match);
		if (pat) FcPatternDestroy(pat);
		if(fs) {
			FcChar8* file;
			if( FcPatternGetString (fs->fonts[0], FC_FILE, 0, &file) == FcResultMatch ) {
				CHECK_FONT_PATH((const char*)file)
			}
			FcFontSetDestroy(fs);
		}
		FcFini();
	}
#endif

	CHECK_FONT_PATH("/usr/share/fonts/truetype/uralic/gothub__.ttf")

	// Check a couple of common paths for Gothic Uralic/bold as a last resort
	// Debian
	/*
	TRANSLATORS: If using the FTGL backend, this should be the path of a bold
	font that contains all the Unicode characters in use in	your translation.
	If the font is available in Debian it should be the Debian path.
	*/
	CHECK_FONT_PATH("/usr/share/fonts/truetype/uralic/gothub__.ttf")
	/*
	TRANSLATORS: If using the FTGL backend, this should be the path of a
	font that contains all the Unicode characters in use in	your translation.
	If the font is available in Debian it should be the Debian path.
	*/
	CHECK_FONT_PATH("/usr/share/fonts/truetype/uralic/gothu___.ttf")
	// Mandrake
	/*
	TRANSLATORS: If using the FTGL backend, this should be the path of a bold
	font that contains all the Unicode characters in use in	your translation.
	If the font is available in Mandrake it should be the Mandrake path.
	*/
	CHECK_FONT_PATH("/usr/share/fonts/TTF/uralic/GOTHUB__.TTF")
	/*
	TRANSLATORS: If using the FTGL backend, this should be the path of a
	font that contains all the Unicode characters in use in	your translation.
	If the font is available in Mandrake it should be the Mandrake path.
	*/
	CHECK_FONT_PATH("/usr/share/fonts/TTF/uralic/GOTHU___.TTF")

	// Check the non-translated versions of the above
	CHECK_FONT_PATH("/usr/share/fonts/truetype/uralic/gothub__.ttf")
	CHECK_FONT_PATH("/usr/share/fonts/truetype/uralic/gothu___.ttf")
	CHECK_FONT_PATH("/usr/share/fonts/TTF/uralic/GOTHUB__.TTF")
	CHECK_FONT_PATH("/usr/share/fonts/TTF/uralic/GOTHU___.TTF")

	CHECK_FONT_PATH("/usr/share/fonts/truetype/linux-libertine/LinLibertine_Re.ttf")

	CHECK_FONT_PATH("/usr/share/fonts/truetype/freefont/FreeSerif.ttf")
	CHECK_FONT_PATH("/usr/share/fonts/truetype/freefont/FreeSans.ttf")
	CHECK_FONT_PATH("/usr/share/fonts/truetype/freefont/FreeMono.ttf")

#ifdef _WIN32
	CHECK_FONT_PATH("c:\\windows\\fonts\\verdana.ttf")
	CHECK_FONT_PATH("c:\\windows\\fonts\\tahoma.ttf")
	CHECK_FONT_PATH("c:\\windows\\fonts\\arial.ttf")
	CHECK_FONT_PATH("\\windows\\fonts\\arial.ttf")
#endif

	return font;
}


}}//end namespace
