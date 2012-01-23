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

#ifdef USE_FREETYPEGL

#include "font_text_freetypegl.h"
#include "vector.h"

#include <stdexcept>
#include <sys/stat.h>

#ifdef HAVE_FONTCONFIG
#include <fontconfig/fontconfig.h>
#endif

#include "platform_common.h"
#include "util.h"
#include "font.h"

using namespace std;
using namespace Shared::Util;
using namespace Shared::PlatformCommon;

namespace Shared { namespace Graphics { namespace Gl {

//====================================================================
TextFreetypeGL::TextFreetypeGL(FontTextHandlerType type) : Text(type) {
	buffer=NULL;
	atlas=NULL;
    font=NULL;
    manager=NULL;

	init("", "", 24);
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

void TextFreetypeGL::init(string fontName, string fontFamilyName, int fontSize) {
	cleanupFont();
	this->fontName = fontName;
	this->fontFamilyName = fontFamilyName;
	this->fontFile = findFont(this->fontName.c_str(),this->fontFamilyName.c_str());
	this->fontFaceSize = fontSize;

    const wchar_t *cache = L" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";

    this->manager = font_manager_new( 512, 512, 1 );
    this->atlas   = texture_atlas_new( 512, 512, 1 );
    this->buffer  = vertex_buffer_new( "v3f:t2f:c4f" );

	//font = texture_font_new( atlas, "Verdana", minsize );
    this->font 	  = texture_font_new( atlas, fontFile, (float)fontFaceSize );
	//font = texture_font_new( atlas, font_manager_match_description( 0, "Verdana", minsize, bold, italic ), minsize );

	//int missed = texture_font_cache_glyphs( font, cache );
    texture_font_cache_glyphs( font, cache );

	free((void*)this->fontFile);
	this->fontFile = NULL;
}

void TextFreetypeGL::SetFaceSize(int value) {
	this->fontFaceSize = value;
	init(this->fontName,this->fontFamilyName,this->fontFaceSize);
}

int TextFreetypeGL::GetFaceSize() {
	return this->fontFaceSize;
}

void TextFreetypeGL::Render(const char* str, const int len) {
	if(str == NULL) {
		return;
	}

	//printf("Render TextFreetypeGL\n");
    //float currentColor[4] = { 0,0,0,1 };
	Vec4f currentColor;
    glGetFloatv(GL_CURRENT_COLOR,currentColor.ptr());

	if(lastTextRendered != str || lastTextColorRendered != currentColor) {
	    Pen pen ;
	    pen.x = 0; pen.y = 0;

		Markup markup = { 0, (float)this->fontFaceSize, 0, 0, 0.0, 0.0,
						  {currentColor.x,currentColor.y,currentColor.z,currentColor.w}, {0,0,0,0},
						  0, {0,0,0,1}, 0, {0,0,0,1},
						  0, {0,0,0,1}, 0, {0,0,0,1}, 0 };

	    // Add glyph one by one to the vertex buffer
	    // Expand totalBox by each glyph in string

		vertex_buffer_clear( this->buffer );

		// for multibyte - we can't rely on sizeof(T) == character
		FreetypeGLUnicodeStringItr<unsigned char> ustr((const unsigned char *)str);

		for(int i = 0; (len < 0 && *ustr) || (len >= 0 && i < len); i++) {
			unsigned int prevChar = (i > 0 ? *ustr-1 : 0);
			unsigned int thisChar = *ustr++;
			//unsigned int nextChar = *ustr;

			// Get glyph (build it if needed
			TextureGlyph *glyph = texture_font_get_glyph( this->font, thisChar );

			// Take kerning into account if necessary
			float kx = texture_glyph_get_kerning( glyph, prevChar );

			// Add glyph to the vertex buffer
			texture_glyph_add_to_vertex_buffer( glyph, this->buffer, &markup, &pen, (int)kx );
		}

		lastTextRendered = str;
		lastTextColorRendered = currentColor;
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
		//result += glyph->width;
		result += glyph->advance_x;
	}
	return result;
}

float TextFreetypeGL::Advance(const char* str, const int len) {
	float result = 0;

//	for(unsigned int i = 0; i < strlen(str); ++i) {
//		TextureGlyph *glyph = texture_font_get_glyph( font, str[i] );
//		//result += glyph->width;
//		result += glyph->advance_x;
//	}
    // for multibyte - we can't rely on sizeof(T) == character
	FreetypeGLUnicodeStringItr<unsigned char> ustr((const unsigned char *)str);

    for(int i = 0; (len < 0 && *ustr) || (len >= 0 && i < len); i++) {
    	//unsigned int prevChar = (i > 0 ? *ustr-1 : 0);
    	unsigned int thisChar = *ustr++;
    	//unsigned int nextChar = *ustr;

		// Get glyph (build it if needed
		TextureGlyph *glyph = texture_font_get_glyph( this->font, thisChar );
		result += glyph->advance_x;
    }
	return result;
}

float TextFreetypeGL::LineHeight(const char* str, const int len) {
	//Font::scaleFontValueCenterHFactor = 30.0;
	//Font::scaleFontValue = 1.0;

	float result = 0;

	//result = font->height - font->linegap;
	result = font->ascender - font->descender - font->linegap;
	//printf("#2 LineHeight [%s] height = %f linegap = %f ascender = %f descender = %f\n",str,font->height,font->linegap,font->ascender,font->descender);

	//result += (result * Font::scaleFontValue);

    // for multibyte - we can't rely on sizeof(T) == character
//	FreetypeGLUnicodeStringItr<unsigned char> ustr((const unsigned char *)str);
//
//	int i = 0;
//    if((len < 0 && *ustr) || (len >= 0 && i < len)) {
//    	unsigned int prevChar = (i > 0 ? *ustr-1 : 0);
//    	unsigned int thisChar = *ustr++;
//    	unsigned int nextChar = *ustr;
//
//		TextureGlyph *glyph = texture_font_get_glyph( font, thisChar );
//		//result = (float)glyph->height;
//
//		printf("#1 LineHeight [%s] result = %f glyph->height = %d glyph->advance_y = %f\n",str,result,glyph->height,glyph->advance_y);
//	}

//    if(str[0] == '\n') {
//		TextureGlyph *glyph2 = texture_font_get_glyph( font, str[0] );
//		float result2 = (float)glyph2->height;
//
//		printf("#2 LineHeight [%s] result = %f result2 = %f\n",str,result,result2);
//    }

	return result;
}

float TextFreetypeGL::LineHeight(const wchar_t* str, const int len) {
	//Font::scaleFontValueCenterHFactor = 2.0;

	float result = 0;

	result = font->height - font->linegap;

//	if(wcslen(str) > 0) {
//		TextureGlyph *glyph = texture_font_get_glyph( font, str[0] );
//		result = (float)glyph->height;
//		//result = (float)glyph->advance_y;
//	}
	return result;
}

}}}//end namespace

#endif // USE_FTGL
