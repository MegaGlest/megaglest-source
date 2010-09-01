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
	Lang &lang= Lang::getInstance();
	NetworkManager &networkManager= NetworkManager::getInstance();

	mainMessageBox.init(lang.get("Ok"));
	mainMessageBox.setEnabled(false);
	mainMessageBoxState=0;

	this->autoloadScenarioName = autoloadScenarioName;
    vector<string> results;

	this->dirList = dirList;

    labelInfo.init(350, 350);
	labelInfo.setFont(CoreData::getInstance().getMenuFontNormal());

    buttonReturn.init(350, 200, 125);
	buttonPlayNow.init(525, 200, 125);

    listBoxScenario.init(350, 400, 190);
	labelScenario.init(350, 430);

	buttonReturn.setText(lang.get("Return"));
	buttonPlayNow.setText(lang.get("PlayNow"));

    labelScenario.setText(lang.get("Scenario"));

    //scenario listbox
	findDirs(dirList, results);
    scenarioFiles = results;
	if(results.size() == 0) {
        throw runtime_error("There are no scenarios");
	}
	for(int i= 0; i<results.size(); ++i){
		results[i] = formatString(results[i]);
	}
    listBoxScenario.setItems(results);

    loadScenarioInfo(Scenario::getScenarioPath(dirList, scenarioFiles[listBoxScenario.getSelectedItemIndex()]), &scenarioInfo );
    labelInfo.setText(scenarioInfo.desc);

	networkManager.init(nrServer);
}

void MenuStateScenario::mouseClick(int x, int y, MouseButton mouseButton){

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
	else if(buttonReturn.mouseClick(x,y)){
		soundRenderer.playFx(coreData.getClickSoundA());
		mainMenu->setState(new MenuStateNewGame(program, mainMenu));
    }
	else if(buttonPlayNow.mouseClick(x,y)){
		soundRenderer.playFx(coreData.getClickSoundC());
		launchGame();
	}
    else if(listBoxScenario.mouseClick(x, y)){
		loadScenarioInfo(Scenario::getScenarioPath(dirList, scenarioFiles[listBoxScenario.getSelectedItemIndex()]), &scenarioInfo);
        labelInfo.setText(scenarioInfo.desc);
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

	if(mainMessageBox.getEnabled()){
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

void MenuStateScenario::update(){
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
}

void MenuStateScenario::launchGame(){
	GameSettings gameSettings;
    loadGameSettings(&scenarioInfo, &gameSettings);
	program->setState(new Game(program, &gameSettings));
}

void MenuStateScenario::setScenario(int i){
	listBoxScenario.setSelectedItemIndex(i);
	loadScenarioInfo(Scenario::getScenarioPath(dirList, scenarioFiles[listBoxScenario.getSelectedItemIndex()]), &scenarioInfo);
}

void MenuStateScenario::loadScenarioInfo(string file, ScenarioInfo *scenarioInfo){

    Lang &lang= Lang::getInstance();

    XmlTree xmlTree;
	xmlTree.load(file);

    const XmlNode *scenarioNode= xmlTree.getRootNode();
	const XmlNode *difficultyNode= scenarioNode->getChild("difficulty");
	scenarioInfo->difficulty = difficultyNode->getAttribute("value")->getIntValue();
	if( scenarioInfo->difficulty < dVeryEasy || scenarioInfo->difficulty > dInsane )
	{
		throw std::runtime_error("Invalid difficulty");
	}

	const XmlNode *playersNode= scenarioNode->getChild("players");

    for(int i= 0; i<GameConstants::maxPlayers; ++i){
    	XmlNode* playerNode;
    	string factionTypeName;
    	ControlType factionControl;

    	if(playersNode->hasChildAtIndex("player",i)){
        	playerNode = playersNode->getChild("player", i);
        	factionControl = strToControllerType( playerNode->getAttribute("control")->getValue() );
    	}
        else{
        	factionControl=ctClosed;
        }

        scenarioInfo->factionControls[i] = factionControl;

        if(factionControl != ctClosed){
            int teamIndex = playerNode->getAttribute("team")->getIntValue();

            if( teamIndex < 1 || teamIndex > GameConstants::maxPlayers )
            {
                throw runtime_error("Team out of range: " + intToStr(teamIndex) );
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
	for(int i=0; i<GameConstants::maxPlayers; ++i )
	{
		if(scenarioInfo->factionControls[i] == ctHuman )
		{
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
		scenarioInfo->fogOfWar = scenarioNode->getChild("fog-of-war")->getAttribute("value")->getBoolValue();
	}
	else {
		scenarioInfo->fogOfWar = true;
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
            gameSettings->setTeam(factionCount, scenarioInfo->teams[i]-1);
			gameSettings->setStartLocationIndex(factionCount, i);
            gameSettings->setFactionTypeName(factionCount, scenarioInfo->factionTypeNames[i]);
			factionCount++;
		}
    }

	gameSettings->setFactionCount(factionCount);
	gameSettings->setFogOfWar(scenarioInfo->fogOfWar);

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

    throw std::runtime_error("Unknown controller type: " + str);
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

}}//end namespace
