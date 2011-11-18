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

#ifdef USE_FREETYPEGL

#include "font_text_freetypegl.h"
#include "vector.h"

//#include "opengl.h"
#include <stdexcept>
#include <sys/stat.h>

#ifdef HAVE_FONTCONFIG
#include <fontconfig/fontconfig.h>
#endif

#include "platform_common.h"
#include "util.h"

using namespace std;
using namespace Shared::Util;
using namespace Shared::PlatformCommon;

namespace Shared { namespace Graphics { namespace Gl {


//string TextFreetypeGL::langHeightText = "yW";
//int TextFreetypeGL::faceResolution 	= 72;

//====================================================================
TextFreetypeGL::TextFreetypeGL(FontTextHandlerType type) : Text(type) {
	buffer=NULL;
	atlas=NULL;
    font=NULL;
    manager=NULL;

	init("", 24);
}

TextFreetypeGL::~TextFreetypeGL() {
	cleanupFont();
}

void TextFreetypeGL::cleanupFont() {
	if(font) {
		texture_font_delete(font);
		font = NULL;
	}
}

void TextFreetypeGL::init(string fontName, int fontSize) {
	cleanupFont();
	this->fontName = fontName;
	this->fontFile = findFont(this->fontName.c_str());
	this->fontFaceSize = fontSize;

    const wchar_t *cache = L" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";

    this->manager = font_manager_new( 512, 512, 1 );
    this->atlas   = texture_atlas_new( 512, 512, 1 );
    this->buffer  = vertex_buffer_new( "v3f:t2f:c4f" );

	//font = texture_font_new( atlas, "Verdana", minsize );
    this->font 	  = texture_font_new( atlas, fontFile, (float)fontFaceSize );
	//font = texture_font_new( atlas, font_manager_match_description( 0, "Verdana", minsize, bold, italic ), minsize );

	int missed = texture_font_cache_glyphs( font, cache );

	free((void*)this->fontFile);
	this->fontFile = NULL;
}

void TextFreetypeGL::SetFaceSize(int value) {
	this->fontFaceSize = value;
	init(this->fontName,this->fontFaceSize);
}

int TextFreetypeGL::GetFaceSize() {
	return this->fontFaceSize;
}

void TextFreetypeGL::Render(const char* str, const int len) {
	if(str == NULL) {
		return;
	}

    Pen pen ;
    pen.x = 0; pen.y = 0;

    vertex_buffer_clear( this->buffer );

    float currentColor[4] = { 0,0,0,1 };
    glGetFloatv(GL_CURRENT_COLOR,currentColor);

	Markup markup = { 0, (float)this->fontFaceSize, 0, 0, 0.0, 0.0,
					  {currentColor[0],currentColor[1],currentColor[2],currentColor[3]}, {0,0,0,0},
					  0, {0,0,0,1}, 0, {0,0,0,1},
					  0, {0,0,0,1}, 0, {0,0,0,1}, 0 };

    // Add glyph one by one to the vertex buffer
    // Expand totalBox by each glyph in string

    // for multibyte - we can't rely on sizeof(T) == character
	FreetypeGLUnicodeStringItr<unsigned char> ustr((const unsigned char *)str);

    for(int i = 0; (len < 0 && *ustr) || (len >= 0 && i < len); i++) {
    	unsigned int prevChar = (i > 0 ? *ustr-1 : 0);
    	unsigned int thisChar = *ustr++;
    	unsigned int nextChar = *ustr;

		// Get glyph (build it if needed
		TextureGlyph *glyph = texture_font_get_glyph( this->font, thisChar );

		// Take kerning into account if necessary
		float kx = texture_glyph_get_kerning( glyph, prevChar );

		// Add glyph to the vertex buffer
		texture_glyph_add_to_vertex_buffer( glyph, this->buffer, &markup, &pen, (int)kx );
	}

    //glBindTexture( GL_TEXTURE_2D, manager->atlas->texid );
	glBindTexture( GL_TEXTURE_2D, this->atlas->texid );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable( GL_TEXTURE_2D );

    vertex_buffer_render( this->buffer, GL_TRIANGLES, "vtc" );

    glDisable( GL_TEXTURE_2D );
    glDisable( GL_BLEND );

    glBindTexture(GL_TEXTURE_2D, 0);
}

void TextFreetypeGL::Render(const wchar_t* str, const int len) {
	if(str == NULL) {
		return;
	}

	const wchar_t *text = str;
    Pen pen ;
    pen.x = 0; pen.y = 0;

    vertex_buffer_clear( this->buffer );

	GLfloat currentColor[4] = { 0,0,0,1 };
    glGetFloatv(GL_CURRENT_COLOR,currentColor);

	Markup markup = { 0, (float)this->fontFaceSize, 0, 0, 0.0, 0.0,
					  {currentColor[0],currentColor[1],currentColor[2],currentColor[3]}, {0,0,0,0},
					  0, {0,0,0,1}, 0, {0,0,0,1},
					  0, {0,0,0,1}, 0, {0,0,0,1}, 0 };

    // Add glyph one by one to the vertex buffer
	for( size_t i = 0; i < wcslen(text); ++i ) {
		// Get glyph (build it if needed
		TextureGlyph *glyph = texture_font_get_glyph( this->font, text[i] );

		// Take kerning into account if necessary
		float kx = texture_glyph_get_kerning( glyph, text[i-1] );

		// Add glyph to the vertex buffer
		texture_glyph_add_to_vertex_buffer( glyph, this->buffer, &markup, &pen, (int)kx );
	}

    //glBindTexture( GL_TEXTURE_2D, manager->atlas->texid );
	glBindTexture( GL_TEXTURE_2D, this->atlas->texid );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable( GL_TEXTURE_2D );
    vertex_buffer_render( this->buffer, GL_TRIANGLES, "vtc" );
}

float TextFreetypeGL::Advance(const wchar_t* str, const int len) {
	float result = 0;

	for(unsigned int i = 0; i < wcslen(str); ++i) {
		TextureGlyph *glyph = texture_font_get_glyph( font, str[i] );
		result += glyph->width;
	}
	return result;
}

float TextFreetypeGL::Advance(const char* str, const int len) {
	float result = 0;

	for(unsigned int i = 0; i < strlen(str); ++i) {
		TextureGlyph *glyph = texture_font_get_glyph( font, str[i] );
		result += glyph->width;
	}
	return result;
}

float TextFreetypeGL::LineHeight(const char* str, const int len) {
	float result = 0;
	if(strlen(str) > 0) {
		TextureGlyph *glyph = texture_font_get_glyph( font, str[0] );
		result = (float)glyph->height;
	}
	return result;
}

float TextFreetypeGL::LineHeight(const wchar_t* str, const int len) {
	float result = 0;
	if(wcslen(str) > 0) {
		TextureGlyph *glyph = texture_font_get_glyph( font, str[0] );
		result = (float)glyph->height;
	}
	return result;
}

const char* TextFreetypeGL::findFont(const char *firstFontToTry) {
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

	return font;
}

}}}//end namespace

#endif // USE_FTGL
