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
#include "util.h"

#include "leak_dumper.h"

using namespace Shared::Util;

namespace Shared{ namespace Graphics{

// =====================================================
//	class Texture
// =====================================================

const int Texture::defaultSize       = 256;
const int Texture::defaultComponents = 4;
bool Texture::useTextureCompression  = false;

Texture::Texture() {
	assert(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false);

	mipmap= true;
	pixmapInit= true;
	wrapMode= wmRepeat;
	format= fAuto;

	inited= false;
	forceCompressionDisabled=false;
}

// =====================================================
//	class Texture1D
// =====================================================

void Texture1D::load(const string &path){
	this->path= path;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] this->path = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->path.c_str());

	if (pixmap.getComponents() == -1) { //TODO: look where you really need that
		pixmap.init(defaultComponents);
	}
	pixmap.load(path);
	this->path= path;
}

string Texture1D::getPath() const {
	return (pixmap.getPath() != "" ? pixmap.getPath() : path);
}

void Texture1D::deletePixels() {
	//printf("+++> Texture1D pixmap deletion for [%s]\n",getPath().c_str());
	pixmap.deletePixels();
}

// =====================================================
//	class Texture2D
// =====================================================

void Texture2D::load(const string &path){
	this->path= path;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] this->path = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->path.c_str());

	if (pixmap.getComponents() == -1) {
		pixmap.init(defaultComponents);
	}
	pixmap.load(path);
	this->path= path;
}

string Texture2D::getPath() const {
	return (pixmap.getPath() != "" ? pixmap.getPath() : path);
}

void Texture2D::deletePixels() {
	//printf("+++> Texture2D pixmap deletion for [%s]\n",getPath().c_str());
	pixmap.deletePixels();
}

// =====================================================
//	class Texture3D
// =====================================================

void Texture3D::loadSlice(const string &path, int slice){
	this->path= path;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] this->path = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->path.c_str());

	if (pixmap.getComponents() == -1) {
		pixmap.init(defaultComponents);
	}
	pixmap.loadSlice(path, slice);
	this->path= path;
}

string Texture3D::getPath() const {
	return (pixmap.getPath() != "" ? pixmap.getPath() : path);
}

void Texture3D::deletePixels() {
	//printf("+++> Texture3D pixmap deletion for [%s]\n",getPath().c_str());
	pixmap.deletePixels();
}

// =====================================================
//	class TextureCube
// =====================================================

void TextureCube::loadFace(const string &path, int face){
	this->path= path;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] this->path = [%s]\n",__FILE__,__FUNCTION__,__LINE__,this->path.c_str());

	if (pixmap.getFace(0)->getComponents() == -1) {
		pixmap.init(defaultComponents);
	}
	pixmap.loadFace(path, face);
	this->path= path;
}

string TextureCube::getPath() const {
	string result = "";
	for(int i = 0; i < 6; ++i) {
		if(pixmap.getPath(i) != "") {
			if(result != "") {
				result += ",";
			}
			result += pixmap.getPath(i);
		}
	}
	if(result == "") {
		result = path;
	}
	return result;
}

void TextureCube::deletePixels() {
	//printf("+++> TextureCube pixmap deletion for [%s]\n",getPath().c_str());
	pixmap.deletePixels();
}

}}//end namespace
