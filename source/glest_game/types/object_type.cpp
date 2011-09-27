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
//	modeltypes.reserve(modelCount);
	this->objectClass= objectClass;
	this->walkable= walkable;
	this->height = height;
}

ObjectType::~ObjectType(){
	while(!(modeltypes.empty())){
		delete modeltypes.back();
		modeltypes.pop_back();
		//Logger::getInstance().add("ObjectType", true);
	}
}

TilesetModelType* ObjectType::loadModel(const string &path, std::map<string,vector<pair<string, string> > > *loadedFileList,
		string parentLoader) {
	Model *model= Renderer::getInstance().newModel(rsGame);
	if(model) {
		model->load(path, false, loadedFileList, &parentLoader);
	}
	color= Vec3f(0.f);
	if(model && model->getMeshCount()>0 && model->getMesh(0)->getTexture(0) != NULL) {
		const Pixmap2D *p= model->getMesh(0)->getTexture(0)->getPixmapConst();
		color= p->getPixel3f(p->getW()/2, p->getH()/2);
	}
	TilesetModelType *modelType=new TilesetModelType();
	modelType->setModel(model);
	modeltypes.push_back(modelType);
	return modelType;
}

void ObjectType::deletePixels() {
	for(int i = 0; i < modeltypes.size(); ++i) {
		TilesetModelType *model = modeltypes[i];
		if(model->getModel() != NULL) {
			model->getModel()->deletePixels();
		}
	}
}

}}//end namespace
