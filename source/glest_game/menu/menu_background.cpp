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

#include "menu_background.h"

#include <ctime>

#include "renderer.h"
#include "core_data.h"
#include "config.h"
#include "xml_parser.h"
#include "util.h"
#include "game_constants.h"
#include "game_util.h"
#include "leak_dumper.h"

using namespace Shared::Util;
using namespace Shared::Xml;
using namespace Shared::Graphics;

namespace Glest{ namespace Game{

// =====================================================
// 	class MenuBackground
// =====================================================

MenuBackground::MenuBackground() : rps(NULL) {
	Renderer &renderer= Renderer::getInstance();

	//load data
	string data_path = getGameReadWritePath(GameConstants::path_data_CacheLookupKey);

	XmlTree xmlTree;
	xmlTree.load(getGameCustomCoreDataPath(data_path, "data/core/menu/menu.xml"),Properties::getTagReplacementValues());
	const XmlNode *menuNode= xmlTree.getRootNode();

	//water
	const XmlNode *waterNode= menuNode->getChild("water");
	water= waterNode->getAttribute("value")->getBoolValue();
	if(water){
		waterHeight= waterNode->getAttribute("height")->getFloatValue();

		//water texture
		waterTexture= renderer.newTexture2D(rsMenu);
		if(waterTexture) {
			waterTexture->getPixmap()->init(4);
			waterTexture->getPixmap()->load(getGameCustomCoreDataPath(data_path, "data/core/menu/textures/water.tga"));
		}
	}

	//fog
	const XmlNode *fogNode= menuNode->getChild("fog");
	fog= fogNode->getAttribute("value")->getBoolValue();
	if(fog){
		fogDensity= fogNode->getAttribute("density")->getFloatValue();
	}

	//rain
	bool withRainEffect = Config::getInstance().getBool("RainEffectMenu","true");
	if(withRainEffect == true) {
		rain= menuNode->getChild("rain")->getAttribute("value")->getBoolValue();
		if(rain) {
			createRainParticleSystem();
		}
	}
	else {
		rain = false;
	}

	//camera
	const XmlNode *cameraNode= menuNode->getChild("camera");

	//position
	const XmlNode *positionNode= cameraNode->getChild("start-position");
	Vec3f startPosition;
    startPosition.x= positionNode->getAttribute("x")->getFloatValue();
	startPosition.y= positionNode->getAttribute("y")->getFloatValue();
	startPosition.z= positionNode->getAttribute("z")->getFloatValue();
	camera.setPosition(startPosition);

	//rotation
	const XmlNode *rotationNode= cameraNode->getChild("start-rotation");
	Vec3f startRotation;
    startRotation.x= rotationNode->getAttribute("x")->getFloatValue();
	startRotation.y= rotationNode->getAttribute("y")->getFloatValue();
	startRotation.z= rotationNode->getAttribute("z")->getFloatValue();
	camera.setOrientation(Quaternion(EulerAngles(
		degToRad(startRotation.x),
		degToRad(startRotation.y),
		degToRad(startRotation.z))));

	//load main model
	mainModel= renderer.newModel(rsMenu);
	if(mainModel) {
		string mainModelFile = "data/core/menu/main_model/menu_main.g3d";
		if(menuNode->hasChild("menu-background-model") == true) {
			//mainModel->load(data_path + "data/core/menu/main_model/menu_main.g3d");
			const XmlNode *mainMenuModelNode= menuNode->getChild("menu-background-model");
			mainModelFile = mainMenuModelNode->getAttribute("value")->getRestrictedValue();
		}
		mainModel->load(getGameCustomCoreDataPath(data_path, mainModelFile));
	}

	//models
	for(int i=0; i<5; ++i){
		characterModels[i]= renderer.newModel(rsMenu);
		if(characterModels[i]) {
			characterModels[i]->load(getGameCustomCoreDataPath(data_path, "data/core/menu/about_models/character"+intToStr(i)+".g3d"));
		}
	}

	//about position
	positionNode= cameraNode->getChild("about-position");
	aboutPosition.x= positionNode->getAttribute("x")->getFloatValue();
	aboutPosition.y= positionNode->getAttribute("y")->getFloatValue();
	aboutPosition.z= positionNode->getAttribute("z")->getFloatValue();
	rotationNode= cameraNode->getChild("about-rotation");

	targetCamera= NULL;
	t= 0.f;
	fade= 0.f;
	anim= 0.f;
}

MenuBackground::~MenuBackground() {
	//printf("In ~MenuBackground() rps = %p\n",rps);

	cleanup();
}

void MenuBackground::cleanup() {
	//printf("In MenuBackground::cleanup() rps = %p\n",rps);

	if(rps != NULL) {
		Renderer &renderer= Renderer::getInstance();
		if(renderer.validateParticleSystemStillExists(rps,rsMenu) == true) {
			rps->fade();
			vector<ParticleSystem *> particleSystems;
			particleSystems.push_back(rps);
			renderer.cleanupParticleSystems(particleSystems, rsMenu);
		}

		rps = NULL;
	}
}

void MenuBackground::createRainParticleSystem() {
	//printf("In MenuBackground::createRainParticleSystem() rps = %p\n",rps);

	if(rps == NULL) {
		rps= new RainParticleSystem();
		rps->setSpeed(12.f/GameConstants::updateFps);
		rps->setEmissionRate(25);
		rps->setWind(-90.f, 4.f/GameConstants::updateFps);
		rps->setPos(Vec3f(0.f, 25.f, 0.f));
		rps->setColor(Vec4f(1.f, 1.f, 1.f, 0.2f));
		rps->setRadius(30.f);

		Renderer &renderer= Renderer::getInstance();
		renderer.manageParticleSystem(rps, rsMenu);

		for(int i=0; i<raindropCount; ++i){
			raindropStates[i]= random.randRange(0.f, 1.f);
			raindropPos[i]= computeRaindropPos();
		}
	}
}

void MenuBackground::setTargetCamera(const Camera *targetCamera){
	this->targetCamera= targetCamera;
	this->lastCamera= camera;
	t= 0.f;
}

void MenuBackground::update() {
	//rain drops
	bool withRainEffect = Config::getInstance().getBool("RainEffectMenu","true");
	if(withRainEffect == true) {
		if(rain == false) {
			rain = true;
			createRainParticleSystem();
		}

		for(int i=0; i<raindropCount; ++i){
			raindropStates[i]+= 1.f / GameConstants::updateFps;
			if(raindropStates[i]>=1.f){
				raindropStates[i]= 0.f;
				raindropPos[i]= computeRaindropPos();
			}
		}
	}
	else if(rain == true) {
		rain = false;

		cleanup();
	}

	if(targetCamera!=NULL){
		t+= ((0.01f+(1.f-t)/10.f)/20.f)*(60.f/GameConstants::updateFps);

		//interpolate position
		camera.setPosition(lastCamera.getPosition().lerp(t, targetCamera->getPosition()));

		//interpolate orientation
		Quaternion q= lastCamera.getOrientation().lerp(t, targetCamera->getOrientation());
		camera.setOrientation(q);

		if(t>=1.f){
			targetCamera= NULL;
			t= 0.f;
		}
	}

	//fade
	if(fade<=1.f){
		fade+= 0.6f/GameConstants::updateFps;
		if(fade>1.f){
			fade= 1.f;
		}
	}

	//animation
	anim+=(0.6f/GameConstants::updateFps)/5+random.randRange(0.f, (0.6f/GameConstants::updateFps)/5.f);
	if(anim>1.f){
		anim= 0.f;
	}
}

Vec2f MenuBackground::computeRaindropPos() {
	float f= static_cast<float>(meshSize);
	return Vec2f(random.randRange(-f, f), random.randRange(-f, f));
}

}}//end namespace

