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

UnitParticleSystemType::UnitParticleSystemType(){
	texture = NULL;
}

void UnitParticleSystemType::load(const XmlNode *particleSystemNode, const string &dir, RendererInterface *renderer) {
	//texture
	const XmlNode *textureNode= particleSystemNode->getChild("texture");
	bool textureEnabled= textureNode->getAttribute("value")->getBoolValue();
	if(textureEnabled == true) {
		//Renderer &renderer= Renderer::getInstance();
		texture= renderer->newTexture2D(rsGame);
		//texture = newTexture;

		if(textureNode->getAttribute("luminance")->getBoolValue()){
			texture->setFormat(Texture::fAlpha);
			texture->getPixmap()->init(1);
		}
		else{
			texture->getPixmap()->init(4);
		}
		texture->load(dir + "/" + textureNode->getAttribute("path")->getRestrictedValue());
	}
	else{
		texture= NULL;

		//delete newTexture;
		//newTexture = NULL;
	}
	
	//primitive
	const XmlNode *primitiveNode= particleSystemNode->getChild("primitive");
	primitive= primitiveNode->getAttribute("value")->getRestrictedValue();

	//offset
	const XmlNode *offsetNode= particleSystemNode->getChild("offset");
	offset.x= offsetNode->getAttribute("x")->getFloatValue();
	offset.y= offsetNode->getAttribute("y")->getFloatValue();
	offset.z= offsetNode->getAttribute("z")->getFloatValue();

	//direction
	const XmlNode *directionNode= particleSystemNode->getChild("direction");
	direction.x= directionNode->getAttribute("x")->getFloatValue();
	direction.y= directionNode->getAttribute("y")->getFloatValue();
	direction.z= directionNode->getAttribute("z")->getFloatValue();

	//color
	const XmlNode *colorNode= particleSystemNode->getChild("color");
	color.x= colorNode->getAttribute("red")->getFloatValue(0.f, 1.0f);
	color.y= colorNode->getAttribute("green")->getFloatValue(0.f, 1.0f);
	color.z= colorNode->getAttribute("blue")->getFloatValue(0.f, 1.0f);
	color.w= colorNode->getAttribute("alpha")->getFloatValue(0.f, 1.0f);

	//color
	const XmlNode *colorNoEnergyNode= particleSystemNode->getChild("color-no-energy");
	colorNoEnergy.x= colorNoEnergyNode->getAttribute("red")->getFloatValue(0.f, 1.0f);
	colorNoEnergy.y= colorNoEnergyNode->getAttribute("green")->getFloatValue(0.f, 1.0f);
	colorNoEnergy.z= colorNoEnergyNode->getAttribute("blue")->getFloatValue(0.f, 1.0f);
	colorNoEnergy.w= colorNoEnergyNode->getAttribute("alpha")->getFloatValue(0.f, 1.0f);

	//radius
	const XmlNode *radiusNode= particleSystemNode->getChild("radius");
	radius= radiusNode->getAttribute("value")->getFloatValue();

	//size
	const XmlNode *sizeNode= particleSystemNode->getChild("size");
	size= sizeNode->getAttribute("value")->getFloatValue();

	//sizeNoEnergy
	const XmlNode *sizeNoEnergyNode= particleSystemNode->getChild("size-no-energy");
	sizeNoEnergy= sizeNoEnergyNode->getAttribute("value")->getFloatValue();

	//speed
	const XmlNode *speedNode= particleSystemNode->getChild("speed");
	speed= speedNode->getAttribute("value")->getFloatValue()/GameConstants::updateFps;

	//gravity
	const XmlNode *gravityNode= particleSystemNode->getChild("gravity");
	gravity= gravityNode->getAttribute("value")->getFloatValue()/GameConstants::updateFps;

	//emission rate
	const XmlNode *emissionRateNode= particleSystemNode->getChild("emission-rate");
	emissionRate= emissionRateNode->getAttribute("value")->getFloatValue();

	//energy max
	const XmlNode *energyMaxNode= particleSystemNode->getChild("energy-max");
	energyMax= energyMaxNode->getAttribute("value")->getIntValue();

	//speed
	const XmlNode *energyVarNode= particleSystemNode->getChild("energy-var");
	energyVar= energyVarNode->getAttribute("value")->getIntValue();
	
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
    
    //fixed
    const XmlNode *fixedNode= particleSystemNode->getChild("fixed");
    fixed= fixedNode->getAttribute("value")->getBoolValue();
    
    //teamcolorNoEnergy
    if(particleSystemNode->hasChild("teamcolorNoEnergy")){
    	const XmlNode *teamcolorNoEnergyNode= particleSystemNode->getChild("teamcolorNoEnergy");
    	teamcolorNoEnergy= teamcolorNoEnergyNode->getAttribute("value")->getBoolValue();
    }
    else{
    	teamcolorNoEnergy=false;
    }
    
    	
    //teamcolorEnergy
    if(particleSystemNode->hasChild("teamcolorEnergy")){
    	const XmlNode *teamcolorEnergyNode= particleSystemNode->getChild("teamcolorEnergy");
    	teamcolorEnergy= teamcolorEnergyNode->getAttribute("value")->getBoolValue();
    }
    else{
    	teamcolorEnergy=false;
    }
    
    //mode
	if(particleSystemNode->hasChild("mode")){
		const XmlNode *modeNode= particleSystemNode->getChild("mode");
    	mode= modeNode->getAttribute("value")->getRestrictedValue();
	}
	else
	{
		mode="normal";
	}
}

void UnitParticleSystemType::setValues(UnitParticleSystem *ups){
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
    ups->setStaticParticleCount(staticParticleCount);
    ups->setRadius(radius);
    ups->setBlendMode(ParticleSystem::strToBlendMode(mode));
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

void UnitParticleSystemType::load(const string &dir, const string &path, RendererInterface *renderer){

	try{
		XmlTree xmlTree;
		xmlTree.load(path);
		const XmlNode *particleSystemNode= xmlTree.getRootNode();
		
		UnitParticleSystemType::load(particleSystemNode, dir, renderer);
	}
	catch(const exception &e){
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		throw runtime_error("Error loading ParticleSystem: "+ path + "\n" +e.what());
	}
}


}}//end mamespace
