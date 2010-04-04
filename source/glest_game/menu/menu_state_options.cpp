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
	//modeinfos=list<ModeInfo> ();
	Shared::Platform::getFullscreenVideoModes(&modeInfos);
	activeInputLabel=NULL;
	
	int leftline=670;
	int rightline=670;
	int leftLabelStart=250;
	int leftColumnStart=leftLabelStart+150;
	int rightLabelStart=500;
	int rightColumnStart=rightLabelStart+150;
	int buttonRowPos=80;
	int captionOffset=75;
	
	mainMessageBox.init(lang.get("Ok"));
	mainMessageBox.setEnabled(false);
	mainMessageBoxState=0;
	
	labelAudioSection.init(leftLabelStart+captionOffset, leftline);
	labelAudioSection.setFont(CoreData::getInstance().getMenuFontVeryBig());
	labelAudioSection.setText(lang.get("Audio"));
	leftline-=30;
	
	//soundboxes
	labelVolumeFx.init(leftLabelStart, leftline);
	labelVolumeFx.setText(lang.get("FxVolume"));
	listBoxVolumeFx.init(leftColumnStart, leftline, 80);
	leftline-=30;
	
	labelVolumeAmbient.init(leftLabelStart, leftline);
	listBoxVolumeAmbient.init(leftColumnStart, leftline, 80);
	labelVolumeAmbient.setText(lang.get("AmbientVolume"));
	leftline-=30;
	
	labelVolumeMusic.init(leftLabelStart, leftline);
	listBoxVolumeMusic.init(leftColumnStart, leftline, 80);
	labelVolumeMusic.setText(lang.get("MusicVolume"));
	leftline-=30;
	
	for(int i=0; i<=100; i+=5){
		listBoxVolumeFx.pushBackItem(intToStr(i));
		listBoxVolumeAmbient.pushBackItem(intToStr(i));
		listBoxVolumeMusic.pushBackItem(intToStr(i));
	}
	listBoxVolumeFx.setSelectedItem(intToStr(config.getInt("SoundVolumeFx")/5*5));
	listBoxVolumeAmbient.setSelectedItem(intToStr(config.getInt("SoundVolumeAmbient")/5*5));
	listBoxVolumeMusic.setSelectedItem(intToStr(config.getInt("SoundVolumeMusic")/5*5));
	
	
	leftline-=30;
	labelMiscSection.init(leftLabelStart+captionOffset, leftline);
	labelMiscSection.setFont(CoreData::getInstance().getMenuFontVeryBig());
	labelMiscSection.setText(lang.get("Misc"));
	leftline-=30;
	
	//lang
	labelLang.init(leftLabelStart, leftline);
	labelLang.setText(lang.get("Language"));
	listBoxLang.init(leftColumnStart, leftline, 170);
	vector<string> langResults;
	findAll("data/lang/*.lng", langResults, true);
	if(langResults.empty()){
        throw runtime_error("There is no lang file");
	}
    listBoxLang.setItems(langResults);
	listBoxLang.setSelectedItem(config.getString("Lang"));
	leftline-=30;
	
	//playerName
	labelPlayerNameLabel.init(leftLabelStart,leftline);
	labelPlayerNameLabel.setText(lang.get("Playername"));
	
	labelPlayerName.init(leftColumnStart,leftline);
	labelPlayerName.setText(config.getString("NetPlayerName",Socket::getHostName().c_str()));
	leftline-=30;
	
	// server port
	labelServerPortLabel.init(leftLabelStart,leftline);
	labelServerPortLabel.setText(lang.get("ServerPort"));
	labelServerPort.init(leftColumnStart,leftline);
	string port=intToStr(config.getInt("ServerPort"));
	if(port!="61357"){
		port=port +" ("+lang.get("NonStandardPort")+"!!)";
	}
	else{
		port=port +" ("+lang.get("StandardPort")+")";
	}
	
	labelServerPort.setText(port);

	leftline-=30;
	labelVideoSection.init(leftLabelStart+captionOffset, leftline);
	labelVideoSection.setFont(CoreData::getInstance().getMenuFontVeryBig());
	labelVideoSection.setText(lang.get("Video"));
	leftline-=30;

	//resolution
	labelScreenModes.init(leftLabelStart, leftline);
	labelScreenModes.setText(lang.get("Resolution"));
	listBoxScreenModes.init(leftColumnStart, leftline, 170);
	for(list<ModeInfo>::const_iterator it= modeInfos.begin(); it!=modeInfos.end(); ++it){
		listBoxScreenModes.pushBackItem((*it).getString());
	}
	listBoxScreenModes.setSelectedItem(config.getString("ScreenWidth")+"x"+
		config.getString("ScreenHeight")+"-"+intToStr(config.getInt("ColorBits")));
	leftline-=30;
	
	//filter
	labelFilter.init(leftLabelStart, leftline);
	labelFilter.setText(lang.get("Filter"));
	listBoxFilter.init(leftColumnStart, leftline, 170);
	listBoxFilter.pushBackItem("Bilinear");
	listBoxFilter.pushBackItem("Trilinear");
	listBoxFilter.setSelectedItem(config.getString("Filter"));
	leftline-=30;
	
	//shadows
	labelShadows.init(leftLabelStart, leftline);
	labelShadows.setText(lang.get("Shadows"));
	listBoxShadows.init(leftColumnStart, leftline, 170);
	for(int i= 0; i<Renderer::sCount; ++i){
		listBoxShadows.pushBackItem(lang.get(Renderer::shadowsToStr(static_cast<Renderer::Shadows>(i))));
	}
	string str= config.getString("Shadows");
	listBoxShadows.setSelectedItemIndex(clamp(Renderer::strToShadows(str), 0, Renderer::sCount-1));
	leftline-=30;
	
	//textures 3d
	labelTextures3D.init(leftLabelStart, leftline);
	listBoxTextures3D.init(leftColumnStart, leftline, 80);
	labelTextures3D.setText(lang.get("Textures3D"));
	listBoxTextures3D.pushBackItem(lang.get("No"));
	listBoxTextures3D.pushBackItem(lang.get("Yes"));
	listBoxTextures3D.setSelectedItemIndex(clamp(config.getBool("Textures3D"), false, true));
	leftline-=30;
	
	//lights
	labelLights.init(leftLabelStart, leftline);
	labelLights.setText(lang.get("MaxLights"));
	listBoxLights.init(leftColumnStart, leftline, 80);
	for(int i= 1; i<=8; ++i){
		listBoxLights.pushBackItem(intToStr(i));
	}
	listBoxLights.setSelectedItemIndex(clamp(config.getInt("MaxLights")-1, 0, 7));
	leftline-=30;
	
	//unit particles
	labelUnitParticles.init(leftLabelStart,leftline);
	labelUnitParticles.setText(lang.get("ShowUnitParticles"));
	listBoxUnitParticles.init(leftColumnStart,leftline,80);
	listBoxUnitParticles.pushBackItem(lang.get("No"));
	listBoxUnitParticles.pushBackItem(lang.get("Yes"));
	listBoxUnitParticles.setSelectedItemIndex(clamp(config.getBool("UnitParticles"), 0, 1));
	leftline-=30;

	// buttons
	buttonOk.init(200, buttonRowPos, 100);	
	buttonOk.setText(lang.get("Ok"));
	buttonAbort.setText(lang.get("Abort"));
	buttonAbort.init(310, buttonRowPos, 100);
	buttonAutoConfig.setText(lang.get("AutoConfig"));
	buttonAutoConfig.init(450, buttonRowPos, 125);

}

void MenuStateOptions::showMessageBox(const string &text, const string &header, bool toggle){
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



void MenuStateOptions::mouseClick(int x, int y, MouseButton mouseButton){

	Config &config= Config::getInstance();
	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	if(mainMessageBox.getEnabled()){
		int button= 1;
		if(mainMessageBox.mouseClick(x, y, button))
		{
			soundRenderer.playFx(coreData.getClickSoundA());
			if(button==1)
			{
				if(mainMessageBoxState==1)
				{
					mainMessageBox.setEnabled(false);
					saveConfig();
					mainMenu->setState(new MenuStateRoot(program, mainMenu));
				}
				else
					mainMessageBox.setEnabled(false);
			}
		}
	}
	else if(buttonOk.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		
		string currentResolution=config.getString("ScreenWidth")+"x"+config.getString("ScreenHeight")+"-"+intToStr(config.getInt("ColorBits"));
		string selectedResolution=listBoxScreenModes.getSelectedItem();
		if(currentResolution!=selectedResolution){
			mainMessageBoxState=1;
			Lang &lang= Lang::getInstance();
			showMessageBox(lang.get("RestartNeeded"), lang.get("ResolutionChanged"), false);
			return;
		}
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
		listBoxScreenModes.mouseClick(x, y);
	}
}

void MenuStateOptions::mouseMove(int x, int y, const MouseState *ms){
	if (mainMessageBox.getEnabled()) {
			mainMessageBox.mouseMove(x, y);
		}
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
	listBoxScreenModes.mouseMove(x, y);
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

	if(mainMessageBox.getEnabled()){
		renderer.renderMessageBox(&mainMessageBox);
	}
	else
	{
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
		renderer.renderLabel(&labelVideoSection);
		renderer.renderLabel(&labelAudioSection);
		renderer.renderLabel(&labelMiscSection);
		renderer.renderLabel(&labelScreenModes);
		renderer.renderListBox(&listBoxScreenModes);
		renderer.renderLabel(&labelServerPortLabel);
		renderer.renderLabel(&labelServerPort);
	}
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
	config.setBool("Textures3D", listBoxTextures3D.getSelectedItemIndex());
	config.setBool("UnitParticles", listBoxUnitParticles.getSelectedItemIndex());
	config.setInt("MaxLights", listBoxLights.getSelectedItemIndex()+1);
	config.setString("SoundVolumeFx", listBoxVolumeFx.getSelectedItem());
	config.setString("SoundVolumeAmbient", listBoxVolumeAmbient.getSelectedItem());
	CoreData::getInstance().getMenuMusic()->setVolume(strToInt(listBoxVolumeMusic.getSelectedItem())/100.f);
	config.setString("SoundVolumeMusic", listBoxVolumeMusic.getSelectedItem());
	
	//just for the moment ....
	string currentResolution=config.getString("ScreenWidth")+"x"+config.getString("ScreenHeight");
	string selectedResolution=listBoxScreenModes.getSelectedItem();
	if(currentResolution!=selectedResolution){
		for(list<ModeInfo>::const_iterator it= modeInfos.begin(); it!=modeInfos.end(); ++it){
			if((*it).getString()==selectedResolution)
			{
				config.setInt("ScreenWidth",(*it).width);
				config.setInt("ScreenHeight",(*it).height);
				config.setInt("ColorBits",(*it).depth);
			}
		}
	}
	
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
