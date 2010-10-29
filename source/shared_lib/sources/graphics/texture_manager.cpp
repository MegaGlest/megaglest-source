// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martio Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "texture_manager.h"

#include <cstdlib>
#include <stdexcept>

#include "graphics_interface.h"
#include "graphics_factory.h"

#include "leak_dumper.h"

namespace Shared{ namespace Graphics{

// =====================================================
//	class TextureManager
// =====================================================

TextureManager::TextureManager(){
	textureFilter= Texture::fBilinear;
	maxAnisotropy= 1;
}

TextureManager::~TextureManager(){
	end();
}

void TextureManager::initTexture(Texture *texture) {
	if(texture != NULL) {
		texture->init(textureFilter, maxAnisotropy);
	}
}

void TextureManager::endTexture(Texture *texture,bool mustExistInList) {
	if(texture != NULL) {
		bool found = false;
		for(unsigned int idx = 0; idx < textures.size(); idx++) {
			Texture *curTexture = textures[idx];
			if(curTexture == texture) {
				found = true;
				textures.erase(textures.begin() + idx);
				break;
			}
		}
		if(found == false && mustExistInList == true) {
			throw std::runtime_error("found == false in endTexture");
		}
		texture->end();
		delete texture;
	}
}

void TextureManager::endLastTexture(bool mustExistInList) {
	bool found = false;
	if(textures.size() > 0) {
		found = true;
		int index = (int)textures.size()-1;
		Texture *curTexture = textures[index];
		textures.erase(textures.begin() + index);

		curTexture->end();
		delete curTexture;
	}
	if(found == false && mustExistInList == true) {
		throw std::runtime_error("found == false in endLastTexture");
	}
}

void TextureManager::init(bool forceInit) {
	for(unsigned int i=0; i<textures.size(); ++i){
		Texture *texture = textures[i];
		if(texture == NULL) {
			throw std::runtime_error("texture == NULL during init");
		}
		if(forceInit == true) {
			texture->reseInitState();
		}
		if(forceInit == true) {
			texture->end(false);
		}
		texture->init(textureFilter, maxAnisotropy);
	}
}

void TextureManager::end(){
	for(unsigned int i=0; i<textures.size(); ++i){
		if(textures[i] != NULL) {
			textures[i]->end();
			delete textures[i];
		}
	}
	textures.clear();
}

void TextureManager::setFilter(Texture::Filter textureFilter){
	this->textureFilter= textureFilter;
}

void TextureManager::setMaxAnisotropy(int maxAnisotropy){
	this->maxAnisotropy= maxAnisotropy;
}

Texture *TextureManager::getTexture(const string &path){
	for(unsigned int i=0; i<textures.size(); ++i){
		if(textures[i]->getPath()==path){
			return textures[i];
		}
	}
	return NULL;
}

Texture1D *TextureManager::newTexture1D(){
	Texture1D *texture1D= GraphicsInterface::getInstance().getFactory()->newTexture1D();
	textures.push_back(texture1D);

	return texture1D;
}

Texture2D *TextureManager::newTexture2D(){
	Texture2D *texture2D= GraphicsInterface::getInstance().getFactory()->newTexture2D();
	textures.push_back(texture2D);

	return texture2D;
}

Texture3D *TextureManager::newTexture3D(){
	Texture3D *texture3D= GraphicsInterface::getInstance().getFactory()->newTexture3D();
	textures.push_back(texture3D);

	return texture3D;
}


TextureCube *TextureManager::newTextureCube(){
	TextureCube *textureCube= GraphicsInterface::getInstance().getFactory()->newTextureCube();
	textures.push_back(textureCube);

	return textureCube;
}

}}//end namespace
