// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martio Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#include "object.h"

#include "faction_type.h"
#include "config.h"
#include "tech_tree.h"
#include "resource.h"
#include "upgrade.h"
#include "object_type.h"
#include "resource.h"
#include "util.h"
#include "randomgen.h"
#include "renderer.h"
#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest{ namespace Game{

ObjectStateInterface *Object::stateCallback=NULL;

// =====================================================
// 	class Object
// =====================================================

Object::Object(ObjectType *objectType, const Vec3f &pos, const Vec2i &mapPos) {
	RandomGen random;

	random.init(static_cast<int>(pos.x*pos.z));
	this->lastRenderFrame = 0;
	this->objectType= objectType;
	resource= NULL;
	this->mapPos = mapPos;
	this->pos= pos + Vec3f(random.randRange(-0.6f, 0.6f), 0.0f, random.randRange(-0.6f, 0.6f));
	rotation= random.randRange(0.f, 360.f);
	if(objectType!=NULL){
		variation = random.randRange(0, objectType->getModelCount()-1);
		TilesetModelType *tmt=objectType->getTilesetModelType(variation);
		if(tmt->getRotationAllowed()!=true){
			rotation=0;
		}
	}
	visible=false;

}

Object::~Object(){
	Renderer &renderer= Renderer::getInstance();
	// fade(and by this remove) all unit particle systems
	while(unitParticleSystems.empty() == false) {
		unitParticleSystems.back()->fade();
		unitParticleSystems.pop_back();
	}
	renderer.removeObjectFromQuadCache(this);
	if(stateCallback) {
		stateCallback->removingObjectEvent(this);
	}
	delete resource;
}

void Object::end(){
	// set Objects to fading and remove them from list.
	// its needed because otherwise they will be accessed from the destructor
	while(unitParticleSystems.empty() == false) {
		unitParticleSystems.back()->fade();
		unitParticleSystems.pop_back();
	}
}

void Object::initParticles(){
	if(this->objectType==NULL) return;
	if(this->objectType->getTilesetModelType(variation)->hasParticles()){
		ModelParticleSystemTypes *particleTypes= this->objectType->getTilesetModelType(variation)->getParticleTypes();
		initParticlesFromTypes(particleTypes);
	}
}

void Object::initParticlesFromTypes(const ModelParticleSystemTypes *particleTypes){
	if(Config::getInstance().getBool("TilesetParticles", "true") && (particleTypes->empty() == false)
	        && (unitParticleSystems.empty() == true)){
		for(ObjectParticleSystemTypes::const_iterator it= particleTypes->begin(); it != particleTypes->end(); ++it){
			UnitParticleSystem *ups= new UnitParticleSystem(200);
			(*it)->setValues(ups);
			ups->setPos(this->pos);
			ups->setRotation(this->rotation);
			ups->setFactionColor(Vec3f(0, 0, 0));
			ups->setVisible(false);
			this->unitParticleSystems.push_back(ups);
			Renderer::getInstance().manageParticleSystem(ups, rsGame);
		}
	}
}


void Object::setHeight(float height){
	pos.y=height;

	for(UnitParticleSystems::iterator it= unitParticleSystems.begin(); it != unitParticleSystems.end(); ++it) {
		(*it)->setPos(this->pos);
	}
}

Model *Object::getModelPtr() const {
	Model* result;
	if(objectType==NULL){
		if(resource != NULL && resource->getType() != NULL){
			result=resource->getType()->getModel();
		}
		else
		{
			result=NULL;
		}
	} else {
		result=objectType->getTilesetModelType(variation)->getModel();
	}
	return result;
}

const Model *Object::getModel() const{
	Model* result;
	if(objectType==NULL){
		if(resource != NULL && resource->getType() != NULL){
			result=resource->getType()->getModel();
		}
		else
		{
			result=NULL;
		}
	} else {
		result=objectType->getTilesetModelType(variation)->getModel();
	}
	return result;
}

bool Object::getWalkable() const{
	return objectType==NULL? false: objectType->getWalkable();
}

void Object::setResource(const ResourceType *resourceType, const Vec2i &pos){
	resource= new Resource();
	resource->init(resourceType, pos);
	initParticlesFromTypes(resourceType->getObjectParticleSystemTypes());
}

void Object::setVisible( bool visible)
{
	this->visible=visible;
	for(UnitParticleSystems::iterator it= unitParticleSystems.begin(); it != unitParticleSystems.end(); ++it) {
			(*it)->setVisible(visible);
		}
}

}}//end namespace
