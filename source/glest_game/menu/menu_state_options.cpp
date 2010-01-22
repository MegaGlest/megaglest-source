// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Martiño Figueroa
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

	//create
	buttonReturn.init(200, 150, 125);	
	buttonAutoConfig.init(375, 150, 125);

	//labels
	labelVolumeFx.init(200, 530);
	labelVolumeAmbient.init(200, 500);
	labelVolumeMusic.init(200, 470);

	labelLang.init(200, 400);
	
	labelFilter.init(200, 340);
	labelShadows.init(200, 310);
	labelTextures3D.init(200, 280);
	labelLights.init(200, 250);

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

	//set text
	buttonReturn.setText(lang.get("Return"));
	buttonAutoConfig.setText(lang.get("AutoConfig"));
	labelLang.setText(lang.get("Language"));
	labelShadows.setText(lang.get("Shadows"));
	labelFilter.setText(lang.get("Filter"));
	labelTextures3D.setText(lang.get("Textures3D"));
	labelLights.setText(lang.get("MaxLights"));
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
	Lang &lang= Lang::getInstance();
	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	if(buttonReturn.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		mainMenu->setState(new MenuStateRoot(program, mainMenu));
    }  
	else if(buttonAutoConfig.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		Renderer::getInstance().autoConfig();
		saveConfig();
		mainMenu->setState(new MenuStateOptions(program, mainMenu));
	}
	else if(listBoxLang.mouseClick(x, y)){
		config.setString("Lang", listBoxLang.getSelectedItem());
		lang.loadStrings(config.getString("Lang"));
		saveConfig();
		mainMenu->setState(new MenuStateOptions(program, mainMenu));
		
	}
	else if(listBoxShadows.mouseClick(x, y)){
		int index= listBoxShadows.getSelectedItemIndex();
		config.setString("Shadows", Renderer::shadowsToStr(static_cast<Renderer::Shadows>(index)));
		saveConfig();
	}
	else if(listBoxFilter.mouseClick(x, y)){
		config.setString("Filter", listBoxFilter.getSelectedItem());
		saveConfig();
	}
	else if(listBoxTextures3D.mouseClick(x, y)){
		config.setInt("Textures3D", listBoxTextures3D.getSelectedItemIndex());
		saveConfig();
	}
	else if(listBoxLights.mouseClick(x, y)){
		config.setInt("MaxLights", listBoxLights.getSelectedItemIndex()+1);
		saveConfig();
	}
	else if(listBoxVolumeFx.mouseClick(x, y)){
		config.setString("SoundVolumeFx", listBoxVolumeFx.getSelectedItem());
		saveConfig();
	}
	else if(listBoxVolumeAmbient.mouseClick(x, y)){
		config.setString("SoundVolumeAmbient", listBoxVolumeAmbient.getSelectedItem());
		saveConfig();
	}
	else if(listBoxVolumeMusic.mouseClick(x, y)){
		CoreData::getInstance().getMenuMusic()->setVolume(strToInt(listBoxVolumeMusic.getSelectedItem())/100.f);
		config.setString("SoundVolumeMusic", listBoxVolumeMusic.getSelectedItem());
		saveConfig();
	}


}

void MenuStateOptions::mouseMove(int x, int y, const MouseState *ms){
	buttonReturn.mouseMove(x, y);
	buttonAutoConfig.mouseMove(x, y);
	listBoxLang.mouseMove(x, y);
	listBoxVolumeFx.mouseMove(x, y);
	listBoxVolumeAmbient.mouseMove(x, y);
	listBoxVolumeMusic.mouseMove(x, y);
	listBoxLang.mouseMove(x, y);
	listBoxFilter.mouseMove(x, y);
	listBoxShadows.mouseMove(x, y);
	listBoxTextures3D.mouseMove(x, y);
	listBoxLights.mouseMove(x, y);
}

void MenuStateOptions::render(){
	Renderer &renderer= Renderer::getInstance();

	renderer.renderButton(&buttonReturn);
	renderer.renderButton(&buttonAutoConfig);
	renderer.renderListBox(&listBoxLang);
	renderer.renderListBox(&listBoxShadows);
	renderer.renderListBox(&listBoxTextures3D);
	renderer.renderListBox(&listBoxLights);
	renderer.renderListBox(&listBoxFilter);
	renderer.renderListBox(&listBoxVolumeFx);
	renderer.renderListBox(&listBoxVolumeAmbient);
	renderer.renderListBox(&listBoxVolumeMusic);
	renderer.renderLabel(&labelLang);
	renderer.renderLabel(&labelShadows);
	renderer.renderLabel(&labelTextures3D);
	renderer.renderLabel(&labelLights);
	renderer.renderLabel(&labelFilter);
	renderer.renderLabel(&labelVolumeFx);
	renderer.renderLabel(&labelVolumeAmbient);
	renderer.renderLabel(&labelVolumeMusic);
}

void MenuStateOptions::saveConfig(){
	Config &config= Config::getInstance();

	config.save();
	Renderer::getInstance().loadConfig();
	SoundRenderer::getInstance().loadConfig();
}

}}//end namespace
