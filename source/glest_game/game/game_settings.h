// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_GAMESETTINGS_H_
#define _GLEST_GAME_GAMESETTINGS_H_

#include "game_constants.h"

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

	ControlType factionControls[GameConstants::maxPlayers];

	int thisFactionIndex;
	int factionCount;
	int teams[GameConstants::maxPlayers];
	int startLocationIndex[GameConstants::maxPlayers];

	bool defaultUnits;
	bool defaultResources;
	bool defaultVictoryConditions;

public:	
	//get
	const string &getDescription() const						{return description;}
	const string &getMap() const 								{return map;}
	const string &getTileset() const							{return tileset;}
	const string &getTech() const								{return tech;}
	const string &getScenario() const							{return scenario;}
	const string &getScenarioDir() const						{return scenarioDir;}
	const string &getFactionTypeName(int factionIndex) const	{return factionTypeNames[factionIndex];}
	ControlType getFactionControl(int factionIndex) const		{return factionControls[factionIndex];}

	int getThisFactionIndex() const						{return thisFactionIndex;}
	int getFactionCount() const							{return factionCount;} 
	int getTeam(int factionIndex) const					{return teams[factionIndex];}
	int getStartLocationIndex(int factionIndex) const	{return startLocationIndex[factionIndex];}

	bool getDefaultUnits() const				{return defaultUnits;}
	bool getDefaultResources() const			{return defaultResources;}
	bool getDefaultVictoryConditions() const	{return defaultVictoryConditions;}

	//set
	void setDescription(const string& description)						{this->description= description;}
	void setMap(const string& map)										{this->map= map;}
	void setTileset(const string& tileset)								{this->tileset= tileset;}
	void setTech(const string& tech)									{this->tech= tech;}
	void setScenario(const string& scenario)							{this->scenario= scenario;}
	void setScenarioDir(const string& scenarioDir)						{this->scenarioDir= scenarioDir;}

	void setFactionTypeName(int factionIndex, const string& factionTypeName)	{this->factionTypeNames[factionIndex]= factionTypeName;}

	void setFactionControl(int factionIndex, ControlType controller)			{this->factionControls[factionIndex]= controller;}

	void setThisFactionIndex(int thisFactionIndex) 							{this->thisFactionIndex= thisFactionIndex;}
	void setFactionCount(int factionCount)									{this->factionCount= factionCount;} 
	void setTeam(int factionIndex, int team)								{this->teams[factionIndex]= team;}
	void setStartLocationIndex(int factionIndex, int startLocationIndex)	{this->startLocationIndex[factionIndex]= startLocationIndex;}

	void setDefaultUnits(bool defaultUnits) 						{this->defaultUnits= defaultUnits;}
	void setDefaultResources(bool defaultResources) 				{this->defaultResources= defaultResources;}
	void setDefaultVictoryConditions(bool defaultVictoryConditions) {this->defaultVictoryConditions= defaultVictoryConditions;}
};

}}//end namespace

#endif
