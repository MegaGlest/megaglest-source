// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2007 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#include "font.h"

#include "leak_dumper.h"

namespace Shared{ namespace Graphics{

// =====================================================
//	class FontMetrics
// =====================================================

FontMetrics::FontMetrics(){
	widths= new float[Font::charCount];
	height= 0;

	for(int i=0; i<Font::charCount; ++i){
		widths[i]= 0;
	}	
}

FontMetrics::~FontMetrics(){
	delete [] widths;
}

float FontMetrics::getTextWidth(const string &str) const{
	float width= 0.f;
	for(int i=0; i<str.size(); ++i){
		width+= widths[str[i]];
	}
	return width;
}

float FontMetrics::getHeight() const{
	return height;
}

// ===============================================
//	class Font
// ===============================================

const int Font::charCount= 256;

Font::Font(){
	inited= false;
	type= "Times New Roman";
	width= 400;
}

// ===============================================
//	class Font2D
// ===============================================

Font2D::Font2D(){
	size= 10;
}

// ===============================================
//	class Font3D
// ===============================================

Font3D::Font3D(){
	depth= 10.f;
}

}}//end namespace
