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

#include "menu_state_root.h"

#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "config.h"
#include "menu_state_new_game.h"
#include "menu_state_join_game.h"
#include "menu_state_options.h"
#include "menu_state_about.h"
#include "menu_state_masterserver.h"
#include "metrics.h"
#include "network_manager.h"
#include "network_message.h"
#include "socket.h"
#include "auto_test.h"

#include "leak_dumper.h"

namespace Glest{ namespace Game{

// =====================================================
// 	class MenuStateRoot
// =====================================================

MenuStateRoot::MenuStateRoot(Program *program, MainMenu *mainMenu):
	MenuState(program, mainMenu, "root")
{
	containerName = "MainMenu";
	Lang &lang= Lang::getInstance();
	int yPos=440;


	labelVersion.registerGraphicComponent(containerName,"labelVersion");
	if(EndsWith(glestVersionString, "-dev") == false){
		labelVersion.init(525, yPos);
		labelVersion.setText(glestVersionString);
	}
	else {
		labelVersion.init(405, yPos);
		labelVersion.setText(glestVersionString + " [" + getCompileDateTime() + ", " + getSVNRevisionString() + "]");
	}

	yPos-=55;
	buttonNewGame.registerGraphicComponent(containerName,"buttonNewGame");
	buttonNewGame.init(425, yPos, 150);
	yPos-=40;
	buttonJoinGame.registerGraphicComponent(containerName,"buttonJoinGame");
    buttonJoinGame.init(425, yPos, 150);
    yPos-=40;
    buttonMasterserverGame.registerGraphicComponent(containerName,"buttonMasterserverGame");
    buttonMasterserverGame.init(425, yPos, 150);
    yPos-=40;
    buttonOptions.registerGraphicComponent(containerName,"buttonOptions");
    buttonOptions.init(425, yPos, 150);
    yPos-=40;
    buttonAbout.registerGraphicComponent(containerName,"buttonAbout");
    buttonAbout.init(425, yPos , 150);
    yPos-=40;
    buttonExit.registerGraphicComponent(containerName,"buttonExit");
    buttonExit.init(425, yPos, 150);

	buttonNewGame.setText(lang.get("NewGame"));
	buttonJoinGame.setText(lang.get("JoinGame"));
	buttonMasterserverGame.setText(lang.get("JoinInternetGame"));
	buttonOptions.setText(lang.get("Options"));
	buttonAbout.setText(lang.get("About"));
	buttonExit.setText(lang.get("Exit"));
	
	//mesage box
	mainMessageBox.registerGraphicComponent(containerName,"mainMessageBox");
	mainMessageBox.init(lang.get("Yes"), lang.get("No"));
	mainMessageBox.setEnabled(false);

	GraphicComponent::applyAllCustomProperties(containerName);
}

void MenuStateRoot::mouseClick(int x, int y, MouseButton mouseButton){

	CoreData &coreData=  CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	if(buttonNewGame.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundB());
		mainMenu->setState(new MenuStateNewGame(program, mainMenu));
    }
	else if(buttonJoinGame.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundB());
		mainMenu->setState(new MenuStateJoinGame(program, mainMenu));
    }
	else if(buttonMasterserverGame.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundB());
		mainMenu->setState(new MenuStateMasterserver(program, mainMenu));
    }
    else if(buttonOptions.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundB());
		mainMenu->setState(new MenuStateOptions(program, mainMenu));
    }
    else if(buttonAbout.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundB());
		mainMenu->setState(new MenuStateAbout(program, mainMenu));
    }
    else if(buttonExit.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		program->exit();
    }
	//exit message box, has to be the last thing to do in this function
    else if(mainMessageBox.getEnabled()){
		int button= 1;
		if(mainMessageBox.mouseClick(x, y, button)) {
			if(button==1) {
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
				soundRenderer.playFx(coreData.getClickSoundA());
				program->exit();
			}
			else {
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
				//close message box
				mainMessageBox.setEnabled(false);
			}
		}
	}
}

void MenuStateRoot::mouseMove(int x, int y, const MouseState *ms){
	buttonNewGame.mouseMove(x, y);
    buttonJoinGame.mouseMove(x, y);
    buttonMasterserverGame.mouseMove(x, y);
    buttonOptions.mouseMove(x, y);
    buttonAbout.mouseMove(x, y);
    buttonExit.mouseMove(x,y);
	if (mainMessageBox.getEnabled()) {
		mainMessageBox.mouseMove(x, y);
	}

}

void MenuStateRoot::render() {
	Renderer &renderer= Renderer::getInstance();
	CoreData &coreData= CoreData::getInstance();
	const Metrics &metrics= Metrics::getInstance();

	int w= 300;
	int h= 150;
	int yPos=495;

	renderer.renderTextureQuad(
		(metrics.getVirtualW()-w)/2, yPos-h/2, w, h,
		coreData.getLogoTexture(), GraphicComponent::getFade());

	int maxLogoWidth=0;
	for(int idx = 0; idx < coreData.getLogoTextureExtraCount(); ++idx) {
		Texture2D *extraLogo = coreData.getLogoTextureExtra(idx);
		maxLogoWidth += extraLogo->getPixmap()->getW();
	}

	int currentX = (metrics.getVirtualW()-maxLogoWidth)/2;
	int currentY = 50;
	for(int idx = 0; idx < coreData.getLogoTextureExtraCount(); ++idx) {
		Texture2D *extraLogo = coreData.getLogoTextureExtra(idx);

		renderer.renderTextureQuad(
				currentX, currentY,
				extraLogo->getPixmap()->getW(), extraLogo->getPixmap()->getH(),
				extraLogo, GraphicComponent::getFade());

		currentX += extraLogo->getPixmap()->getW();
	}
	renderer.renderButton(&buttonNewGame);
	renderer.renderButton(&buttonJoinGame);
	renderer.renderButton(&buttonMasterserverGame);
	renderer.renderButton(&buttonOptions);
	renderer.renderButton(&buttonAbout);
	renderer.renderButton(&buttonExit);
	renderer.renderLabel(&labelVersion);

	//exit message box
	if(mainMessageBox.getEnabled()){
		renderer.renderMessageBox(&mainMessageBox);
	}
	if(program != NULL) program->renderProgramMsgBox();
}

void MenuStateRoot::update(){
	if(Config::getInstance().getBool("AutoTest")){
		AutoTest::getInstance().updateRoot(program, mainMenu);
	}
}

void MenuStateRoot::keyDown(char key) {

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = [%c] [%d]\n",__FILE__,__FUNCTION__,__LINE__,key,key);

	Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
	//exit
	if(key == configKeys.getCharKey("ExitKey")) {
		Lang &lang= Lang::getInstance();
		showMessageBox(lang.get("ExitGame?"), "", true);
	}
	else if(mainMessageBox.getEnabled() == true && key == vkReturn) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		program->exit();
	}
	else if(key == configKeys.getCharKey("SaveGUILayout")) {
		bool saved = GraphicComponent::saveAllCustomProperties(containerName);
		//Lang &lang= Lang::getInstance();
		//console.addLine(lang.get("GUILayoutSaved") + " [" + (saved ? lang.get("Yes") : lang.get("No"))+ "]");
	}

}

void MenuStateRoot::showMessageBox(const string &text, const string &header, bool toggle){
	if(!toggle){
		mainMessageBox.setEnabled(false);
	}

	if(!mainMessageBox.getEnabled()){
		mainMessageBox.setText(text);
		mainMessageBox.setHeader(header);
		mainMessageBox.setEnabled(true);
	}
	else{
		mainMessageBox.setEnabled(false);
	}
}

}}//end namespace
