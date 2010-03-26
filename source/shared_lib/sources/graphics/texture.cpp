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

#include "texture.h"

#include "leak_dumper.h"

namespace Shared{ namespace Graphics{

// =====================================================
//	class Texture
// =====================================================

const int Texture::defaultSize= 256;
const int Texture::defaultComponents = 4;

Texture::Texture(){
	mipmap= true;
	pixmapInit= true;
	wrapMode= wmRepeat;
	format= fAuto;

	inited= false;
}

// =====================================================
//	class Texture1D
// =====================================================

void Texture1D::load(const string &path){
	this->path= path;
	if (pixmap.getComponents() == -1) { //TODO: look where you really need that
		pixmap.init(defaultComponents);
	}
	pixmap.load(path);
}

// =====================================================
//	class Texture2D
// =====================================================

void Texture2D::load(const string &path){
	this->path= path;
	if (pixmap.getComponents() == -1) {
		pixmap.init(defaultComponents);
	}
	pixmap.load(path);
}

// =====================================================
//	class Texture3D
// =====================================================

void Texture3D::loadSlice(const string &path, int slice){
	this->path= path;
	if (pixmap.getComponents() == -1) {
		pixmap.init(defaultComponents);
	}
	pixmap.loadSlice(path, slice);
}

// =====================================================
//	class TextureCube
// =====================================================

void TextureCube::loadFace(const string &path, int face){
	this->path= path;
	if (pixmap.getFace(0)->getComponents() == -1) {
		pixmap.init(defaultComponents);
	}
	pixmap.loadFace(path, face);
}

}}//end namespace
