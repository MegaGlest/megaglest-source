// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_GAMESETTINGS_H_
#define _GLEST_GAME_GAMESETTINGS_H_

#include "game_constants.h"
#include "conversion.h"

using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
//	class GameSettings
// =====================================================

class GameSettings{
private:
	string description;
	string map;
	string tileset;
	string tech;
	string scenario;
	string scenarioDir;
	string factionTypeNames[GameConstants::maxPlayers]; //faction names
	string networkPlayerNames[GameConstants::maxPlayers];

	ControlType factionControls[GameConstants::maxPlayers];

	int thisFactionIndex;
	int factionCount;
	int teams[GameConstants::maxPlayers];
	int startLocationIndex[GameConstants::maxPlayers];
	

	bool defaultUnits;
	bool defaultResources;
	bool defaultVictoryConditions;

	bool fogOfWar;
	bool enableObserverModeAtEndGame;
	bool enableServerControlledAI;
	int networkFramePeriod;

public:


    GameSettings() {		
    	fogOfWar = true;
    	enableObserverModeAtEndGame = false;
    	enableServerControlledAI    = false;
    	networkFramePeriod			= GameConstants::networkFramePeriod;

    	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
    		factionTypeNames[i] = "";
    		networkPlayerNames[i] = "";
    		factionControls[i] = ctClosed;
    		teams[i] = 0;
    		startLocationIndex[i] = i;
    	}
    }

	// default copy constructor will do fine, and will maintain itself ;)

	//get
	const string &getDescription() const						{return description;}
	const string &getMap() const 								{return map;}
	const string &getTileset() const							{return tileset;}
	const string &getTech() const								{return tech;}
	const string &getScenario() const							{return scenario;}
	const string &getScenarioDir() const						{return scenarioDir;}
	const string &getFactionTypeName(int factionIndex) const	{return factionTypeNames[factionIndex];}
	const string &getNetworkPlayerName(int factionIndex) const  {return networkPlayerNames[factionIndex];}
	ControlType getFactionControl(int factionIndex) const		{return factionControls[factionIndex];}

	bool isNetworkGame() const {
		bool result = false;
		for(int idx = 0; idx < GameConstants::maxPlayers; ++idx) {
			if(factionControls[idx] == ctNetwork) {
				result = true;
				break;
			}
		}
		return result;
	}
	int getThisFactionIndex() const						{return thisFactionIndex;}
	int getFactionCount() const							{return factionCount;}
	int getTeam(int factionIndex) const					{return teams[factionIndex];}
	int getStartLocationIndex(int factionIndex) const	{return startLocationIndex[factionIndex];}

	bool getDefaultUnits() const				{return defaultUnits;}
	bool getDefaultResources() const			{return defaultResources;}
	bool getDefaultVictoryConditions() const	{return defaultVictoryConditions;}

	bool getFogOfWar() const					{return fogOfWar;}
	bool getEnableObserverModeAtEndGame() const {return enableObserverModeAtEndGame;}
	bool getEnableServerControlledAI() 	  const {return enableServerControlledAI;}
	int getNetworkFramePeriod()			  const {return networkFramePeriod; }

	//set
	void setDescription(const string& description)						{this->description= description;}
	void setMap(const string& map)										{this->map= map;}
	void setTileset(const string& tileset)								{this->tileset= tileset;}
	void setTech(const string& tech)									{this->tech= tech;}
	void setScenario(const string& scenario)							{this->scenario= scenario;}
	void setScenarioDir(const string& scenarioDir)						{this->scenarioDir= scenarioDir;}

	void setFactionTypeName(int factionIndex, const string& factionTypeName)	{this->factionTypeNames[factionIndex]= factionTypeName;}
	void setNetworkPlayerName(int factionIndex,const string& playername)    {this->networkPlayerNames[factionIndex]= playername;}
	void setFactionControl(int factionIndex, ControlType controller)		{this->factionControls[factionIndex]= controller;}
	void setThisFactionIndex(int thisFactionIndex) 							{this->thisFactionIndex= thisFactionIndex;}
	void setFactionCount(int factionCount)									{this->factionCount= factionCount;}
	void setTeam(int factionIndex, int team)								{this->teams[factionIndex]= team;}
	void setStartLocationIndex(int factionIndex, int startLocationIndex)	{this->startLocationIndex[factionIndex]= startLocationIndex;}

	void setDefaultUnits(bool defaultUnits) 						{this->defaultUnits= defaultUnits;}
	void setDefaultResources(bool defaultResources) 				{this->defaultResources= defaultResources;}
	void setDefaultVictoryConditions(bool defaultVictoryConditions) {this->defaultVictoryConditions= defaultVictoryConditions;}

	void setFogOfWar(bool fogOfWar)									{this->fogOfWar = fogOfWar;}
	void setEnableObserverModeAtEndGame(bool value) 				{this->enableObserverModeAtEndGame = value;}
	void setEnableServerControlledAI(bool value)					{this->enableServerControlledAI = value;}
	void setNetworkFramePeriod(int value)							{this->networkFramePeriod = value; }

	string toString() const {
		string result = "";

		result += "description = " + description + "\n";
		result += "map = " + map + "\n";
		result += "tileset = " + tileset + "\n";
		result += "tech = " + tech + "\n";
		result += "scenario = " + scenario + "\n";
		result += "scenarioDir = " + scenarioDir + "\n";

		for(int idx =0; idx < GameConstants::maxPlayers; idx++) {
			result += "player index = " + intToStr(idx) + "\n";
			result += "factionTypeName = " + factionTypeNames[idx] + "\n";
			result += "networkPlayerName = " + networkPlayerNames[idx] + "\n";

			result += "factionControl = " + intToStr(factionControls[idx]) + "\n";
			result += "team = " + intToStr(teams[idx]) + "\n";
			result += "startLocationIndex = " + intToStr(startLocationIndex[idx]) + "\n";
		}

		result += "thisFactionIndex = " + intToStr(thisFactionIndex) + "\n";
		result += "factionCount = " + intToStr(factionCount) + "\n";
		result += "defaultUnits = " + intToStr(defaultUnits) + "\n";
		result += "defaultResources = " + intToStr(defaultResources) + "\n";
		result += "defaultVictoryConditions = " + intToStr(defaultVictoryConditions) + "\n";
		result += "fogOfWar = " + intToStr(fogOfWar) + "\n";
		result += "enableObserverModeAtEndGame = " + intToStr(enableObserverModeAtEndGame) + "\n";
		result += "enableServerControlledAI = " + intToStr(enableServerControlledAI) + "\n";
		result += "networkFramePeriod = " + intToStr(networkFramePeriod) + "\n";

		return result;
	}
};

}}//end namespace

#endif
