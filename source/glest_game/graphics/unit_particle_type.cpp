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

#include "unit_particle_type.h"

#include "util.h"
#include "core_data.h"
#include "xml_parser.h"
#include "config.h"
#include "game_constants.h"

#include "leak_dumper.h"

using namespace Shared::Xml;
using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class UnitParticleSystemType
// =====================================================

void UnitParticleSystemType::load(const XmlNode *particleSystemNode, const string &dir,
		RendererInterface *renderer, std::map<string,vector<pair<string, string> > > &loadedFileList,
		string parentLoader, string techtreePath) {
	ParticleSystemType::load(particleSystemNode, dir, renderer, loadedFileList,
			parentLoader,techtreePath);
	//radius
	const XmlNode *radiusNode= particleSystemNode->getChild("radius");
	radius= radiusNode->getAttribute("value")->getFloatValue();

	//relative
    const XmlNode *relativeNode= particleSystemNode->getChild("relative");
    relative= relativeNode->getAttribute("value")->getBoolValue();
    
	//direction
	const XmlNode *directionNode= particleSystemNode->getChild("direction");
	direction.x= directionNode->getAttribute("x")->getFloatValue();
	direction.y= directionNode->getAttribute("y")->getFloatValue();
	direction.z= directionNode->getAttribute("z")->getFloatValue();

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
}

const void UnitParticleSystemType::setValues(UnitParticleSystem *ups){
	ups->setTexture(texture);
	ups->setPrimitive(UnitParticleSystem::strToPrimitive(primitive));
	ups->setOffset(offset);
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
    ups->setTeamcolorNoEnergy(teamcolorNoEnergy);
    ups->setTeamcolorEnergy(teamcolorEnergy);
    ups->setAlternations(alternations);

    ups->setIsVisibleAtNight(isVisibleAtNight);
	ups->setIsVisibleAtDay(isVisibleAtDay);
    ups->setStaticParticleCount(staticParticleCount);
    ups->setRadius(radius);
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
}

void UnitParticleSystemType::load(const string &dir, const string &path,
		RendererInterface *renderer, std::map<string,vector<pair<string, string> > > &loadedFileList,
		string parentLoader, string techtreePath) {

	try{
		XmlTree xmlTree;

		std::map<string,string> mapExtraTagReplacementValues;
		mapExtraTagReplacementValues["$COMMONDATAPATH"] = techtreePath + "/commondata/";
		xmlTree.load(path, Properties::getTagReplacementValues(&mapExtraTagReplacementValues));
		loadedFileList[path].push_back(make_pair(parentLoader,parentLoader));
		const XmlNode *particleSystemNode= xmlTree.getRootNode();
		
		UnitParticleSystemType::load(particleSystemNode, dir, renderer,
				loadedFileList, parentLoader, techtreePath);
	}
	catch(const exception &e){
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		throw runtime_error("Error loading ParticleSystem: "+ path + "\n" +e.what());
	}
}

}}//end mamespace
