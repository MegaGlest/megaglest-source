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

#include "model_manager.h"

#include "graphics_interface.h"
#include "graphics_factory.h"
#include "leak_dumper.h"

namespace Shared{ namespace Graphics{

// =====================================================
//	class ModelManager
// =====================================================

ModelManager::ModelManager(){
	textureManager= NULL;
}

ModelManager::~ModelManager(){
	end();
}

Model *ModelManager::newModel(){
	Model *model= GraphicsInterface::getInstance().getFactory()->newModel();
	model->setTextureManager(textureManager);
	models.push_back(model);
	return model;
}

void ModelManager::init(){
	for(size_t i=0; i<models.size(); ++i){
		models[i]->init();
	}
} 

void ModelManager::end(){
	for(size_t i=0; i<models.size(); ++i){
		models[i]->end();
		delete models[i];
	}
	models.clear();
}


}}//end namespace
