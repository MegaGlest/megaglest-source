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
#include "leak_dumper.h"

using namespace std;
using namespace Shared::Util;

namespace Shared{ namespace Graphics{

int Font::charCount= 256;
std::string Font::fontTypeName = "Times New Roman";

// =====================================================
//	class FontMetrics
// =====================================================

FontMetrics::FontMetrics() {
	widths= new float[Font::charCount];
	height= 0;

	for(int i=0; i < Font::charCount; ++i) {
		widths[i]= 0;
	}
}

FontMetrics::~FontMetrics() {
	delete [] widths;
	widths = NULL;
}

float FontMetrics::getTextWidth(const string &str) const {
	float width= 0.f;
	for(unsigned int i=0; i< str.size() && (int)i < Font::charCount; ++i){
		if(str[i] >= Font::charCount) {
			string sError = "str[i] >= Font::charCount, [" + str + "] i = " + intToStr(i);
			throw runtime_error(sError);
		}
		//Treat 2 byte characters as spaces
        if(str[i] < 0) {
            width+= (widths[87]); // This is the letter W which is a fairly wide character and good to use for spacing
            //i++;
        }
        else {
            width+= widths[str[i]];
        }
	}
	return width;
}

float FontMetrics::getHeight() const{
	return height;
}

// ===============================================
//	class Font
// ===============================================

Font::Font(){
	inited= false;
	type= fontTypeName;
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
