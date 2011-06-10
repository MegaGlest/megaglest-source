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

#ifdef USE_FTGL

//#include "gettext.h"

#include "font_textFTGL.h"

#include <stdexcept>
#include <sys/stat.h>
#include <FTGL/ftgl.h>

#ifdef HAVE_FONTCONFIG
#include <fontconfig/fontconfig.h>
#endif

#include "platform_common.h"
#include "util.h"
using namespace std;
using namespace Shared::Util;
using namespace Shared::PlatformCommon;

namespace Shared { namespace Graphics { namespace Gl {


//====================================================================
TextFTGL::TextFTGL(FontTextHandlerType type) : Text(type) {

	//setenv("MEGAGLEST_FONT","/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc",0);
	//setenv("MEGAGLEST_FONT","/usr/share/fonts/truetype/arphic/uming.ttc",0); // Chinese
	//setenv("MEGAGLEST_FONT","/usr/share/fonts/truetype/arphic/ukai.ttc",0); // Chinese
	//setenv("MEGAGLEST_FONT","/usr/share/fonts/truetype/ttf-sil-doulos/DoulosSILR.ttf",0); // Russian / Cyrillic
	//setenv("MEGAGLEST_FONT","/usr/share/fonts/truetype/ttf-sil-charis/CharisSILR.ttf",0); // Russian / Cyrillic
	//setenv("MEGAGLEST_FONT","/usr/share/fonts/truetype/ubuntu-font-family/Ubuntu-R.ttf",0); // Russian / Cyrillic
	//setenv("MEGAGLEST_FONT","/usr/share/fonts/truetype/takao/TakaoPGothic.ttf",0); // Japanese
	//setenv("MEGAGLEST_FONT","/usr/share/fonts/truetype/ttf-sil-scheherazade/ScheherazadeRegOT.ttf",0); // Arabic
	//setenv("MEGAGLEST_FONT","/usr/share/fonts/truetype/linux-libertine/LinLibertine_Re.ttf",0); // Hebrew
	//setenv("MEGAGLEST_FONT","/usr/share/fonts/truetype/unifont/unifont.ttf",0); // Czech?
	//setenv("MEGAGLEST_FONT","/usr/share/fonts/truetype/ttf-liberation/LiberationSans-Regular.ttf",0); // Czech?


	fontFile = findFont();
	//ftFont = new FTBufferFont(fontFile);
	//ftFont = new FTGLPixmapFont(fontFile);
	//ftFont = new FTGLExtrdFont(fontFile);
	//ftFont = new FTGLPolygonFont("/usr/share/fonts/truetype/arphic/uming.ttc");

	//ftFont = new FTGLPixmapFont("/usr/share/fonts/truetype/arphic/uming.ttc");
	if(type == ftht_2D) {
		ftFont = new FTGLPixmapFont(fontFile);
		//printf("2D font [%s]\n",fontFile);
	}
	else if(type == ftht_3D) {
		ftFont = new FTGLTextureFont(fontFile);
		//printf("3D font [%s]\n",fontFile);
	}
	else {
		throw runtime_error("font render type not set to a known value!");
	}

	if(ftFont->Error())	{
		printf("FTGL: error loading font: %s\n", fontFile);
		delete ftFont; ftFont = NULL;
		free((void*)fontFile);
		fontFile = NULL;
		throw runtime_error("FTGL: error loading font");
	}
	free((void*)fontFile);
	fontFile = NULL;

	ftFont->FaceSize(24);
	if(ftFont->Error())	{
		throw runtime_error("FTGL: error setting face size");
	}
	//ftFont->UseDisplayList(false);
	//ftFont->CharMap(ft_encoding_gb2312);
	//ftFont->CharMap(ft_encoding_big5);
	if(ftFont->CharMap(ft_encoding_unicode) == false) {
		throw runtime_error("FTGL: error setting encoding");
	}

	if(ftFont->Error())	{
		throw runtime_error("FTGL: error setting encoding");
	}
}

TextFTGL::~TextFTGL() {
	cleanupFont();
}

void TextFTGL::cleanupFont() {
	delete ftFont;
	ftFont = NULL;

	free((void*)fontFile);
	fontFile = NULL;
}

void TextFTGL::init(string fontName, int fontSize) {
	cleanupFont();
	fontFile = findFont(fontName.c_str());

	//ftFont = new FTBufferFont(fontFile);
	//ftFont = new FTGLPixmapFont(fontFile);
	//ftFont = new FTGLExtrdFont(fontFile);
	//ftFont = new FTGLPolygonFont("/usr/share/fonts/truetype/arphic/uming.ttc");
	//ftFont = new FTGLPixmapFont("/usr/share/fonts/truetype/arphic/uming.ttc");

	if(type == ftht_2D) {
		ftFont = new FTGLPixmapFont(fontFile);
		//printf("2D font [%s]\n",fontFile);
	}
	else if(type == ftht_3D) {
		ftFont = new FTGLTextureFont(fontFile);
		//printf("3D font [%s]\n",fontFile);
	}
	else {
		throw runtime_error("font render type not set to a known value!");
	}

	if(ftFont->Error())	{
		printf("FTGL: error loading font: %s\n", fontFile);
		delete ftFont; ftFont = NULL;
		free((void*)fontFile);
		fontFile = NULL;
		throw runtime_error("FTGL: error loading font");
	}
	free((void*)fontFile);
	fontFile = NULL;

	if(fontSize > 0) {
		ftFont->FaceSize(fontSize);
	}
	else {
		ftFont->FaceSize(24);
	}

	if(ftFont->Error())	{
		throw runtime_error("FTGL: error setting face size");
	}

	//ftFont->UseDisplayList(false);
	//ftFont->CharMap(ft_encoding_gb2312);
	//ftFont->CharMap(ft_encoding_big5);
	if(ftFont->CharMap(ft_encoding_unicode) == false) {
		throw runtime_error("FTGL: error setting encoding");
	}
	if(ftFont->Error())	{
		throw runtime_error("FTGL: error setting encoding");
	}
}

void TextFTGL::SetFaceSize(int value) {
	ftFont->FaceSize(value);
	if(ftFont->Error())	{
		throw runtime_error("FTGL: error setting face size");
	}
}
int TextFTGL::GetFaceSize() {
	return ftFont->FaceSize();
}

void TextFTGL::Render(const char* str, const int len) {
	/*
	  FTGL renders the whole string when len == 0
	  but we don't want any text rendered then.
	*/
	if(len != 0) {
		ftFont->Render(str, len);

		if(ftFont->Error())	{
			throw runtime_error("FTGL: error trying to render");
		}
	}
}

float TextFTGL::Advance(const char* str, const int len) {
	return ftFont->Advance(str, len);
}

float TextFTGL::LineHeight(const char* str, const int len) {
	return ftFont->LineHeight();
}

void TextFTGL::Render(const wchar_t* str, const int len) {
	/*
	  FTGL renders the whole string when len == 0
	  but we don't want any text rendered then.
	*/
	if(len != 0) {
		ftFont->Render(str, len);
	}
}

float TextFTGL::Advance(const wchar_t* str, const int len) {
	return ftFont->Advance(str, len);
}

float TextFTGL::LineHeight(const wchar_t* str, const int len) {
	return ftFont->LineHeight();
}

const char* TextFTGL::findFont(const char *firstFontToTry) {
	const char* font = NULL;
	const char* path = NULL;

	#define CHECK_FONT_PATH(filename) \
	{ \
		path = filename; \
		if( !font && path && fileExists(path) == true ) \
			font = strdup(path); \
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
	string defaultFont = data_path + "data/core/fonts/gothub__.ttf";
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

	return font;
}

}}}//end namespace

#endif // USE_FTGL
