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

#ifndef _SHARED_GRAPHICS_TEXTUREMANAGER_H_
#define _SHARED_GRAPHICS_TEXTUREMANAGER_H_

#include <vector>
#include "texture.h"
#include "leak_dumper.h"

using std::vector;

namespace Shared{ namespace Graphics{

// =====================================================
//	class TextureManager
// =====================================================

//manages textures, creation on request and deletion on destruction
class TextureManager{
protected:
	typedef vector<Texture*> TextureContainer;
	
protected:
	TextureContainer textures;
	
	Texture::Filter textureFilter;
	int maxAnisotropy;

public:
	TextureManager();
	~TextureManager();
	void init(bool forceInit=false);
	void end();

	void setFilter(Texture::Filter textureFilter);
	void setMaxAnisotropy(int maxAnisotropy);
	void initTexture(Texture *texture);
	void endTexture(Texture *texture,bool mustExistInList=false);
	void endLastTexture(bool mustExistInList=false);
	void reinitTextures();

	Texture *getTexture(const string &path);
	Texture1D *newTexture1D();
	Texture2D *newTexture2D();
	Texture3D *newTexture3D();
	TextureCube *newTextureCube();
};


}}//end namespace

#endif
