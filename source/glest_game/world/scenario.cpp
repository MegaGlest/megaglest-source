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

#include "scenario.h"

#include <stdexcept>
#include "logger.h"
#include "xml_parser.h"
#include "util.h"
#include "game_util.h"
#include <stdio.h>
#include "platform_common.h"
#include "properties.h"
#include "lang.h"
#include "socket.h"
#include "config.h"
#include "platform_util.h"
#include "leak_dumper.h"

using namespace Shared::Xml;
using namespace Shared::Util;
using namespace Shared::PlatformCommon;
using namespace std;

namespace Glest{ namespace Game{

// =====================================================
//	class Scenario
// =====================================================

Scenario::~Scenario() {

}

Checksum Scenario::load(const string &path) {
	//printf("[%s:%s] Line: %d path [%s]\n",__FILE__,__FUNCTION__,__LINE__,path.c_str());

    Checksum scenarioChecksum;
	try {
		scenarioChecksum.addFile(path);
		checksumValue.addFile(path);

		string name= cutLastExt(lastDir(path));

		char szBuf[1024]="";
		sprintf(szBuf,Lang::getInstance().get("LogScreenGameLoadingScenario","",true).c_str(),formatString(name).c_str());
		Logger::getInstance().add(szBuf, true);

		Scenario::loadScenarioInfo(path, &info);

		//parse xml
		XmlTree xmlTree;
		xmlTree.load(path,Properties::getTagReplacementValues());
		const XmlNode *scenarioNode= xmlTree.getRootNode();
		const XmlNode *scriptsNode= scenarioNode->getChild("scripts");

		for(int i= 0; i<scriptsNode->getChildCount(); ++i){
			const XmlNode *scriptNode = scriptsNode->getChild(i);

			scripts.push_back(Script(getFunctionName(scriptNode), scriptNode->getText()));
		}
	}
	//Exception handling (conversions and so on);
	catch(const exception &ex) {
	    char szBuf[8096]="";
	    sprintf(szBuf,"In [%s::%s %d]\nError loading scenario [%s]:\n%s\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,path.c_str(),ex.what());
	    SystemFlags::OutputDebug(SystemFlags::debugError,szBuf);
	    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);

		throw megaglest_runtime_error(szBuf);
	}

	return scenarioChecksum;
}

int Scenario::getScenarioPathIndex(const vector<string> dirList, const string &scenarioName) {
    int iIndex = 0;
    for(int idx = 0; idx < dirList.size(); idx++) {
    	string currentPath = dirList[idx];
    	endPathWithSlash(currentPath);
        string scenarioFile = currentPath + scenarioName + "/" + scenarioName + ".xml";
        if(fileExists(scenarioFile) == true) {
            iIndex = idx;
            break;
        }
    }

    return iIndex;
}

string Scenario::getScenarioDir(const vector<string> dir, const string &scenarioName) {
    string scenarioDir = "";
    for(int idx = 0; idx < dir.size(); idx++) {
    	string currentPath = dir[idx];
    	endPathWithSlash(currentPath);
    	string scenarioFile = currentPath + scenarioName + "/" + scenarioName + ".xml";

    	//printf("\n[%s:%s] Line: %d scenarioName [%s] scenarioFile [%s]\n",__FILE__,__FUNCTION__,__LINE__,scenarioName.c_str(),scenarioFile.c_str());

        if(fileExists(scenarioFile) == true) {
			scenarioDir = currentPath + scenarioName + "/";
            break;
        }
    }

    return scenarioDir;
}

string Scenario::getScenarioPath(const vector<string> dirList, const string &scenarioName, bool getMatchingRootScenarioPathOnly){
    string scenarioFile = "";
    for(int idx = 0; idx < dirList.size(); idx++) {
    	string currentPath = dirList[idx];
    	endPathWithSlash(currentPath);
    	scenarioFile = currentPath + scenarioName + "/" + scenarioName + ".xml";

    	//printf("\n[%s:%s] Line: %d scenarioName [%s] scenarioFile [%s]\n",__FILE__,__FUNCTION__,__LINE__,scenarioName.c_str(),scenarioFile.c_str());

        if(fileExists(scenarioFile) == true) {
			if(getMatchingRootScenarioPathOnly == true) {
				scenarioFile = dirList[idx];
			}
            break;
        }
		else {
			scenarioFile = "";
		}
    }

    return scenarioFile;
}

string Scenario::getScenarioPath(const string &dir, const string &scenarioName){
	string currentPath = dir;
	endPathWithSlash(currentPath);
	string scenarioFile = currentPath + scenarioName + "/" + scenarioName + ".xml";
    return scenarioFile;
}

string Scenario::getFunctionName(const XmlNode *scriptNode){
	string name= scriptNode->getName();

	for(int i= 0; i<scriptNode->getAttributeCount(); ++i){
		name+= "_" + scriptNode->getAttribute(i)->getValue();
	}
	return name;
}

void Scenario::loadScenarioInfo(string file, ScenarioInfo *scenarioInfo) {
	//printf("[%s:%s] Line: %d file [%s]\n",__FILE__,__FUNCTION__,__LINE__,file.c_str());
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] file [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,file.c_str());
	//printf("In [%s::%s Line: %d] file [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,file.c_str());

    Lang &lang= Lang::getInstance();

    XmlTree xmlTree;
	xmlTree.load(file,Properties::getTagReplacementValues());

    const XmlNode *scenarioNode= xmlTree.getRootNode();
    if(scenarioNode == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"scenarioNode == NULL for file [%s]\n",file.c_str());
		throw std::runtime_error(szBuf);
    }
	const XmlNode *difficultyNode= scenarioNode->getChild("difficulty");
    if(difficultyNode == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"difficultyNode == NULL for file [%s]\n",file.c_str());
		throw std::runtime_error(szBuf);
    }

	scenarioInfo->difficulty = difficultyNode->getAttribute("value")->getIntValue();
	if( scenarioInfo->difficulty < dVeryEasy || scenarioInfo->difficulty > dInsane ) {
		char szBuf[4096]="";
		sprintf(szBuf,"Invalid difficulty value specified in scenario: %d must be between %d and %d",scenarioInfo->difficulty,dVeryEasy,dInsane);
		throw std::runtime_error(szBuf);
	}

	const XmlNode *playersNode= scenarioNode->getChild("players");
    if(playersNode == NULL) {
		char szBuf[4096]="";
		sprintf(szBuf,"playersNode == NULL for file [%s]\n",file.c_str());
		throw std::runtime_error(szBuf);
    }

    for(int i= 0; i < GameConstants::maxPlayers; ++i) {
    	XmlNode* playerNode=NULL;
    	string factionTypeName="";
    	ControlType factionControl;

    	if(playersNode->hasChildAtIndex("player",i)) {
        	playerNode = playersNode->getChild("player", i);
            if(playerNode == NULL) {
        		char szBuf[4096]="";
        		sprintf(szBuf,"playerNode == NULL for index = %d for file [%s]\n",i,file.c_str());
        		throw std::runtime_error(szBuf);
            }

        	factionControl = strToControllerType( playerNode->getAttribute("control")->getValue() );

        	if(playerNode->getAttribute("resource_multiplier",false) != NULL) {
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

        if(factionControl != ctClosed) {
            int teamIndex = playerNode->getAttribute("team")->getIntValue();

            if( teamIndex < 1 || teamIndex > GameConstants::maxPlayers  + GameConstants::specialFactions) {
        		char szBuf[4096]="";
        		sprintf(szBuf,"Invalid team value specified in scenario: %d must be between %d and %d",teamIndex,1,GameConstants::maxPlayers);
        		throw std::runtime_error(szBuf);
            }

            scenarioInfo->teams[i]= teamIndex;
            scenarioInfo->factionTypeNames[i]= playerNode->getAttribute("faction")->getValue();
        }
        else {
            scenarioInfo->teams[i]= 0;
            scenarioInfo->factionTypeNames[i]= "";
        }

        if(scenarioNode->getChild("map") == NULL) {
    		char szBuf[4096]="";
    		sprintf(szBuf,"map == NULL for file [%s]\n",file.c_str());
    		throw std::runtime_error(szBuf);
        }
        if(scenarioNode->getChild("tileset") == NULL) {
    		char szBuf[4096]="";
    		sprintf(szBuf,"tileset == NULL for file [%s]\n",file.c_str());
    		throw std::runtime_error(szBuf);
        }
        if(scenarioNode->getChild("tech-tree") == NULL) {
    		char szBuf[4096]="";
    		sprintf(szBuf,"tech-tree == NULL for file [%s]\n",file.c_str());
    		throw std::runtime_error(szBuf);
        }
        if(scenarioNode->getChild("default-units") == NULL) {
    		char szBuf[4096]="";
    		sprintf(szBuf,"default-units == NULL for file [%s]\n",file.c_str());
    		throw std::runtime_error(szBuf);
        }
        if(scenarioNode->getChild("default-resources") == NULL) {
    		char szBuf[4096]="";
    		sprintf(szBuf,"default-resources == NULL for file [%s]\n",file.c_str());
    		throw std::runtime_error(szBuf);
        }
        if(scenarioNode->getChild("default-victory-conditions") == NULL) {
    		char szBuf[4096]="";
    		sprintf(szBuf,"default-victory-conditions == NULL for file [%s]\n",file.c_str());
    		throw std::runtime_error(szBuf);
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

	scenarioInfo->file = file;
	scenarioInfo->name = extractFileFromDirectoryPath(file);
	scenarioInfo->name = cutLastExt(scenarioInfo->name);

	//scenarioLogoTexture = NULL;
	//cleanupPreviewTexture();
	//previewLoadDelayTimer=time(NULL);
	//needToLoadTextures=true;
}

ControlType Scenario::strToControllerType(const string &str) {
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
    else if(str=="network"){
	    return ctNetwork;
    }

	char szBuf[4096]="";
	sprintf(szBuf,"Invalid controller value specified in scenario: [%s] must be one of the following: closed, cpu-easy, cpu, cpu-ultra, cpu-mega, human",str.c_str());
	throw std::runtime_error(szBuf);
}

string Scenario::controllerTypeToStr(const ControlType &ct) {
	string controlString = "";

	Lang &lang= Lang::getInstance();
	switch(ct) {
		case ctCpuEasy:
			controlString= lang.get("CpuEasy");
			break;
		case ctCpu:
			controlString= lang.get("Cpu");
			break;
		case ctCpuUltra:
			controlString= lang.get("CpuUltra");
			break;
		case ctCpuMega:
			controlString= lang.get("CpuMega");
			break;
		case ctNetwork:
			controlString= lang.get("Network");
			break;
		case ctHuman:
			controlString= lang.get("Human");
			break;

		case ctNetworkCpuEasy:
			controlString= lang.get("NetworkCpuEasy");
			break;
		case ctNetworkCpu:
			controlString= lang.get("NetworkCpu");
			break;
		case ctNetworkCpuUltra:
			controlString= lang.get("NetworkCpuUltra");
			break;
		case ctNetworkCpuMega:
			controlString= lang.get("NetworkCpuMega");
			break;

		default:
			printf("Error control = %d\n",ct);
			//assert(false);
			break;
	}

	return controlString;
}

void Scenario::loadGameSettings(const vector<string> &dirList,
		const ScenarioInfo *scenarioInfo, GameSettings *gameSettings,
		string scenarioDescription) {
	int factionCount= 0;
	gameSettings->setDescription(scenarioDescription);
	gameSettings->setMap(scenarioInfo->mapName);
    gameSettings->setTileset(scenarioInfo->tilesetName);
    gameSettings->setTech(scenarioInfo->techTreeName);
    gameSettings->setScenario(scenarioInfo->name);
    gameSettings->setScenarioDir(Scenario::getScenarioPath(dirList, scenarioInfo->name));
	gameSettings->setDefaultUnits(scenarioInfo->defaultUnits);
	gameSettings->setDefaultResources(scenarioInfo->defaultResources);
	gameSettings->setDefaultVictoryConditions(scenarioInfo->defaultVictoryConditions);

    for(int i = 0; i < GameConstants::maxPlayers; ++i) {
        ControlType ct= static_cast<ControlType>(scenarioInfo->factionControls[i]);
		if(ct != ctClosed) {
			if(ct == ctHuman) {
				gameSettings->setThisFactionIndex(factionCount);

				if(gameSettings->getNetworkPlayerName(i) == "") {
					gameSettings->setNetworkPlayerName(i,Config::getInstance().getString("NetPlayerName",Socket::getHostName().c_str()));
				}
			}
			else {
				if(gameSettings->getNetworkPlayerName(i) == "") {
					gameSettings->setNetworkPlayerName(i,controllerTypeToStr(ct));
				}
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


}}//end namespace
