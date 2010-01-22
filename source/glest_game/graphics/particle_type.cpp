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

#include "particle_type.h"

#include "util.h"
#include "core_data.h"
#include "xml_parser.h"
#include "renderer.h"
#include "config.h"
#include "game_constants.h"

#include "leak_dumper.h"

using namespace Shared::Xml;
using namespace Shared::Graphics;

namespace Glest{ namespace Game{

// =====================================================
// 	class ParticleSystemType
// =====================================================

ParticleSystemType::ParticleSystemType(){
}

void ParticleSystemType::load(const XmlNode *particleSystemNode, const string &dir){
	
	Renderer &renderer= Renderer::getInstance();

	//texture
	const XmlNode *textureNode= particleSystemNode->getChild("texture");
	bool textureEnabled= textureNode->getAttribute("value")->getBoolValue();
	if(textureEnabled){
		texture= renderer.newTexture2D(rsGame);
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
	}
	
	//model
	const XmlNode *modelNode= particleSystemNode->getChild("model");
	bool modelEnabled= modelNode->getAttribute("value")->getBoolValue();
	if(modelEnabled){
		string path= modelNode->getAttribute("path")->getRestrictedValue();
		model= renderer.newModel(rsGame);
		model->load(dir + "/" + path);
	}
	else{
		model= NULL;
	}

	//primitive
	const XmlNode *primitiveNode= particleSystemNode->getChild("primitive");
	primitive= primitiveNode->getAttribute("value")->getRestrictedValue();

	//offset
	const XmlNode *offsetNode= particleSystemNode->getChild("offset");
	offset.x= offsetNode->getAttribute("x")->getFloatValue();
	offset.y= offsetNode->getAttribute("y")->getFloatValue();
	offset.z= offsetNode->getAttribute("z")->getFloatValue();

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
	emissionRate= emissionRateNode->getAttribute("value")->getIntValue();

	//energy max
	const XmlNode *energyMaxNode= particleSystemNode->getChild("energy-max");
	energyMax= energyMaxNode->getAttribute("value")->getIntValue();

	//speed
	const XmlNode *energyVarNode= particleSystemNode->getChild("energy-var");
	energyVar= energyVarNode->getAttribute("value")->getIntValue();
}

void ParticleSystemType::setValues(AttackParticleSystem *ats){
	ats->setTexture(texture);
	ats->setPrimitive(AttackParticleSystem::strToPrimitive(primitive));
	ats->setOffset(offset);
	ats->setColor(color);
	ats->setColorNoEnergy(colorNoEnergy);
	ats->setSpeed(speed);
	ats->setGravity(gravity);
	ats->setParticleSize(size);
	ats->setSizeNoEnergy(sizeNoEnergy);
	ats->setEmissionRate(emissionRate);
	ats->setMaxParticleEnergy(energyMax);
	ats->setVarParticleEnergy(energyVar);
	ats->setModel(model);
}

// ===========================================================
//	class ParticleSystemTypeProjectile
// ===========================================================

void ParticleSystemTypeProjectile::load(const string &dir, const string &path){

	try{
		XmlTree xmlTree;
		xmlTree.load(path);
		const XmlNode *particleSystemNode= xmlTree.getRootNode();
		
		ParticleSystemType::load(particleSystemNode, dir);

		//trajectory values
		const XmlNode *tajectoryNode= particleSystemNode->getChild("trajectory");
		trajectory= tajectoryNode->getAttribute("type")->getRestrictedValue();

		//trajectory speed
		const XmlNode *tajectorySpeedNode= tajectoryNode->getChild("speed");
		trajectorySpeed= tajectorySpeedNode->getAttribute("value")->getFloatValue()/GameConstants::updateFps;

		if(trajectory=="parabolic" || trajectory=="spiral"){
			//trajectory scale
			const XmlNode *tajectoryScaleNode= tajectoryNode->getChild("scale");
			trajectoryScale= tajectoryScaleNode->getAttribute("value")->getFloatValue();
		}
		else{
			trajectoryScale= 1.0f;
		}

		if(trajectory=="spiral"){
			//trajectory frequency
			const XmlNode *tajectoryFrequencyNode= tajectoryNode->getChild("frequency");
			trajectoryFrequency= tajectoryFrequencyNode->getAttribute("value")->getFloatValue();
		}
		else{
			trajectoryFrequency= 1.0f;
		}
	}
	catch(const exception &e){
		throw runtime_error("Error loading ParticleSystem: "+ path + "\n" +e.what());
	}
}

ProjectileParticleSystem *ParticleSystemTypeProjectile::create(){
	ProjectileParticleSystem *ps=  new ProjectileParticleSystem();

	ParticleSystemType::setValues(ps);

	ps->setTrajectory(ProjectileParticleSystem::strToTrajectory(trajectory));
	ps->setTrajectorySpeed(trajectorySpeed);
	ps->setTrajectoryScale(trajectoryScale);
	ps->setTrajectoryFrequency(trajectoryFrequency);

	return ps;
}

// ===========================================================
//	class ParticleSystemTypeSplash
// ===========================================================

void ParticleSystemTypeSplash::load(const string &dir, const string &path){

	try{
		XmlTree xmlTree;
		xmlTree.load(path);
		const XmlNode *particleSystemNode= xmlTree.getRootNode();
		
		ParticleSystemType::load(particleSystemNode, dir);

		//emission rate fade
		const XmlNode *emissionRateFadeNode= particleSystemNode->getChild("emission-rate-fade");
		emissionRateFade= emissionRateFadeNode->getAttribute("value")->getIntValue();
		
		//spread values
		const XmlNode *verticalSpreadNode= particleSystemNode->getChild("vertical-spread");
		verticalSpreadA= verticalSpreadNode->getAttribute("a")->getFloatValue(0.0f, 1.0f);
		verticalSpreadB= verticalSpreadNode->getAttribute("b")->getFloatValue(-1.0f, 1.0f);

		const XmlNode *horizontalSpreadNode= particleSystemNode->getChild("horizontal-spread");
		horizontalSpreadA= horizontalSpreadNode->getAttribute("a")->getFloatValue(0.0f, 1.0f);
		horizontalSpreadB= horizontalSpreadNode->getAttribute("b")->getFloatValue(-1.0f, 1.0f);
	}
	catch(const exception &e){
		throw runtime_error("Error loading ParticleSystem: "+ path + "\n" +e.what());
	}
}

SplashParticleSystem *ParticleSystemTypeSplash::create(){
	SplashParticleSystem *ps=  new SplashParticleSystem();

	ParticleSystemType::setValues(ps);

	ps->setEmissionRateFade(emissionRateFade);
	ps->setVerticalSpreadA(verticalSpreadA);
	ps->setVerticalSpreadB(verticalSpreadB);
	ps->setHorizontalSpreadA(horizontalSpreadA);
	ps->setHorizontalSpreadB(horizontalSpreadB);

	return ps;
}

}}//end mamespace
