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

#ifndef _SHARED_GRAPHICS_MODELMANAGER_H_
#define _SHARED_GRAPHICS_MODELMANAGER_H_

#include "model.h"

#include <vector>

using namespace std;

namespace Shared{ namespace Graphics{

class TextureManager;

// =====================================================
//	class ModelManager
// =====================================================

class ModelManager{
protected:
	typedef vector<Model*> ModelContainer;

protected:
	ModelContainer models;
	TextureManager *textureManager;

public:
	ModelManager();
	virtual ~ModelManager();

	Model *newModel();

	void init();
	void end();

	void setTextureManager(TextureManager *textureManager)	{this->textureManager= textureManager;}
};

}}//end namespace

#endif
