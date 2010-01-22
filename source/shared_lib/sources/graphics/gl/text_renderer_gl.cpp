// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#include "text_renderer_gl.h"

#include "opengl.h"
#include "font_gl.h"
#include "leak_dumper.h"

namespace Shared{ namespace Graphics{ namespace Gl{

// =====================================================
//	class TextRenderer2DGl
// =====================================================

TextRenderer2DGl::TextRenderer2DGl(){
	rendering= false;
}

void TextRenderer2DGl::begin(const Font2D *font){
	assert(!rendering);
	rendering= true;
	
	this->font= static_cast<const Font2DGl*>(font);
}

void TextRenderer2DGl::render(const string &text, int x, int y, bool centered){
	assert(rendering);
	
	assertGl();

	int line=0;
	int size= font->getSize();
    const unsigned char *utext= reinterpret_cast<const unsigned char*>(text.c_str());

	Vec2f rasterPos;
	const FontMetrics *metrics= font->getMetrics();
	if(centered){
		rasterPos.x= x-metrics->getTextWidth(text)/2.f;
		rasterPos.y= y+metrics->getHeight()/2.f;
	}
	else{
		rasterPos= Vec2f(static_cast<float>(x), static_cast<float>(y));
	}
	glRasterPos2f(rasterPos.x, rasterPos.y);

	for (int i=0; utext[i]!='\0'; ++i) {
		switch(utext[i]){
		case '\t':
			rasterPos= Vec2f((rasterPos.x/size+3.f)*size, y-(size+1.f)*line);
			glRasterPos2f(rasterPos.x, rasterPos.y);
			break;
		case '\n':
			line++;
			rasterPos= Vec2f(static_cast<float>(x), y-(metrics->getHeight()*2.f)*line);
			glRasterPos2f(rasterPos.x, rasterPos.y);
			break;
		default:
			glCallList(font->getHandle()+utext[i]);
		}
	}

	assertGl();
}

void TextRenderer2DGl::end(){
	assert(rendering);
	rendering= false;
}

// =====================================================
//	class TextRenderer3DGl
// =====================================================

TextRenderer3DGl::TextRenderer3DGl(){
	rendering= false;
}

void TextRenderer3DGl::begin(const Font3D *font){
	assert(!rendering);
	rendering= true;
	
	this->font= static_cast<const Font3DGl*>(font);

	assertGl();

	//load color
	glPushAttrib(GL_TRANSFORM_BIT);

	assertGl();
}

void TextRenderer3DGl::render(const string &text, float  x, float y, float size, bool centered){
	assert(rendering);
	
	assertGl();

	const unsigned char *utext= reinterpret_cast<const unsigned char*>(text.c_str());

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glPushAttrib(GL_POLYGON_BIT);
	float scale= size/10.f;
	if(centered){
		const FontMetrics *metrics= font->getMetrics();
		glTranslatef(x-scale*metrics->getTextWidth(text)/2.f, y-scale*metrics->getHeight()/2.f, 0);
	}
	else{
		glTranslatef(x-scale, y-scale, 0);
	}
	glScalef(scale, scale, scale);
                     
	for (int i=0; utext[i]!='\0'; ++i) {
		glCallList(font->getHandle()+utext[i]);
	}

	glPopMatrix();
	glPopAttrib();

	assertGl();
}

void TextRenderer3DGl::end(){
	assert(rendering);
	rendering= false;

	assertGl();

	glPopAttrib();

	assertGl();
}

}}}//end namespace
