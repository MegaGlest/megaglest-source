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

#include "menu_state_options.h"

#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "config.h"
#include "menu_state_root.h"
#include "util.h"

#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class MenuStateOptions
// =====================================================

MenuStateOptions::MenuStateOptions(Program *program, MainMenu *mainMenu):
	MenuState(program, mainMenu, "config")
{
	Lang &lang= Lang::getInstance();
	Config &config= Config::getInstance();
	activeInputLabel=NULL;

	//create
	buttonOk.init(200, 150, 100);
	buttonAbort.init(310, 150, 100);
	buttonAutoConfig.init(450, 150, 125);

	//labels
	labelVolumeFx.init(200, 530);
	labelVolumeAmbient.init(200, 500);
	labelVolumeMusic.init(200, 470);

	labelLang.init(200, 400);
	labelPlayerNameLabel.init(200,370);
	labelPlayerName.init(350,370);

	labelFilter.init(200, 340);
	labelShadows.init(200, 310);
	labelTextures3D.init(200, 280);
	labelLights.init(200, 250);
	labelUnitParticles.init(200,220);

	//list boxes
	listBoxVolumeFx.init(350, 530, 80);
	listBoxVolumeAmbient.init(350, 500, 80);
	listBoxVolumeMusic.init(350, 470, 80);
	listBoxMusicSelect.init(350, 440, 150);

	listBoxLang.init(350, 400, 170);

	listBoxFilter.init(350, 340, 170);
	listBoxShadows.init(350, 310, 170);
	listBoxTextures3D.init(350, 280, 80);
	listBoxLights.init(350, 250, 80);
	listBoxUnitParticles.init(350,220,80);

	//set text
	buttonOk.setText(lang.get("Ok"));
	buttonAbort.setText(lang.get("Abort"));
	buttonAutoConfig.setText(lang.get("AutoConfig"));
	labelLang.setText(lang.get("Language"));
	labelPlayerNameLabel.setText(lang.get("Playername"));
	labelShadows.setText(lang.get("Shadows"));
	labelFilter.setText(lang.get("Filter"));
	labelTextures3D.setText(lang.get("Textures3D"));
	labelLights.setText(lang.get("MaxLights"));
	labelUnitParticles.setText(lang.get("ShowUnitParticles"));
	labelVolumeFx.setText(lang.get("FxVolume"));
	labelVolumeAmbient.setText(lang.get("AmbientVolume"));
	labelVolumeMusic.setText(lang.get("MusicVolume"));

	//sound

	//lang
	vector<string> langResults;
	findAll("data/lang/*.lng", langResults, true);
	if(langResults.empty()){
        throw runtime_error("There is no lang file");
	}
    listBoxLang.setItems(langResults);
	listBoxLang.setSelectedItem(config.getString("Lang"));

	//playerName
	labelPlayerName.setText(config.getString("NetPlayerName",Socket::getHostName().c_str()));

	//shadows
	for(int i= 0; i<Renderer::sCount; ++i){
		listBoxShadows.pushBackItem(lang.get(Renderer::shadowsToStr(static_cast<Renderer::Shadows>(i))));
	}

	string str= config.getString("Shadows");
	listBoxShadows.setSelectedItemIndex(clamp(Renderer::strToShadows(str), 0, Renderer::sCount-1));

	//filter
	listBoxFilter.pushBackItem("Bilinear");
	listBoxFilter.pushBackItem("Trilinear");
	listBoxFilter.setSelectedItem(config.getString("Filter"));

	//textures 3d
	listBoxTextures3D.pushBackItem(lang.get("No"));
	listBoxTextures3D.pushBackItem(lang.get("Yes"));
	listBoxTextures3D.setSelectedItemIndex(clamp(config.getInt("Textures3D"), 0, 1));

	//textures 3d
	listBoxUnitParticles.pushBackItem(lang.get("No"));
	listBoxUnitParticles.pushBackItem(lang.get("Yes"));
	listBoxUnitParticles.setSelectedItemIndex(clamp(config.getBool("UnitParticles"), 0, 1));

	//lights
	for(int i= 1; i<=8; ++i){
		listBoxLights.pushBackItem(intToStr(i));
	}
	listBoxLights.setSelectedItemIndex(clamp(config.getInt("MaxLights")-1, 0, 7));

	//sound
	for(int i=0; i<=100; i+=5){
		listBoxVolumeFx.pushBackItem(intToStr(i));
		listBoxVolumeAmbient.pushBackItem(intToStr(i));
		listBoxVolumeMusic.pushBackItem(intToStr(i));
	}
	listBoxVolumeFx.setSelectedItem(intToStr(config.getInt("SoundVolumeFx")/5*5));
	listBoxVolumeAmbient.setSelectedItem(intToStr(config.getInt("SoundVolumeAmbient")/5*5));
	listBoxVolumeMusic.setSelectedItem(intToStr(config.getInt("SoundVolumeMusic")/5*5));

}

void MenuStateOptions::mouseClick(int x, int y, MouseButton mouseButton){

	Config &config= Config::getInstance();
	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	if(buttonOk.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		saveConfig();
		mainMenu->setState(new MenuStateRoot(program, mainMenu));
    }
	else if(buttonAbort.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		mainMenu->setState(new MenuStateRoot(program, mainMenu));
    }
	else if(buttonAutoConfig.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		Renderer::getInstance().autoConfig();
		saveConfig();
		mainMenu->setState(new MenuStateOptions(program, mainMenu));
	}
	else if(labelPlayerName.mouseClick(x, y) && ( activeInputLabel != &labelPlayerName )){
			setActiveInputLable(&labelPlayerName);
	}
	else
	{
		listBoxLang.mouseClick(x, y);
		listBoxShadows.mouseClick(x, y);
		listBoxFilter.mouseClick(x, y);
		listBoxTextures3D.mouseClick(x, y);
		listBoxUnitParticles.mouseClick(x, y);
		listBoxLights.mouseClick(x, y);
		listBoxVolumeFx.mouseClick(x, y);
		listBoxVolumeAmbient.mouseClick(x, y);
		listBoxVolumeMusic.mouseClick(x, y);
	}
}

void MenuStateOptions::mouseMove(int x, int y, const MouseState *ms){
	buttonOk.mouseMove(x, y);
	buttonAbort.mouseMove(x, y);
	buttonAutoConfig.mouseMove(x, y);
	listBoxLang.mouseMove(x, y);
	listBoxVolumeFx.mouseMove(x, y);
	listBoxVolumeAmbient.mouseMove(x, y);
	listBoxVolumeMusic.mouseMove(x, y);
	listBoxLang.mouseMove(x, y);
	listBoxFilter.mouseMove(x, y);
	listBoxShadows.mouseMove(x, y);
	listBoxTextures3D.mouseMove(x, y);
	listBoxUnitParticles.mouseMove(x, y);
	listBoxLights.mouseMove(x, y);
}

void MenuStateOptions::keyDown(char key){
	if(activeInputLabel!=NULL)
	{
		if(key==vkBack){
			string text= activeInputLabel->getText();
			if(text.size()>1){
				text.erase(text.end()-2);
			}
			activeInputLabel->setText(text);
		}
	}
}

void MenuStateOptions::keyPress(char c){
	if(activeInputLabel!=NULL)
	{
		int maxTextSize= 16;
		if(&labelPlayerName==activeInputLabel){
			if((c>='0' && c<='9')||(c>='a' && c<='z')||(c>='A' && c<='Z')||
				(c=='-')||(c=='(')||(c==')')){
				if(activeInputLabel->getText().size()<maxTextSize){
					string text= activeInputLabel->getText();
					text.insert(text.end()-1, c);
					activeInputLabel->setText(text);
				}
			}
		}
	}
}

void MenuStateOptions::render(){
	Renderer &renderer= Renderer::getInstance();

	renderer.renderButton(&buttonOk);
	renderer.renderButton(&buttonAbort);
	renderer.renderButton(&buttonAutoConfig);
	renderer.renderListBox(&listBoxLang);
	renderer.renderListBox(&listBoxShadows);
	renderer.renderListBox(&listBoxTextures3D);
	renderer.renderListBox(&listBoxUnitParticles);
	renderer.renderListBox(&listBoxLights);
	renderer.renderListBox(&listBoxFilter);
	renderer.renderListBox(&listBoxVolumeFx);
	renderer.renderListBox(&listBoxVolumeAmbient);
	renderer.renderListBox(&listBoxVolumeMusic);
	renderer.renderLabel(&labelLang);
	renderer.renderLabel(&labelPlayerNameLabel);
	renderer.renderLabel(&labelPlayerName);
	renderer.renderLabel(&labelShadows);
	renderer.renderLabel(&labelTextures3D);
	renderer.renderLabel(&labelUnitParticles);
	renderer.renderLabel(&labelLights);
	renderer.renderLabel(&labelFilter);
	renderer.renderLabel(&labelVolumeFx);
	renderer.renderLabel(&labelVolumeAmbient);
	renderer.renderLabel(&labelVolumeMusic);
}

void MenuStateOptions::saveConfig(){
	Config &config= Config::getInstance();
	Lang &lang= Lang::getInstance();
	setActiveInputLable(NULL);

	if(labelPlayerName.getText().length()>0)
	{
		config.setString("NetPlayerName", labelPlayerName.getText());
	}
	//Copy values
	config.setString("Lang", listBoxLang.getSelectedItem());
	lang.loadStrings(config.getString("Lang"));

	int index= listBoxShadows.getSelectedItemIndex();
	config.setString("Shadows", Renderer::shadowsToStr(static_cast<Renderer::Shadows>(index)));

	config.setString("Filter", listBoxFilter.getSelectedItem());
	config.setInt("Textures3D", listBoxTextures3D.getSelectedItemIndex());
	config.setBool("UnitParticles", listBoxUnitParticles.getSelectedItemIndex());
	config.setInt("MaxLights", listBoxLights.getSelectedItemIndex()+1);
	config.setString("SoundVolumeFx", listBoxVolumeFx.getSelectedItem());
	config.setString("SoundVolumeAmbient", listBoxVolumeAmbient.getSelectedItem());
	CoreData::getInstance().getMenuMusic()->setVolume(strToInt(listBoxVolumeMusic.getSelectedItem())/100.f);
	config.setString("SoundVolumeMusic", listBoxVolumeMusic.getSelectedItem());

	config.save();
	Renderer::getInstance().loadConfig();
	SoundRenderer::getInstance().loadConfig();
}

void MenuStateOptions::setActiveInputLable(GraphicLabel *newLable)
{
	if(newLable!=NULL){
		string text= newLable->getText();
		size_t found;
		found=text.find_last_of("_");
		if (found==string::npos)
		{
			text=text+"_";
		}
		newLable->setText(text);
	}
	if(activeInputLabel!=NULL && !activeInputLabel->getText().empty()){
		string text= activeInputLabel->getText();
		size_t found;
		found=text.find_last_of("_");
		if (found!=string::npos)
		{
			text=text.substr(0,found);
		}
		activeInputLabel->setText(text);
	}
	activeInputLabel=newLable;
}

}}//end namespace
