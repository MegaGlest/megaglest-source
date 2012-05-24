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

#include "unit_particle_type.h"

#include "util.h"
#include "core_data.h"
#include "xml_parser.h"
#include "config.h"
#include "game_constants.h"
#include "platform_common.h"
#include "conversion.h"
#include "leak_dumper.h"

using namespace Shared::Xml;
using namespace Shared::Graphics;
using namespace Shared::Util;
using namespace Shared::PlatformCommon;

namespace Glest{ namespace Game{

// =====================================================
// 	class UnitParticleSystemType
// =====================================================
UnitParticleSystemType::UnitParticleSystemType() : ParticleSystemType() {
	shape = UnitParticleSystem::sLinear;
	angle = 0;
	radius = 0;
	minRadius = 0;
	emissionRateFade = 0;
    relative = false;
    relativeDirection = false;
    fixed = false;
    staticParticleCount = 0;
	isVisibleAtNight = true;
	isVisibleAtDay = true;
	isDaylightAffected = false;
	radiusBasedStartenergy = false;
	delay = 0;
	lifetime = 0;
	startTime = 0;
	endTime = 1;
}

void UnitParticleSystemType::load(const XmlNode *particleSystemNode, const string &dir,
		RendererInterface *renderer, std::map<string,vector<pair<string, string> > > &loadedFileList,
		string parentLoader, string techtreePath) {

	ParticleSystemType::load(particleSystemNode, dir, renderer, loadedFileList,
			parentLoader,techtreePath);
	
	//shape
	angle= 0.0f;
	if(particleSystemNode->hasChild("shape")){
		const XmlNode *shapeNode= particleSystemNode->getChild("shape");
		shape= UnitParticleSystem::strToShape(shapeNode->getAttribute("value")->getRestrictedValue());
		if(shape == UnitParticleSystem::sConical){
			angle= shapeNode->getChild("angle")->getAttribute("value")->getFloatValue();
		}
	} else {
		shape = UnitParticleSystem::sLinear;
	}
	if(shape != UnitParticleSystem::sSpherical){
		//direction
		const XmlNode *directionNode= particleSystemNode->getChild("direction");
		direction.x= directionNode->getAttribute("x")->getFloatValue();
		direction.y= directionNode->getAttribute("y")->getFloatValue();
		direction.z= directionNode->getAttribute("z")->getFloatValue();
		if((shape == UnitParticleSystem::sConical) && (0.0 == direction.length()))
			throw megaglest_runtime_error("direction cannot be zero");
		// ought to warn about 0 directions generally
	}

	//emission rate fade
	if(particleSystemNode->hasChild("emission-rate-fade")){
		const XmlNode *emissionRateFadeNode= particleSystemNode->getChild("emission-rate-fade");
		emissionRateFade= emissionRateFadeNode->getAttribute("value")->getFloatValue();
	} else {
		emissionRateFade = 0;
	}
	
	//radius
	const XmlNode *radiusNode= particleSystemNode->getChild("radius");
	radius= radiusNode->getAttribute("value")->getFloatValue();
	
	// min radius
	if(particleSystemNode->hasChild("min-radius")){
		const XmlNode *minRadiusNode= particleSystemNode->getChild("min-radius");
		minRadius= minRadiusNode->getAttribute("value")->getFloatValue();
		if(minRadius > radius)
			throw megaglest_runtime_error("min-radius cannot be bigger than radius");
	} else {
		minRadius = 0;
	}
	if((minRadius == 0) && (shape == UnitParticleSystem::sConical)) {
		minRadius = 0.001f; // fudge it so we aren't generating particles that are exactly centred
		if(minRadius > radius)
			radius = minRadius;
	}

	//relative
    const XmlNode *relativeNode= particleSystemNode->getChild("relative");
    relative= relativeNode->getAttribute("value")->getBoolValue();
    

    //relativeDirection
    if(particleSystemNode->hasChild("relativeDirection")){
    	const XmlNode *relativeDirectionNode= particleSystemNode->getChild("relativeDirection");
    	relativeDirection= relativeDirectionNode->getAttribute("value")->getBoolValue();
    }
    else{
    	relativeDirection=true;
    }
    
    if(particleSystemNode->hasChild("static-particle-count")){
	    //staticParticleCount
		const XmlNode *staticParticleCountNode= particleSystemNode->getChild("static-particle-count");
		staticParticleCount= staticParticleCountNode->getAttribute("value")->getIntValue();
    }
    else{
    	staticParticleCount=0;
    }
    
    //isVisibleAtNight
	if(particleSystemNode->hasChild("isVisibleAtNight")){
		const XmlNode *isVisibleAtNightNode= particleSystemNode->getChild("isVisibleAtNight");
		isVisibleAtNight= isVisibleAtNightNode->getAttribute("value")->getBoolValue();
	}
	else {
		isVisibleAtNight=true;
	}

    //isVisibleAtDay
	if(particleSystemNode->hasChild("isVisibleAtDay")){
		const XmlNode *isVisibleAtDayNode= particleSystemNode->getChild("isVisibleAtDay");
		isVisibleAtDay= isVisibleAtDayNode->getAttribute("value")->getBoolValue();
	}
	else {
		isVisibleAtDay=true;
	}

    //isDaylightAffected
	if(particleSystemNode->hasChild("isDaylightAffected")){
		const XmlNode *node= particleSystemNode->getChild("isDaylightAffected");
		isDaylightAffected= node->getAttribute("value")->getBoolValue();
	}
	else {
		isDaylightAffected=false;
	}

	//radiusBasedStartenergy
	if(particleSystemNode->hasChild("radiusBasedStartenergy")){
		const XmlNode *isVisibleAtDayNode= particleSystemNode->getChild("radiusBasedStartenergy");
		radiusBasedStartenergy= isVisibleAtDayNode->getAttribute("value")->getBoolValue();
	}
	else{
		radiusBasedStartenergy= true;
	}

	//fixed
	const XmlNode *fixedNode= particleSystemNode->getChild("fixed");
	fixed= fixedNode->getAttribute("value")->getBoolValue();

	// delay
	if(particleSystemNode->hasChild("delay")) {
		const XmlNode* delayNode = particleSystemNode->getChild("delay");
		const float delay_secs = delayNode->getAttribute("value")->getFloatValue();
		if(delay_secs < 0)
			throw megaglest_runtime_error("particle effect delay cannot be negative");
		delay = (int)delay_secs * GameConstants::updateFps;
	} else{
		delay= 0;
	}

	// lifetime
	if(particleSystemNode->hasChild("lifetime")) {
		const XmlNode* lifetimeNode = particleSystemNode->getChild("lifetime");
		const float lifetime_secs = lifetimeNode->getAttribute("value")->getFloatValue();
		if(lifetime_secs < 0 && lifetime_secs != -1)
			throw megaglest_runtime_error("particle effect lifetime cannot be negative (-1 means inherited from parent particle)");
		lifetime = (int)lifetime_secs * GameConstants::updateFps;
	} else{
		lifetime= -1; //default
	}
}

ObjectParticleSystemType::ObjectParticleSystemType() : UnitParticleSystemType() {
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s Line: %d] NEW [%p]\n",__FUNCTION__,__LINE__,this);
}
ObjectParticleSystemType::~ObjectParticleSystemType() {
	if(SystemFlags::VERBOSE_MODE_ENABLED) {
		printf("In [%s Line: %d] NEW [%p]\n",__FUNCTION__,__LINE__,this);
		printf("%s\n",PlatformExceptionHandler::getStackTrace().c_str());
	}
}

const void UnitParticleSystemType::setValues(UnitParticleSystem *ups){
	// whilst we extend ParticleSystemType we don't use ParticleSystemType::setValues()
	// add instances of all children; some settings will cascade to all children
	for(Children::iterator i=children.begin(); i!=children.end(); ++i){
		UnitParticleSystem *child = new UnitParticleSystem();
		(*i)->setValues(child);
		ups->addChild(child);
	}
	// set values
	ups->setModel(model);
	ups->setModelCycle(modelCycle);
	ups->setTexture(texture);
	ups->setPrimitive(UnitParticleSystem::strToPrimitive(primitive));
	ups->setOffset(offset);
	ups->setShape(shape);
	ups->setAngle(angle);
	ups->setDirection(direction);
	ups->setColor(color);
	ups->setColorNoEnergy(colorNoEnergy);
	ups->setSpeed(speed);
	ups->setGravity(gravity);
	ups->setParticleSize(size);
	ups->setSizeNoEnergy(sizeNoEnergy);
	ups->setEmissionRate(emissionRate);
	ups->setMaxParticleEnergy(energyMax);
	ups->setVarParticleEnergy(energyVar);
	ups->setFixed(fixed);
	ups->setRelative(relative);
	ups->setRelativeDirection(relativeDirection);
	ups->setDelay(delay);
	ups->setLifetime(lifetime);
	ups->setEmissionRateFade(emissionRateFade);
    ups->setTeamcolorNoEnergy(teamcolorNoEnergy);
    ups->setTeamcolorEnergy(teamcolorEnergy);
    ups->setAlternations(alternations);
    ups->setParticleSystemStartDelay(particleSystemStartDelay);

    ups->setIsVisibleAtNight(isVisibleAtNight);
	ups->setIsVisibleAtDay(isVisibleAtDay);
	ups->setIsDaylightAffected(isDaylightAffected);
    ups->setStaticParticleCount(staticParticleCount);
    ups->setRadius(radius);
    ups->setMinRadius(minRadius);
    ups->setBlendMode(ParticleSystem::strToBlendMode(mode));
    ups->setRadiusBasedStartenergy(radiusBasedStartenergy);
    //prepare system for given staticParticleCount
	if(staticParticleCount>0)
	{
		ups->setEmissionRate(0.0f);
		ups->setSpeed(0);
		direction.x= 0.0f;
		direction.y= 0.0f;
		direction.z= 0.0f;
		ups->setDirection(direction);
	}

	ups->setStartTime(startTime);
	ups->setEndTime(endTime);
}

void UnitParticleSystemType::load(const XmlNode *particleFileNode, const string &dir, const string &path,
		RendererInterface *renderer, std::map<string,vector<pair<string, string> > > &loadedFileList,
		string parentLoader, string techtreePath) {

	try{
		XmlTree xmlTree;

		std::map<string,string> mapExtraTagReplacementValues;
		mapExtraTagReplacementValues["$COMMONDATAPATH"] = techtreePath + "/commondata/";
		xmlTree.load(path, Properties::getTagReplacementValues(&mapExtraTagReplacementValues));
		loadedFileList[path].push_back(make_pair(parentLoader,parentLoader));
		const XmlNode *particleSystemNode= xmlTree.getRootNode();
		
		if(particleFileNode){
			// immediate children in the particleFileNode will override the particleSystemNode
			particleFileNode->setSuper(particleSystemNode);
			particleSystemNode= particleFileNode;
		}
		
		UnitParticleSystemType::load(particleSystemNode, dir, renderer,
				loadedFileList, parentLoader, techtreePath);
	}
	catch(const exception &e){
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		throw megaglest_runtime_error("Error loading ParticleSystem: "+ path + "\n" +e.what());
	}
}

void UnitParticleSystemType::saveGame(XmlNode *rootNode) {
	ParticleSystemType::saveGame(rootNode);

	std::map<string,string> mapTagReplacements;
	XmlNode *unitParticleSystemTypeNode = rootNode->addChild("UnitParticleSystemType");

//	UnitParticleSystem::Shape shape;
	unitParticleSystemTypeNode->addAttribute("shape",intToStr(shape), mapTagReplacements);
//	float angle;
	unitParticleSystemTypeNode->addAttribute("angle",floatToStr(angle), mapTagReplacements);
//	float radius;
	unitParticleSystemTypeNode->addAttribute("radius",floatToStr(radius), mapTagReplacements);
//	float minRadius;
	unitParticleSystemTypeNode->addAttribute("minRadius",floatToStr(minRadius), mapTagReplacements);
//	float emissionRateFade;
	unitParticleSystemTypeNode->addAttribute("emissionRateFade",floatToStr(emissionRateFade), mapTagReplacements);
//	Vec3f direction;
	unitParticleSystemTypeNode->addAttribute("direction",direction.getString(), mapTagReplacements);
//    bool relative;
	unitParticleSystemTypeNode->addAttribute("relative",intToStr(relative), mapTagReplacements);
//    bool relativeDirection;
	unitParticleSystemTypeNode->addAttribute("relativeDirection",intToStr(relativeDirection), mapTagReplacements);
//    bool fixed;
	unitParticleSystemTypeNode->addAttribute("fixed",intToStr(fixed), mapTagReplacements);
//    int staticParticleCount;
	unitParticleSystemTypeNode->addAttribute("staticParticleCount",intToStr(staticParticleCount), mapTagReplacements);
//	bool isVisibleAtNight;
	unitParticleSystemTypeNode->addAttribute("isVisibleAtNight",intToStr(isVisibleAtNight), mapTagReplacements);
//	bool isVisibleAtDay;
	unitParticleSystemTypeNode->addAttribute("isVisibleAtDay",intToStr(isVisibleAtDay), mapTagReplacements);
//	bool isDaylightAffected;
	unitParticleSystemTypeNode->addAttribute("isDaylightAffected",intToStr(isDaylightAffected), mapTagReplacements);
//	bool radiusBasedStartenergy;
	unitParticleSystemTypeNode->addAttribute("radiusBasedStartenergy",intToStr(radiusBasedStartenergy), mapTagReplacements);
//	int delay;
	unitParticleSystemTypeNode->addAttribute("delay",intToStr(delay), mapTagReplacements);
//	int lifetime;
	unitParticleSystemTypeNode->addAttribute("lifetime",intToStr(lifetime), mapTagReplacements);
//	float startTime;
	unitParticleSystemTypeNode->addAttribute("startTime",floatToStr(startTime), mapTagReplacements);
//	float endTime;
	unitParticleSystemTypeNode->addAttribute("endTime",floatToStr(endTime), mapTagReplacements);
}

}}//end mamespace
