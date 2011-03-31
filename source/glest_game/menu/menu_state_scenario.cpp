// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Martio Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "menu_state_scenario.h"

#include "renderer.h"
#include "menu_state_new_game.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "menu_state_options.h"
#include "network_manager.h"
#include "config.h"
#include "auto_test.h"
#include "game.h"

#include "leak_dumper.h"

namespace Glest{ namespace Game{

using namespace	Shared::Xml;

// =====================================================
// 	class MenuStateScenario
// =====================================================

MenuStateScenario::MenuStateScenario(Program *program, MainMenu *mainMenu, const vector<string> &dirList, string autoloadScenarioName):
    MenuState(program, mainMenu, "scenario")
{
	containerName = "Scenario";
	enableScenarioTexturePreview = Config::getInstance().getBool("EnableScenarioTexturePreview","true");
	scenarioLogoTexture=NULL;
	previewLoadDelayTimer=time(NULL);
	needToLoadTextures=true;

	Lang &lang= Lang::getInstance();
	NetworkManager &networkManager= NetworkManager::getInstance();

	mainMessageBox.registerGraphicComponent(containerName,"mainMessageBox");
	mainMessageBox.init(lang.get("Ok"));
	mainMessageBox.setEnabled(false);
	mainMessageBoxState=0;

	this->autoloadScenarioName = autoloadScenarioName;
    vector<string> results;

	this->dirList = dirList;

	int startY=100;
	int startX=350;

	labelInfo.registerGraphicComponent(containerName,"labelInfo");
    labelInfo.init(startX, startY+130);
	labelInfo.setFont(CoreData::getInstance().getMenuFontNormal());

	buttonReturn.registerGraphicComponent(containerName,"buttonReturn");
    buttonReturn.init(startX, startY, 125);

    buttonPlayNow.registerGraphicComponent(containerName,"buttonPlayNow");
	buttonPlayNow.init(startX+175, startY, 125);

	listBoxScenario.registerGraphicComponent(containerName,"listBoxScenario");
    listBoxScenario.init(startX, startY+160, 190);

    labelScenario.registerGraphicComponent(containerName,"labelScenario");
	labelScenario.init(startX, startY+190);

	buttonReturn.setText(lang.get("Return"));
	buttonPlayNow.setText(lang.get("PlayNow"));

    labelScenario.setText(lang.get("Scenario"));

    //scenario listbox
	findDirs(dirList, results);
    scenarioFiles = results;
	if(results.size() == 0) {
        throw runtime_error("There are no scenarios found to load");
	}
	for(int i= 0; i<results.size(); ++i){
		results[i] = formatString(results[i]);
	}
    listBoxScenario.setItems(results);

    try {
        loadScenarioInfo(Scenario::getScenarioPath(dirList, scenarioFiles[listBoxScenario.getSelectedItemIndex()]), &scenarioInfo );
        labelInfo.setText(scenarioInfo.desc);

        GraphicComponent::applyAllCustomProperties(containerName);

        networkManager.init(nrServer);
    }
	catch(const std::exception &ex) {
		char szBuf[4096]="";
		sprintf(szBuf,"In [%s::%s %d] Error detected:\n%s\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);

        mainMessageBoxState=1;
        showMessageBox( "Error: " + string(ex.what()), "Error detected", false);
	}
}

MenuStateScenario::~MenuStateScenario() {
	cleanupPreviewTexture();
}

void MenuStateScenario::cleanupPreviewTexture() {
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] scenarioLogoTexture [%p]\n",__FILE__,__FUNCTION__,__LINE__,scenarioLogoTexture);

	if(scenarioLogoTexture != NULL) {
		Renderer::getInstance().endTexture(rsGlobal, scenarioLogoTexture, false);
	}
	scenarioLogoTexture = NULL;
}

void MenuStateScenario::mouseClick(int x, int y, MouseButton mouseButton) {
	CoreData &coreData= CoreData::getInstance();
	SoundRenderer &soundRenderer= SoundRenderer::getInstance();

	if(mainMessageBox.getEnabled()){
		int button= 1;
		if(mainMessageBox.mouseClick(x, y, button))
		{
			soundRenderer.playFx(coreData.getClickSoundA());
			if(button==1) {
				mainMessageBox.setEnabled(false);
			}
		}
	}
	else if(buttonReturn.mouseClick(x,y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		mainMenu->setState(new MenuStateNewGame(program, mainMenu));
    }
	else if(buttonPlayNow.mouseClick(x,y)){
		soundRenderer.playFx(coreData.getClickSoundC());
		launchGame();
	}
    else if(listBoxScenario.mouseClick(x, y)){
        try {
            loadScenarioInfo(Scenario::getScenarioPath(dirList, scenarioFiles[listBoxScenario.getSelectedItemIndex()]), &scenarioInfo);
            labelInfo.setText(scenarioInfo.desc);
        }
        catch(const std::exception &ex) {
            char szBuf[4096]="";
            sprintf(szBuf,"In [%s::%s %d] Error detected:\n%s\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
            SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
            if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);

            mainMessageBoxState=1;
            showMessageBox( "Error: " + string(ex.what()), "Error detected", false);
        }
	}
}

void MenuStateScenario::mouseMove(int x, int y, const MouseState *ms){

	if (mainMessageBox.getEnabled()) {
		mainMessageBox.mouseMove(x, y);
	}

	listBoxScenario.mouseMove(x, y);

	buttonReturn.mouseMove(x, y);
	buttonPlayNow.mouseMove(x, y);
}

void MenuStateScenario::render(){

	Renderer &renderer= Renderer::getInstance();

	if(scenarioLogoTexture != NULL) {
		renderer.renderTextureQuad(300,350,400,300,scenarioLogoTexture,1.0f);
		//renderer.renderBackground(scenarioLogoTexture);
	}

	if(mainMessageBox.getEnabled()) {
		renderer.renderMessageBox(&mainMessageBox);
	}
	else {
		renderer.renderLabel(&labelInfo);
		renderer.renderLabel(&labelScenario);
		renderer.renderListBox(&listBoxScenario);

		renderer.renderButton(&buttonReturn);
		renderer.renderButton(&buttonPlayNow);
	}
	if(program != NULL) program->renderProgramMsgBox();
}

void MenuStateScenario::update() {
	if(Config::getInstance().getBool("AutoTest")) {
		AutoTest::getInstance().updateScenario(this);
	}
	if(this->autoloadScenarioName != "") {
		listBoxScenario.setSelectedItem(formatString(this->autoloadScenarioName),false);

		if(listBoxScenario.getSelectedItem() != formatString(this->autoloadScenarioName)) {
			mainMessageBoxState=1;
			showMessageBox( "Could not find scenario name: " + formatString(this->autoloadScenarioName), "Scenario Missing", false);
			this->autoloadScenarioName = "";
		}
		else {
			loadScenarioInfo(Scenario::getScenarioPath(dirList, scenarioFiles[listBoxScenario.getSelectedItemIndex()]), &scenarioInfo);
			labelInfo.setText(scenarioInfo.desc);

			SoundRenderer &soundRenderer= SoundRenderer::getInstance();
			CoreData &coreData= CoreData::getInstance();
			soundRenderer.playFx(coreData.getClickSoundC());
			launchGame();
		}
	}

	if(needToLoadTextures) {
		// this delay is done to make it possible to switch faster
		if(difftime(time(NULL), previewLoadDelayTimer) >= 2){
			loadScenarioPreviewTexture();
			needToLoadTextures= false;
		}
	}
}

void MenuStateScenario::launchGame() {
	GameSettings gameSettings;
    loadGameSettings(&scenarioInfo, &gameSettings);
	program->setState(new Game(program, &gameSettings));
}

void MenuStateScenario::setScenario(int i) {
	listBoxScenario.setSelectedItemIndex(i);
	loadScenarioInfo(Scenario::getScenarioPath(dirList, scenarioFiles[listBoxScenario.getSelectedItemIndex()]), &scenarioInfo);
}

void MenuStateScenario::loadScenarioInfo(string file, ScenarioInfo *scenarioInfo) {
    Lang &lang= Lang::getInstance();

    XmlTree xmlTree;
	xmlTree.load(file);

    const XmlNode *scenarioNode= xmlTree.getRootNode();
	const XmlNode *difficultyNode= scenarioNode->getChild("difficulty");
	scenarioInfo->difficulty = difficultyNode->getAttribute("value")->getIntValue();
	if( scenarioInfo->difficulty < dVeryEasy || scenarioInfo->difficulty > dInsane ) {
		char szBuf[4096]="";
		sprintf(szBuf,"Invalid difficulty value specified in scenario: %d must be between %d and %d",scenarioInfo->difficulty,dVeryEasy,dInsane);
		throw std::runtime_error(szBuf);
	}

	const XmlNode *playersNode= scenarioNode->getChild("players");

    for(int i= 0; i < GameConstants::maxPlayers; ++i) {
    	XmlNode* playerNode=NULL;
    	string factionTypeName="";
    	ControlType factionControl;

    	if(playersNode->hasChildAtIndex("player",i)){
        	playerNode = playersNode->getChild("player", i);
        	factionControl = strToControllerType( playerNode->getAttribute("control")->getValue() );

        	if(playerNode->getAttribute("resource_multiplier",false)!=NULL) {
        		// if a multiplier exists use it
				scenarioInfo->resourceMultipliers[i]=playerNode->getAttribute("resource_multiplier")->getFloatValue();
			}
			else {
				// if no multiplier exists use defaults
				scenarioInfo->resourceMultipliers[i]=GameConstants::normalMultiplier;
				if(factionControl==ctCpuEasy) {
					scenarioInfo->resourceMultipliers[i]=GameConstants::easyMultiplier;
				}
				if(factionControl==ctCpuUltra) {
					scenarioInfo->resourceMultipliers[i]=GameConstants::ultraMultiplier;
				}
				else if(factionControl==ctCpuMega) {
					scenarioInfo->resourceMultipliers[i]=GameConstants::megaMultiplier;
				}
			}

    	}
        else {
        	factionControl=ctClosed;
        }

        scenarioInfo->factionControls[i] = factionControl;

        if(factionControl != ctClosed){
            int teamIndex = playerNode->getAttribute("team")->getIntValue();

            if( teamIndex < 1 || teamIndex > GameConstants::maxPlayers ) {
        		char szBuf[4096]="";
        		sprintf(szBuf,"Invalid team value specified in scenario: %d must be between %d and %d",teamIndex,1,GameConstants::maxPlayers);
        		throw std::runtime_error(szBuf);
            }

            scenarioInfo->teams[i]= playerNode->getAttribute("team")->getIntValue();
            scenarioInfo->factionTypeNames[i]= playerNode->getAttribute("faction")->getValue();
        }

        scenarioInfo->mapName = scenarioNode->getChild("map")->getAttribute("value")->getValue();
        scenarioInfo->tilesetName = scenarioNode->getChild("tileset")->getAttribute("value")->getValue();
        scenarioInfo->techTreeName = scenarioNode->getChild("tech-tree")->getAttribute("value")->getValue();
        scenarioInfo->defaultUnits = scenarioNode->getChild("default-units")->getAttribute("value")->getBoolValue();
        scenarioInfo->defaultResources = scenarioNode->getChild("default-resources")->getAttribute("value")->getBoolValue();
        scenarioInfo->defaultVictoryConditions = scenarioNode->getChild("default-victory-conditions")->getAttribute("value")->getBoolValue();
    }

	//add player info
    scenarioInfo->desc= lang.get("Player") + ": ";
	for(int i=0; i<GameConstants::maxPlayers; ++i) {
		if(scenarioInfo->factionControls[i] == ctHuman) {
			scenarioInfo->desc+= formatString(scenarioInfo->factionTypeNames[i]);
			break;
		}
	}

	//add misc info
	string difficultyString = "Difficulty" + intToStr(scenarioInfo->difficulty);

	scenarioInfo->desc+= "\n";
    scenarioInfo->desc+= lang.get("Difficulty") + ": " + lang.get(difficultyString) +"\n";
    scenarioInfo->desc+= lang.get("Map") + ": " + formatString(scenarioInfo->mapName) + "\n";
    scenarioInfo->desc+= lang.get("Tileset") + ": " + formatString(scenarioInfo->tilesetName) + "\n";
	scenarioInfo->desc+= lang.get("TechTree") + ": " + formatString(scenarioInfo->techTreeName) + "\n";

	if(scenarioNode->hasChild("fog-of-war") == true) {
		if(scenarioNode->getChild("fog-of-war")->getAttribute("value")->getValue() == "explored") {
			scenarioInfo->fogOfWar 				= true;
			scenarioInfo->fogOfWar_exploredFlag = true;
		}
		else {
			scenarioInfo->fogOfWar = scenarioNode->getChild("fog-of-war")->getAttribute("value")->getBoolValue();
			scenarioInfo->fogOfWar_exploredFlag = false;
		}
		//printf("\nFOG OF WAR is set to [%d]\n",scenarioInfo->fogOfWar);
	}
	else {
		scenarioInfo->fogOfWar 				= true;
		scenarioInfo->fogOfWar_exploredFlag = false;
	}
	//scenarioLogoTexture = NULL;
	cleanupPreviewTexture();
	previewLoadDelayTimer=time(NULL);
	needToLoadTextures=true;
}

void MenuStateScenario::loadScenarioPreviewTexture(){
	if(enableScenarioTexturePreview == true) {
		GameSettings gameSettings;
		loadGameSettings(&scenarioInfo, &gameSettings);

		string scenarioLogo 	= "";
		bool loadingImageUsed 	= false;

		Game::extractScenarioLogoFile(&gameSettings, scenarioLogo, loadingImageUsed);

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] scenarioLogo [%s]\n",__FILE__,__FUNCTION__,__LINE__,scenarioLogo.c_str());

		if(scenarioLogo != "") {
			cleanupPreviewTexture();
			scenarioLogoTexture = Renderer::findFactionLogoTexture(scenarioLogo);
		}
		else {
			cleanupPreviewTexture();
			scenarioLogoTexture = NULL;
		}
	}
}

void MenuStateScenario::loadGameSettings(const ScenarioInfo *scenarioInfo, GameSettings *gameSettings){

	int factionCount= 0;

	gameSettings->setDescription(formatString(scenarioFiles[listBoxScenario.getSelectedItemIndex()]));
	gameSettings->setMap(scenarioInfo->mapName);
    gameSettings->setTileset(scenarioInfo->tilesetName);
    gameSettings->setTech(scenarioInfo->techTreeName);
	gameSettings->setScenario(scenarioFiles[listBoxScenario.getSelectedItemIndex()]);
	gameSettings->setScenarioDir(Scenario::getScenarioPath(dirList, scenarioFiles[listBoxScenario.getSelectedItemIndex()]));
	gameSettings->setDefaultUnits(scenarioInfo->defaultUnits);
	gameSettings->setDefaultResources(scenarioInfo->defaultResources);
	gameSettings->setDefaultVictoryConditions(scenarioInfo->defaultVictoryConditions);

    for(int i=0; i<GameConstants::maxPlayers; ++i){
        ControlType ct= static_cast<ControlType>(scenarioInfo->factionControls[i]);
		if(ct!=ctClosed){
			if(ct==ctHuman){
				gameSettings->setThisFactionIndex(factionCount);
			}
			gameSettings->setFactionControl(factionCount, ct);
			gameSettings->setResourceMultiplierIndex(factionCount, (scenarioInfo->resourceMultipliers[i]-0.5f)/0.1f);
            gameSettings->setTeam(factionCount, scenarioInfo->teams[i]-1);
			gameSettings->setStartLocationIndex(factionCount, i);
            gameSettings->setFactionTypeName(factionCount, scenarioInfo->factionTypeNames[i]);
			factionCount++;
		}
    }

	gameSettings->setFactionCount(factionCount);
	gameSettings->setFogOfWar(scenarioInfo->fogOfWar);
	uint32 valueFlags1 = gameSettings->getFlagTypes1();
	if(scenarioInfo->fogOfWar == false || scenarioInfo->fogOfWar_exploredFlag) {
        valueFlags1 |= ft1_show_map_resources;
        gameSettings->setFlagTypes1(valueFlags1);
	}
	else {
        valueFlags1 &= ~ft1_show_map_resources;
        gameSettings->setFlagTypes1(valueFlags1);
	}

	gameSettings->setPathFinderType(static_cast<PathFinderType>(Config::getInstance().getInt("ScenarioPathFinderType",intToStr(pfBasic).c_str())));
}

ControlType MenuStateScenario::strToControllerType(const string &str){
    if(str=="closed"){
        return ctClosed;
    }
    else if(str=="cpu-easy"){
	    return ctCpuEasy;
    }
    else if(str=="cpu"){
	    return ctCpu;
    }
    else if(str=="cpu-ultra"){
        return ctCpuUltra;
    }
    else if(str=="cpu-mega"){
        return ctCpuMega;
    }
    else if(str=="human"){
	    return ctHuman;
    }

	char szBuf[4096]="";
	sprintf(szBuf,"Invalid controller value specified in scenario: [%s] must be one of the following: closed, cpu-easy, cpu, cpu-ultra, cpu-mega, human",str.c_str());
	throw std::runtime_error(szBuf);
}

void MenuStateScenario::showMessageBox(const string &text, const string &header, bool toggle){
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

void MenuStateScenario::keyDown(char key) {
	Config &configKeys = Config::getInstance(std::pair<ConfigType,ConfigType>(cfgMainKeys,cfgUserKeys));
	if(key == configKeys.getCharKey("SaveGUILayout")) {
		bool saved = GraphicComponent::saveAllCustomProperties(containerName);
		//Lang &lang= Lang::getInstance();
		//console.addLine(lang.get("GUILayoutSaved") + " [" + (saved ? lang.get("Yes") : lang.get("No"))+ "]");
	}
}

}}//end namespace
