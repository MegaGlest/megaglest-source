// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#include "object_type.h"

#include "renderer.h"
#include "leak_dumper.h"

namespace Glest{ namespace Game{

// =====================================================
// 	class ObjectType
// =====================================================

void ObjectType::init(int modelCount, int objectClass, bool walkable, int height) {
	models.reserve(modelCount);
	this->objectClass= objectClass;
	this->walkable= walkable;
	this->height = height;
}

void ObjectType::loadModel(const string &path){
	Model *model= Renderer::getInstance().newModel(rsGame);
	model->load(path);
	color= Vec3f(0.f);
	if(model->getMeshCount()>0 && model->getMesh(0)->getTexture(0) != NULL) {
		const Pixmap2D *p= model->getMesh(0)->getTexture(0)->getPixmapConst();
		color= p->getPixel3f(p->getW()/2, p->getH()/2);
	}
	models.push_back(model);
}

void ObjectType::deletePixels() {
	for(int i = 0; i < models.size(); ++i) {
		Model *model = models[i];
		if(model != NULL) {
			model->deletePixels();
		}
	}
}

}}//end namespace
