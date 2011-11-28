// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 MartiC1o Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#include "menu_state_about.h"

#include "renderer.h"
#include "menu_state_root.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "config.h"
#include "menu_state_options.h"
#include <iostream>
#include "metrics.h"
#include "cache_manager.h"

#include "leak_dumper.h"

using namespace std;

namespace Glest {
namespace Game {

// =====================================================
// 	class MenuStateAbout
// =====================================================

MenuStateAbout::MenuStateAbout(Program *program, MainMenu *mainMenu) :
	MenuState(program, mainMenu, "about") {

	containerName= "About";
	Lang &lang= Lang::getInstance();

	adjustModelText = true;

	string additionalCredits= loadAdditionalCredits();

	//init
	buttonReturn.registerGraphicComponent(containerName, "buttonReturn");
	buttonReturn.init(460, 100, 125);
	buttonReturn.setText(lang.get("Return"));

	labelAdditionalCredits.registerGraphicComponent(containerName, "labelAdditionalCredits");
	labelAdditionalCredits.init(500, 700);
	labelAdditionalCredits.setText(additionalCredits);

	if(additionalCredits == "") {
		for(int i= 0; i < aboutStringCount1; ++i) {
			labelAbout1[i].registerGraphicComponent(containerName, "labelAbout1" + intToStr(i));
			labelAbout1[i].init(100, 700 - i * 20);
			labelAbout1[i].setText(getAboutString1(i));
		}

		for(int i= 0; i < aboutStringCount2; ++i) {
			labelAbout2[i].registerGraphicComponent(containerName, "labelAbout2" + intToStr(i));
			labelAbout2[i].init(450, 620 - i * 20);
			labelAbout2[i].setText(getAboutString2(i));
		}
	}
	else {
		for(int i= 0; i < aboutStringCount1; ++i) {
			labelAbout1[i].registerGraphicComponent(containerName, "labelAbout1" + intToStr(i));
			labelAbout1[i].init(100, 700 - i * 20);
			labelAbout1[i].setText(getAboutString1(i));
		}

		for(int i= 0; i < aboutStringCount2; ++i) {
			labelAbout2[i].registerGraphicComponent(containerName, "labelAbout2" + intToStr(i));
			labelAbout2[i].init(100, 620 - i * 20);
			labelAbout2[i].setText(getAboutString2(i));
		}
	}

	for(int i= 0; i < teammateCount; ++i) {
		int xPos = (182 + i * 138);
		labelTeammateName[i].registerGraphicComponent(containerName, "labelTeammateName" + intToStr(i));
		labelTeammateName[i].init(xPos, 500);
		labelTeammateRole[i].registerGraphicComponent(containerName, "labelTeammateRole" + intToStr(i));
		labelTeammateRole[i].init(xPos, 520);

		labelTeammateName[i].setText(getTeammateName(i));
		labelTeammateRole[i].setText(getTeammateRole(i));
	}

	for(int i = teammateTopLineCount; i < teammateCount; ++i) {
		labelTeammateName[i].init(202 + (i-5) * 138, 160);
		labelTeammateRole[i].init(202 + (i-5) * 138, 180);
	}
	labelTeammateName[8].init(labelTeammateName[4].getX(), 160);
	labelTeammateRole[8].init(labelTeammateRole[4].getX(), 180);

	GraphicComponent::applyAllCustomProperties(containerName);
}

void MenuStateAbout::reloadUI() {
	Lang &lang= Lang::getInstance();

	adjustModelText = true;
	string additionalCredits= loadAdditionalCredits();

	buttonReturn.setText(lang.get("Return"));
	labelAdditionalCredits.setText(additionalCredits);

	if(additionalCredits == "") {
		for(int i= 0; i < aboutStringCount1; ++i){
			labelAbout1[i].setText(getAboutString1(i));
		}

		for(int i= 0; i < aboutStringCount2; ++i){
			labelAbout2[i].setText(getAboutString2(i));
		}
	}
	else {
		for(int i= 0; i < aboutStringCount1; ++i){
			labelAbout1[i].setText(getAboutString1(i));
		}

		for(int i= 0; i < aboutStringCount2; ++i){
			labelAbout2[i].setText(getAboutString2(i));
		}
	}

	for(int i= 0; i < teammateCount; ++i) {
		labelTeammateName[i].setText(getTeammateName(i));
		labelTeammateRole[i].setText(getTeammateRole(i));
	}

	GraphicComponent::reloadFontsForRegisterGraphicComponents(containerName);
}

string MenuStateAbout::loadAdditionalCredits(){
	string data_path= getGameReadWritePath(GameConstants::path_data_CacheLookupKey);
	if(data_path != ""){
		endPathWithSlash(data_path);
	}
	string result= "";
	const string dir= getGameCustomCoreDataPath(data_path,"data/core/menu/credits.txt");
	//printf("dir [%s]\n",dir.c_str());

	if(fileExists(dir) == true) {
#if defined(WIN32) && !defined(__MINGW32__)
		FILE *fp = _wfopen(utf8_decode(dir).c_str(), L"r");
		ifstream file(fp);
#else
		ifstream file(dir.c_str());
#endif
		std::string buffer;
		while(!file.eof()){
			getline(file, buffer);
			result+= buffer + "\n";
		}
		std::cout << buffer << std::endl;
		file.close();
#if defined(WIN32) && !defined(__MINGW32__)
		fclose(fp);
#endif
	}
	return result;
}

void MenuStateAbout::mouseClick(int x, int y, MouseButton mouseButton){

	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	if(buttonReturn.mouseClick(x, y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		mainMenu->setState(new MenuStateRoot(program, mainMenu));
	}

}

void MenuStateAbout::mouseMove(int x, int y, const MouseState *ms){
	buttonReturn.mouseMove(x, y);
}

void MenuStateAbout::render() {
	Renderer &renderer= Renderer::getInstance();

	renderer.renderLabel(&labelAdditionalCredits);
	renderer.renderButton(&buttonReturn);

	for(int i= 0; i < aboutStringCount1; ++i) {
		renderer.renderLabel(&labelAbout1[i]);
	}
	for(int i= 0; i < aboutStringCount2; ++i) {
		renderer.renderLabel(&labelAbout2[i]);
	}

	if(adjustModelText == true) {
		std::vector<Vec3f> &characterMenuScreenPositionListCache =
				CacheManager::getCachedItem< std::vector<Vec3f> >(GameConstants::characterMenuScreenPositionListCacheLookupKey);

		for(int i= 0; i < teammateCount; ++i) {
			int characterPos = (i % teammateTopLineCount);
			if(characterPos < characterMenuScreenPositionListCache.size()) {
				adjustModelText = false;

				int xPos = characterMenuScreenPositionListCache[characterPos].x;
				if(i == 7 && characterPos+1 < characterMenuScreenPositionListCache.size()) {
					xPos += ((characterMenuScreenPositionListCache[characterPos+1].x - characterMenuScreenPositionListCache[characterPos].x) / 2);
				}
				else if(i == 8 && characterPos+1 < characterMenuScreenPositionListCache.size()) {
					xPos = characterMenuScreenPositionListCache[characterPos+1].x;
				}

				FontMetrics *fontMetrics= NULL;
				if(Renderer::renderText3DEnabled == false) {
					fontMetrics= labelTeammateName[i].getFont()->getMetrics();
				}
				else {
					fontMetrics= labelTeammateName[i].getFont3D()->getMetrics();
				}
				int newxPos = xPos - (fontMetrics->getTextWidth(labelTeammateName[i].getText()) / 2);
				if(newxPos != labelTeammateName[i].getX()) {
					labelTeammateName[i].init(newxPos, labelTeammateName[i].getY());
				}

				newxPos = xPos - (fontMetrics->getTextWidth(labelTeammateRole[i].getText()) / 2);
				if(newxPos != labelTeammateRole[i].getX()) {
					labelTeammateRole[i].init(newxPos, labelTeammateRole[i].getY());
				}
			}
		}
	}

	for(int i= 0; i < teammateCount; ++i) {
		renderer.renderLabel(&labelTeammateName[i]);
		renderer.renderLabel(&labelTeammateRole[i]);
	}

	if(program != NULL)
		program->renderProgramMsgBox();

}

void MenuStateAbout::keyDown(SDL_KeyboardEvent key){
	Config &configKeys= Config::getInstance(std::pair<ConfigType, ConfigType>(cfgMainKeys, cfgUserKeys));
	if(isKeyPressed(configKeys.getSDLKey("SaveGUILayout"),key) == true) {
		GraphicComponent::saveAllCustomProperties(containerName);
	}
}

}
}//end namespace
