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

#include "model_manager.h"

#include "graphics_interface.h"
#include "graphics_factory.h"
#include <cstdlib>
#include <stdexcept>

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
		if(models[i] != NULL) {
			models[i]->init();
		}
	}
} 

void ModelManager::end(){
	for(size_t i=0; i<models.size(); ++i){
		if(models[i] != NULL) {
			models[i]->end();
			delete models[i];
		}
	}
	models.clear();
}

void ModelManager::endModel(Model *model,bool mustExistInList) {
	if(model != NULL) {
		bool found = false;
		for(unsigned int idx = 0; idx < models.size(); idx++) {
			Model *curModel = models[idx];
			if(curModel == model) {
				found = true;
				models.erase(models.begin() + idx);
				break;
			}
		}

		if(found == false && mustExistInList == true) {
			throw std::runtime_error("found == false in endModel");
		}

		model->end();
		delete model;
	}
}

void ModelManager::endLastModel(bool mustExistInList) {
	bool found = false;
	if(models.size() > 0) {
		found = true;
		size_t index = models.size()-1;
		Model *curModel = models[index];
		models.erase(models.begin() + index);

		curModel->end();
		delete curModel;
	}
	if(found == false && mustExistInList == true) {
		throw std::runtime_error("found == false in endLastModel");
	}
}


}}//end namespace
