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
#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
//	class GameSettings
// =====================================================

enum FlagTypes1 {
    ft1_none                = 0x00,
    ft1_show_map_resources  = 0x01
    //ft1_xx                  = 0x02,
    //ft1_xx                  = 0x04,
    //ft1_xx                  = 0x08,
    //ft1_xx                  = 0x10,
};


class GameSettings {
private:
	string description;
	string map;
	string tileset;
	string tech;
	string scenario;
	string scenarioDir;
	string factionTypeNames[GameConstants::maxPlayers]; //faction names
	string networkPlayerNames[GameConstants::maxPlayers];
	int    networkPlayerStatuses[GameConstants::maxPlayers];

	ControlType factionControls[GameConstants::maxPlayers];
	int resourceMultiplierIndex[GameConstants::maxPlayers];

	int thisFactionIndex;
	int factionCount;
	int teams[GameConstants::maxPlayers];
	int startLocationIndex[GameConstants::maxPlayers];
	int mapFilterIndex;


	bool defaultUnits;
	bool defaultResources;
	bool defaultVictoryConditions;

	bool fogOfWar;
	bool allowObservers;
	bool enableObserverModeAtEndGame;
	bool enableServerControlledAI;
	int networkFramePeriod;
	bool networkPauseGameForLaggedClients;
	PathFinderType pathFinderType;

	uint32 flagTypes1;

    int32 mapCRC;
    int32 tilesetCRC;
    int32 techCRC;

public:

    GameSettings() {
    	thisFactionIndex					= 0;
    	fogOfWar 							= true;
    	allowObservers						= false;
    	enableObserverModeAtEndGame 		= false;
    	enableServerControlledAI    		= false;
    	networkFramePeriod					= GameConstants::networkFramePeriod;
    	networkPauseGameForLaggedClients 	= false;
    	pathFinderType						= pfBasic;

    	for(int i = 0; i < GameConstants::maxPlayers; ++i) {
    		factionTypeNames[i] = "";
    		networkPlayerNames[i] = "";
    		networkPlayerStatuses[i] = 0;
    		factionControls[i] = ctClosed;
    		resourceMultiplierIndex[i] = 1.0f;
    		teams[i] = 0;
    		startLocationIndex[i] = i;
    	}

    	flagTypes1 = ft1_none;

		mapCRC      = 0;
		tilesetCRC  = 0;
		techCRC     = 0;
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
	const int    getNetworkPlayerStatuses(int factionIndex) const { return networkPlayerStatuses[factionIndex];}

	const string getNetworkPlayerNameByPlayerIndex(int playerIndex) const  {
		string result = "";
		for(int i = 0; i < GameConstants::maxPlayers; ++i) {
			if(startLocationIndex[i] == playerIndex) {
				result = networkPlayerNames[i];
				break;
			}
		}
		return result;
	}
	ControlType getFactionControl(int factionIndex) const		{return factionControls[factionIndex];}
	int getResourceMultiplierIndex(int factionIndex) const		{return resourceMultiplierIndex[factionIndex];}

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
	int getMapFilterIndex() const 						{return mapFilterIndex;}

	bool getDefaultUnits() const				{return defaultUnits;}
	bool getDefaultResources() const			{return defaultResources;}
	bool getDefaultVictoryConditions() const	{return defaultVictoryConditions;}

	bool getFogOfWar() const					{return fogOfWar;}
	bool getAllowObservers() const				{ return allowObservers;}
	bool getEnableObserverModeAtEndGame() const {return enableObserverModeAtEndGame;}
	bool getEnableServerControlledAI() 	  const {return enableServerControlledAI;}
	int getNetworkFramePeriod()			  const {return networkFramePeriod; }
	bool getNetworkPauseGameForLaggedClients()	  const {return networkPauseGameForLaggedClients; }
	PathFinderType getPathFinderType() const { return pathFinderType; }
	uint32 getFlagTypes1() const             { return flagTypes1;}

	int32 getMapCRC() const { return mapCRC; }
	int32 getTilesetCRC() const { return tilesetCRC; }
	int32 getTechCRC() const { return techCRC; }

	//set
	void setDescription(const string& description)						{this->description= description;}
	void setMap(const string& map)										{this->map= map;}
	void setTileset(const string& tileset)								{this->tileset= tileset;}
	void setTech(const string& tech)									{this->tech= tech;}
	void setScenario(const string& scenario)							{this->scenario= scenario;}
	void setScenarioDir(const string& scenarioDir)						{this->scenarioDir= scenarioDir;}

	void setFactionTypeName(int factionIndex, const string& factionTypeName)	{this->factionTypeNames[factionIndex]= factionTypeName;}
	void setNetworkPlayerName(int factionIndex,const string& playername)    {this->networkPlayerNames[factionIndex]= playername;}
	void setNetworkPlayerStatuses(int factionIndex,int status)    			{this->networkPlayerStatuses[factionIndex]= status;}
	void setFactionControl(int factionIndex, ControlType controller)		{this->factionControls[factionIndex]= controller;}
	void setResourceMultiplierIndex(int factionIndex, int multiplierIndex)	{this->resourceMultiplierIndex[factionIndex]= multiplierIndex;}

	void setThisFactionIndex(int thisFactionIndex) 							{this->thisFactionIndex= thisFactionIndex;}
	void setFactionCount(int factionCount)									{this->factionCount= factionCount;}
	void setTeam(int factionIndex, int team)								{this->teams[factionIndex]= team;}
	void setStartLocationIndex(int factionIndex, int startLocationIndex)	{this->startLocationIndex[factionIndex]= startLocationIndex;}
	void setMapFilterIndex(int mapFilterIndex)								{this->mapFilterIndex=mapFilterIndex;}

	void setDefaultUnits(bool defaultUnits) 						{this->defaultUnits= defaultUnits;}
	void setDefaultResources(bool defaultResources) 				{this->defaultResources= defaultResources;}
	void setDefaultVictoryConditions(bool defaultVictoryConditions) {this->defaultVictoryConditions= defaultVictoryConditions;}

	void setFogOfWar(bool fogOfWar)									{this->fogOfWar = fogOfWar;}
	void setAllowObservers(bool value)								{this->allowObservers = value;}
	void setEnableObserverModeAtEndGame(bool value) 				{this->enableObserverModeAtEndGame = value;}
	void setEnableServerControlledAI(bool value)					{this->enableServerControlledAI = value;}
	void setNetworkFramePeriod(int value)							{this->networkFramePeriod = value; }
	void setNetworkPauseGameForLaggedClients(bool value)			{this->networkPauseGameForLaggedClients = value; }
	void setPathFinderType(PathFinderType value)					{this->pathFinderType = value; }

	void setFlagTypes1(uint32 value)                                {this->flagTypes1 = value; }

	void setMapCRC(int32 value)     { mapCRC = value; }
	void setTilesetCRC(int32 value) { tilesetCRC = value; }
	void setTechCRC(int32 value)    { techCRC = value; }

	string toString() const {
		string result = "";

		result += "description = " + description + "\n";
		result += "mapFilterIndex = " + intToStr(mapFilterIndex) + "\n";
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
			result += "resourceMultiplierIndex = " + intToStr(resourceMultiplierIndex[idx]) + "\n";
			result += "team = " + intToStr(teams[idx]) + "\n";
			result += "startLocationIndex = " + intToStr(startLocationIndex[idx]) + "\n";
		}

		result += "thisFactionIndex = " + intToStr(thisFactionIndex) + "\n";
		result += "factionCount = " + intToStr(factionCount) + "\n";
		result += "defaultUnits = " + intToStr(defaultUnits) + "\n";
		result += "defaultResources = " + intToStr(defaultResources) + "\n";
		result += "defaultVictoryConditions = " + intToStr(defaultVictoryConditions) + "\n";
		result += "fogOfWar = " + intToStr(fogOfWar) + "\n";
		result += "allowObservers = " + intToStr(allowObservers) + "\n";
		result += "enableObserverModeAtEndGame = " + intToStr(enableObserverModeAtEndGame) + "\n";
		result += "enableServerControlledAI = " + intToStr(enableServerControlledAI) + "\n";
		result += "networkFramePeriod = " + intToStr(networkFramePeriod) + "\n";
		result += "networkPauseGameForLaggedClients = " + intToStr(networkPauseGameForLaggedClients) + "\n";
		result += "pathFinderType = " + intToStr(pathFinderType) + "\n";
		result += "flagTypes1 = " + intToStr(flagTypes1) + "\n";
		result += "mapCRC = " + intToStr(mapCRC) + "\n";
		result += "tilesetCRC = " + intToStr(tilesetCRC) + "\n";
		result += "techCRC = " + intToStr(techCRC) + "\n";

		return result;
	}
};

}}//end namespace

#endif
