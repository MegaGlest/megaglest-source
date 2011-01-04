// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2011-  by Titus Tscharntke
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "menu_state_keysetup.h"

#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "config.h"
#include "menu_state_options.h"
#include "menu_state_root.h"
#include "metrics.h"

#include "leak_dumper.h"

namespace Glest{ namespace Game{


// =====================================================
// 	class MenuStateKeysetup
// =====================================================

MenuStateKeysetup::MenuStateKeysetup(Program *program, MainMenu *mainMenu):
	MenuState(program, mainMenu, "config")
{
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	containerName = "KeySetup";

	Lang &lang= Lang::getInstance();
	int buttonRowPos=80;
	// header
	labelTitle.registerGraphicComponent(containerName,"labelTitle");
	labelTitle.init(330,700);
	labelTitle.setFont(CoreData::getInstance().getMenuFontBig());
	labelTitle.setText(lang.get("Keyboardsetup"));

	// mainMassegeBox
	mainMessageBox.registerGraphicComponent(containerName,"mainMessageBox");
	mainMessageBox.init(lang.get("Ok"));
	mainMessageBox.setEnabled(false);
	mainMessageBoxState=0;

	keyScrollBar.init(800,200,false,200,20);
	keyScrollBar.setLength(400);
	keyScrollBar.setElementCount(0);
	keyScrollBar.setVisibleSize(keyButtonsToRender);
	keyScrollBar.setVisibleStart(0);


	// buttons
	buttonOk.registerGraphicComponent(containerName,"buttonOk");
	buttonOk.init(200, buttonRowPos, 100);
	buttonOk.setText(lang.get("Ok"));

	buttonReturn.registerGraphicComponent(containerName,"buttonReturn");
	buttonReturn.init(310, buttonRowPos, 100);
	buttonReturn.setText(lang.get("Return"));

	keyButtonsLineHeight=25;
	keyButtonsHeight=20;
	keyButtonsWidth=200;
	keyButtonsXBase=300;
	keyButtonsYBase=200+400-keyButtonsLineHeight;
	keyButtonsToRender=400/keyButtonsLineHeight;
	int labelWidth=100;

	Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
	mergedProperties=configKeys.getMergedProperties();
	masterProperties=configKeys.getMasterProperties();
	userProperties=configKeys.getUserProperties();

	for(int i = 0; i < mergedProperties.size(); ++i) {
		GraphicButton *button=new GraphicButton();
		button->init(keyButtonsXBase, keyButtonsYBase, keyButtonsWidth,keyButtonsHeight);
		button->setText(mergedProperties[i].first);
		keyButtons.push_back(button);
		GraphicLabel *label=new GraphicLabel();
		label->init(keyButtonsXBase+keyButtonsWidth+10,keyButtonsYBase,labelWidth,20);
		label->setText(mergedProperties[i].second);
		labels.push_back(label);
	}

	keyScrollBar.init(keyButtonsXBase+keyButtonsWidth+labelWidth+20,200,false,200,20);
	keyScrollBar.setLength(400);
	keyScrollBar.setElementCount(keyButtons.size());
	keyScrollBar.setVisibleSize(keyButtonsToRender);
	keyScrollBar.setVisibleStart(0);

}


void MenuStateKeysetup::cleanup() {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	clearUserButtons();
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] END\n",__FILE__,__FUNCTION__,__LINE__);
}

MenuStateKeysetup::~MenuStateKeysetup() {
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	cleanup();
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] END\n",__FILE__,__FUNCTION__,__LINE__);
}

void MenuStateKeysetup::clearUserButtons() {
	while(!keyButtons.empty()) {
		delete keyButtons.back();
		keyButtons.pop_back();
	}
	while(!labels.empty()) {
			delete labels.back();
			labels.pop_back();
		}
}

void MenuStateKeysetup::mouseClick(int x, int y, MouseButton mouseButton){
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	if(mainMessageBox.getEnabled()){
		int button= 1;
		if(mainMessageBox.mouseClick(x, y, button))
		{
			soundRenderer.playFx(coreData.getClickSoundA());
			if(button==1)
			{
				mainMessageBox.setEnabled(false);
			}
		}
	}
	else if(keyScrollBar.mouseClick(x, y)){
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			soundRenderer.playFx(coreData.getClickSoundB());
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	    }
    else if(buttonReturn.mouseClick(x, y)){
    	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		soundRenderer.playFx(coreData.getClickSoundB());
        SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		mainMenu->setState(new MenuStateOptions(program, mainMenu));
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    }
    else if(buttonOk.mouseClick(x, y)){
    	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		soundRenderer.playFx(coreData.getClickSoundB());
        SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		mainMenu->setState(new MenuStateOptions(program, mainMenu));
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
    }
    else {
    	if ( keyScrollBar.getElementCount() != 0) {
			for (int i = keyScrollBar.getVisibleStart(); i
					<= keyScrollBar.getVisibleEnd(); ++i) {
				if (keyButtons[i]->mouseClick(x, y)) {
					break;
				}
			}
		}
    }
}

void MenuStateKeysetup::mouseMove(int x, int y, const MouseState *ms){
    buttonReturn.mouseMove(x, y);
    buttonOk.mouseMove(x, y);
    if (ms->get(mbLeft)) {
		keyScrollBar.mouseDown(x, y);
	} else {
		keyScrollBar.mouseMove(x, y);
	}

    if(keyScrollBar.getElementCount()!=0 ) {
    	for(int i = keyScrollBar.getVisibleStart(); i <= keyScrollBar.getVisibleEnd(); ++i) {
    		keyButtons[i]->mouseMove(x, y);
    	}
    }

}

void MenuStateKeysetup::render(){
	Renderer &renderer= Renderer::getInstance();

	if(mainMessageBox.getEnabled()) {
		renderer.renderMessageBox(&mainMessageBox);
	}
	else
	{
		renderer.renderButton(&buttonReturn);
		renderer.renderButton(&buttonOk);
		renderer.renderLabel(&labelTitle);

		if(keyScrollBar.getElementCount()!=0 ) {
			for(int i = keyScrollBar.getVisibleStart(); i <= keyScrollBar.getVisibleEnd(); ++i) {
				renderer.renderButton(keyButtons[i]);
				renderer.renderLabel(labels[i]);
			}
		}
		renderer.renderScrollBar(&keyScrollBar);
	}
	if(program != NULL) program->renderProgramMsgBox();
}

void MenuStateKeysetup::update() {
	if (keyScrollBar.getElementCount() != 0) {
		for (int i = keyScrollBar.getVisibleStart(); i
				<= keyScrollBar.getVisibleEnd(); ++i) {
			keyButtons[i]->setY(keyButtonsYBase - keyButtonsLineHeight * (i
					- keyScrollBar.getVisibleStart()));
			labels[i]->setY(keyButtonsYBase - keyButtonsLineHeight * (i
					- keyScrollBar.getVisibleStart()));
		}
	}
}



void MenuStateKeysetup::showMessageBox(const string &text, const string &header, bool toggle){
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


void MenuStateKeysetup::keyDown(char key) {
	//Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
}

void MenuStateKeysetup::keyPress(char c) {

}
void MenuStateKeysetup::keyUp(char key) {

}

}}//end namespace
