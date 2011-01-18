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
	}
}

Object::~Object(){
	delete resource;

	Renderer &renderer= Renderer::getInstance();
	renderer.removeObjectFromQuadCache(this);
}

Model *Object::getModelPtr() const {
	return objectType==NULL ?  (resource != NULL && resource->getType() != NULL ? resource->getType()->getModel() : NULL ) : objectType->getModel(variation);
}

const Model *Object::getModel() const{
	return objectType==NULL ?  (resource != NULL && resource->getType() != NULL ? resource->getType()->getModel() : NULL ) : objectType->getModel(variation);
}

bool Object::getWalkable() const{
	return objectType==NULL? false: objectType->getWalkable();
}

void Object::setResource(const ResourceType *resourceType, const Vec2i &pos){
	resource= new Resource();
	resource->init(resourceType, pos);
}

}}//end namespace
