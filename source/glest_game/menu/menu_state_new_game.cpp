// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "menu_state_new_game.h"

#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "config.h"
#include "menu_state_custom_game.h"
#include "menu_state_scenario.h"
#include "menu_state_root.h"
#include "metrics.h"
#include "network_manager.h"
#include "network_message.h"
#include "auto_test.h"
#include "socket.h"

#include "leak_dumper.h"

namespace Glest{ namespace Game{

// =====================================================
// 	class MenuStateNewGame
// =====================================================

MenuStateNewGame::MenuStateNewGame(Program *program, MainMenu *mainMenu):
	MenuState(program, mainMenu, "root")
{
	containerName = "NewGame";
	Lang &lang= Lang::getInstance();

	buttonCustomGame.registerGraphicComponent(containerName,"buttonCustomGame");
	buttonCustomGame.init(425, 350, 150);
	buttonScenario.registerGraphicComponent(containerName,"buttonScenario");
    buttonScenario.init(425, 310, 150);
    buttonTutorial.registerGraphicComponent(containerName,"buttonTutorial");
    buttonTutorial.init(425, 270, 150);
    buttonReturn.registerGraphicComponent(containerName,"buttonReturn");
    buttonReturn.init(425, 230, 150);

	buttonCustomGame.setText(lang.get("CustomGame"));
	buttonScenario.setText(lang.get("Scenario"));
	buttonTutorial.setText(lang.get("Tutorial"));
	buttonReturn.setText(lang.get("Return"));

	GraphicComponent::applyAllCustomProperties(containerName);

	NetworkManager::getInstance().end();
}

void MenuStateNewGame::mouseClick(int x, int y, MouseButton mouseButton){

	CoreData &coreData=  CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	if(buttonCustomGame.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundB());
		mainMenu->setState(new MenuStateCustomGame(program, mainMenu));
    }
	else if(buttonScenario.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundB());
		mainMenu->setState(new MenuStateScenario(program, mainMenu, Config::getInstance().getPathListForType(ptScenarios)));
    }
	else if(buttonTutorial.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundB());
		mainMenu->setState(new MenuStateScenario(program, mainMenu, Config::getInstance().getPathListForType(ptTutorials)));
    }
    else if(buttonReturn.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundB());
		mainMenu->setState(new MenuStateRoot(program, mainMenu));
    }
}

void MenuStateNewGame::mouseMove(int x, int y, const MouseState *ms){
	buttonCustomGame.mouseMove(x, y);
    buttonScenario.mouseMove(x, y);
    buttonTutorial.mouseMove(x, y);
    buttonReturn.mouseMove(x, y);
}

void MenuStateNewGame::render(){
	Renderer &renderer= Renderer::getInstance();

	renderer.renderButton(&buttonCustomGame);
	renderer.renderButton(&buttonScenario);
	renderer.renderButton(&buttonTutorial);
	renderer.renderButton(&buttonReturn);

	if(program != NULL) program->renderProgramMsgBox();
}

void MenuStateNewGame::update(){
	if(Config::getInstance().getBool("AutoTest")){
		AutoTest::getInstance().updateNewGame(program, mainMenu);
	}
}

void MenuStateNewGame::keyDown(char key) {
	Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
	if(key == configKeys.getCharKey("SaveGUILayout")) {
		bool saved = GraphicComponent::saveAllCustomProperties(containerName);
		//Lang &lang= Lang::getInstance();
		//console.addLine(lang.get("GUILayoutSaved") + " [" + (saved ? lang.get("Yes") : lang.get("No"))+ "]");
	}
}

}}//end namespace
