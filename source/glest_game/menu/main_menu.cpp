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

#include "main_menu.h" 

#include "renderer.h"
#include "sound.h"
#include "config.h"
#include "program.h"
#include "game_util.h" 
#include "game.h"
#include "platform_util.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "faction.h"
#include "metrics.h"
#include "network_manager.h"
#include "network_message.h"
#include "socket.h"
#include "menu_state_root.h"

#include "leak_dumper.h"

using namespace Shared::Sound;
using namespace Shared::Platform;
using namespace Shared::Util;
using namespace Shared::Graphics;
using namespace	Shared::Xml;

namespace Glest{ namespace Game{

// =====================================================
// 	class MainMenu
// =====================================================

// ===================== PUBLIC ========================

MainMenu::MainMenu(Program *program):
	ProgramState(program)
{
	mouseX=100; 
	mouseY=100;

	state= NULL;
	this->program= program;

	fps= 0;
	lastFps= 0;

	setState(new MenuStateRoot(program, this));
}

MainMenu::~MainMenu(){
	delete state;
	Renderer::getInstance().endMenu();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();
	soundRenderer.stopAllSounds();
}

void MainMenu::init(){
	Renderer::getInstance().initMenu(this);
}

//asynchronus render update
void MainMenu::render(){

	Config &config= Config::getInstance();
	Renderer &renderer= Renderer::getInstance();
	CoreData &coreData= CoreData::getInstance();

	fps++;

    renderer.clearBuffers();
	
	//3d
	renderer.reset3dMenu();
	renderer.clearZBuffer();
	renderer.loadCameraMatrix(menuBackground.getCamera());
	renderer.renderMenuBackground(&menuBackground);
	renderer.renderParticleManager(rsMenu);

	//2d
	renderer.reset2d();
	state->render();
    renderer.renderMouse2d(mouseX, mouseY, mouse2dAnim);

    if(config.getBool("DebugMode")){
		renderer.renderText(
			"FPS: " + intToStr(lastFps), 
			coreData.getMenuFontNormal(), Vec3f(1.f), 10, 10, false);
    }

	renderer.swapBuffers();
}

//syncronus update
void MainMenu::update(){
	Renderer::getInstance().updateParticleManager(rsMenu);
	mouse2dAnim= (mouse2dAnim +1) % Renderer::maxMouse2dAnim;
	menuBackground.update();
	state->update();
}

void MainMenu::tick(){
	lastFps= fps;
	fps= 0;
}

//event magangement: mouse click
void MainMenu::mouseMove(int x, int y, const MouseState *ms){
    mouseX= x; mouseY= y;
	state->mouseMove(x, y, ms);
}

//returns if exiting
void MainMenu::mouseDownLeft(int x, int y){
	state->mouseClick(x, y, mbLeft);	
}

void MainMenu::mouseDownRight(int x, int y){
	state->mouseClick(x, y, mbRight);	
}

void MainMenu::keyDown(char key){
	state->keyDown(key);
}

void MainMenu::keyPress(char c){
	state->keyPress(c);
}

void MainMenu::setState(MenuState *state){
	delete this->state;
	this->state= state;
	GraphicComponent::resetFade();

	menuBackground.setTargetCamera(state->getCamera());
}


// =====================================================
// 	class MenuState
// =====================================================

MenuState::MenuState(Program *program, MainMenu *mainMenu, const string &stateName){
	this->program= program;
	this->mainMenu= mainMenu;

	//camera
	XmlTree xmlTree;
	xmlTree.load("data/core/menu/menu.xml");
	const XmlNode *menuNode= xmlTree.getRootNode();
	const XmlNode *cameraNode= menuNode->getChild("camera");

	//position
	const XmlNode *positionNode= cameraNode->getChild(stateName + "-position");
	Vec3f startPosition;
    startPosition.x= positionNode->getAttribute("x")->getFloatValue();
	startPosition.y= positionNode->getAttribute("y")->getFloatValue();
	startPosition.z= positionNode->getAttribute("z")->getFloatValue();
	camera.setPosition(startPosition);

	//rotation
	const XmlNode *rotationNode= cameraNode->getChild(stateName + "-rotation");
	Vec3f startRotation;
    startRotation.x= rotationNode->getAttribute("x")->getFloatValue();
	startRotation.y= rotationNode->getAttribute("y")->getFloatValue();
	startRotation.z= rotationNode->getAttribute("z")->getFloatValue();
	camera.setOrientation(Quaternion(EulerAngles(
		degToRad(startRotation.x), 
		degToRad(startRotation.y), 
		degToRad(startRotation.z))));
}












}}//end namespace
