// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
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
#include "game.h"

using namespace Shared::Util;

namespace Glest{ namespace Game{

ObjectStateInterface *Object::stateCallback=NULL;

// =====================================================
// 	class Object
// =====================================================

Object::Object(ObjectType *objectType, const Vec3f &pos, const Vec2i &mapPos) : BaseColorPickEntity() {
	RandomGen random;

	random.init(static_cast<int>(pos.x*pos.z));
	this->lastRenderFrame = 0;
	this->objectType= objectType;
	resource= NULL;
	highlight= 0.f;
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
	animProgress=0.0f;
}

Object::~Object() {
	Renderer &renderer = Renderer::getInstance();
	// fade(and by this remove) all unit particle systems
	while(unitParticleSystems.empty() == false) {
		bool particleValid = renderer.validateParticleSystemStillExists(unitParticleSystems.back(),rsGame);
		if(particleValid == true) {
			unitParticleSystems.back()->fade();
		}
		unitParticleSystems.pop_back();
	}
	renderer.removeObjectFromQuadCache(this);
	if(stateCallback) {
		stateCallback->removingObjectEvent(this);
	}
	delete resource;
	resource = NULL;

	recycleUniqueColor();
}

void Object::end() {
	// set Objects to fading and remove them from list.
	// its needed because otherwise they will be accessed from the destructor
	while(unitParticleSystems.empty() == false) {
		bool particleValid = Renderer::getInstance().validateParticleSystemStillExists(unitParticleSystems.back(),rsGame);
		if(particleValid == true) {
			unitParticleSystems.back()->fade();
		}
		unitParticleSystems.pop_back();
	}
}

void Object::initParticles() {
	if(this->objectType == NULL) {
		return;
	}
	if(this->objectType->getTilesetModelType(variation)->hasParticles()) {
		ModelParticleSystemTypes *particleTypes= this->objectType->getTilesetModelType(variation)->getParticleTypes();
		initParticlesFromTypes(particleTypes);
	}
}

void Object::initParticlesFromTypes(const ModelParticleSystemTypes *particleTypes) {
	bool showTilesetParticles = Config::getInstance().getBool("TilesetParticles", "true");
	if(showTilesetParticles == true && GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false &&
			particleTypes->empty() == false && unitParticleSystems.empty() == true) {
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

void Object::setHeight(float height) {
	pos.y=height;

	for(UnitParticleSystems::iterator it= unitParticleSystems.begin(); it != unitParticleSystems.end(); ++it) {
		bool particleValid = Renderer::getInstance().validateParticleSystemStillExists((*it),rsGame);
		if(particleValid == true) {
			(*it)->setPos(this->pos);
		}
	}
}

void Object::update() {
	//highlight
	if(highlight > 0.f) {
		const Game *game = Renderer::getInstance().getGame();
		highlight -= 1.f / (Game::highlightTime * game->getWorld()->getUpdateFps(-1));
	}

	if(objectType != NULL && objectType->getTilesetModelType(variation) != NULL &&
			objectType->getTilesetModelType(variation)->getAnimSpeed() != 0.0) {
//		printf("#1 Object updating [%s] Speed [%d] animProgress [%f]\n",this->objectType->getTilesetModelType(variation)->getModel()->getFileName().c_str(),objectType->getTilesetModelType(variation)->getAnimSpeed(),animProgress);

		float heightFactor   = 1.f;
		const float speedDivider= 100.f;
		float speedDenominator = (speedDivider * GameConstants::updateFps);
		float newAnimProgress = animProgress + (((float)objectType->getTilesetModelType(variation)->getAnimSpeed() * heightFactor) / speedDenominator);

//		printf("A [%f] B [%f] C [%f] D [%f] E [%f] F [%f]\n",
//				((float)objectType->getTilesetModelType(variation)->getAnimSpeed() * heightFactor),
//				speedDenominator,
//				((objectType->getTilesetModelType(variation)->getAnimSpeed() * heightFactor) / speedDenominator),
//				(animProgress + ((objectType->getTilesetModelType(variation)->getAnimSpeed() * heightFactor) / speedDenominator)),
//				animProgress,newAnimProgress);

		animProgress = newAnimProgress;
//		printf("#2 new animProgress [%f]\n",animProgress);

		if(animProgress > 1.f) {
			animProgress = 0.f;
		}
	}
}

void Object::resetHighlight(){
	highlight= 1.f;
}

Model *Object::getModelPtr() const {
	Model* result = NULL;
	if(objectType==NULL) {
		if(resource != NULL && resource->getType() != NULL){
			result = resource->getType()->getModel();
		}
	}
	else {
		result=objectType->getTilesetModelType(variation)->getModel();
	}
	return result;
}

const Model *Object::getModel() const {
	Model* result = NULL;
	if(objectType == NULL){
		if(resource != NULL && resource->getType() != NULL){
			result=resource->getType()->getModel();
		}
	}
	else {
		result=objectType->getTilesetModelType(variation)->getModel();
	}
	return result;
}

bool Object::getWalkable() const{
	return objectType==NULL? false: objectType->getWalkable();
}

void Object::setResource(const ResourceType *resourceType, const Vec2i &pos){
	delete resource;
	resource= new Resource();
	resource->init(resourceType, pos);
	initParticlesFromTypes(resourceType->getObjectParticleSystemTypes());
}

void Object::setVisible( bool visible)
{
	this->visible=visible;
	for(UnitParticleSystems::iterator it= unitParticleSystems.begin(); it != unitParticleSystems.end(); ++it) {
		bool particleValid = Renderer::getInstance().validateParticleSystemStillExists((*it),rsGame);
		if(particleValid == true) {
			(*it)->setVisible(visible);
		}
	}
}

string Object::getUniquePickName() const {
	string result = "";
	if(resource != NULL) {
		result += resource->getDescription() + " : ";
	}
	result += mapPos.getString();
	return result;
}

void Object::saveGame(XmlNode *rootNode) {
	std::map<string,string> mapTagReplacements;
	XmlNode *objectNode = rootNode->addChild("Object");

//	ObjectType *objectType;
	if(objectType != NULL) {
		objectNode->addAttribute("objectType",intToStr(objectType->getClass()), mapTagReplacements);
	}
//	vector<UnitParticleSystem*> unitParticleSystems;
	for(unsigned int i = 0; i < unitParticleSystems.size(); ++i) {
		UnitParticleSystem *ptr= unitParticleSystems[i];
		if(ptr != NULL) {
			ptr->saveGame(objectNode);
		}
	}
//	Resource *resource;
	if(resource != NULL) {
		resource->saveGame(objectNode);
	}
//	Vec3f pos;
	objectNode->addAttribute("pos",pos.getString(), mapTagReplacements);
//	float rotation;
	objectNode->addAttribute("rotation",floatToStr(rotation), mapTagReplacements);
//	int variation;
	objectNode->addAttribute("variation",intToStr(variation), mapTagReplacements);
//	int lastRenderFrame;
	objectNode->addAttribute("lastRenderFrame",intToStr(lastRenderFrame), mapTagReplacements);
//	Vec2i mapPos;
	objectNode->addAttribute("mapPos",mapPos.getString(), mapTagReplacements);
//	bool visible;
	objectNode->addAttribute("visible",intToStr(visible), mapTagReplacements);
}

void Object::loadGame(const XmlNode *rootNode,const TechTree *techTree) {
	const XmlNode *objectNode = rootNode->getChild("Object");

	//description = objectNode->getAttribute("description")->getValue();

	//	ObjectType *objectType;
//	if(objectType != NULL) {
//		objectNode->addAttribute("objectType",intToStr(objectType->getClass()), mapTagReplacements);
//	}
//	//	vector<UnitParticleSystem*> unitParticleSystems;
//	for(unsigned int i = 0; i < unitParticleSystems.size(); ++i) {
//		UnitParticleSystem *ptr= unitParticleSystems[i];
//		if(ptr != NULL) {
//			ptr->saveGame(objectNode);
//		}
//	}
	//	Resource *resource;
	if(resource != NULL) {
		resource->loadGame(objectNode,0,techTree);
	}
	//	Vec3f pos;
	pos = Vec3f::strToVec3(objectNode->getAttribute("pos")->getValue());
	//	float rotation;
	rotation = objectNode->getAttribute("rotation")->getFloatValue();
	//	int variation;
	variation = objectNode->getAttribute("variation")->getIntValue();
	//	int lastRenderFrame;
	lastRenderFrame = objectNode->getAttribute("lastRenderFrame")->getIntValue();
	//	Vec2i mapPos;
	mapPos = Vec2i::strToVec2(objectNode->getAttribute("mapPos")->getValue());
	//	bool visible;
	visible = objectNode->getAttribute("visible")->getIntValue() != 0;
}

}}//end namespace
