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

#ifndef _GLEST_GAME_STATS_H_
#define _GLEST_GAME_STATS_H_

#include <string>

#include "game_constants.h"
#include "faction.h"
#include "faction_type.h"
#include "vec.h"
#include "leak_dumper.h"

using std::string;
using namespace Shared::Graphics;

namespace Glest{ namespace Game{

class PlayerStats {
public:
	PlayerStats() {
		control = ctClosed;
		resourceMultiplier=1.0f;
		factionTypeName = "";
		personalityType = fpt_Normal;
		teamIndex = 0;
		victory = false;
		kills = 0;
		deaths = 0;
		unitsProduced = 0;
		resourcesHarvested = 0;
		playerName = "";
		playerColor = Vec3f(0,0,0);
	}

	ControlType control;
	float resourceMultiplier;
	string factionTypeName;
	FactionPersonalityType personalityType;
	int teamIndex;
	bool victory;
	int kills;
	int deaths;
	int unitsProduced;
	int resourcesHarvested;
	string playerName;
	Vec3f playerColor;
};

// =====================================================
// 	class Stats
//
///	Player statistics that are shown after the game ends
// =====================================================

class Stats {
private:
	PlayerStats playerStats[GameConstants::maxPlayers];
	
	string description;
	int factionCount;
	int thisFactionIndex;

public:

	Stats() {
		description = "";
		factionCount = 0;
		thisFactionIndex = 0;
	}
	void init(int factionCount, int thisFactionIndex, const string &description);

	string getDescription() const	{return description;}
	int getThisFactionIndex() const	{return thisFactionIndex;}
	int getFactionCount() const		{return factionCount;}
	const string &getFactionTypeName(int factionIndex) const	{return playerStats[factionIndex].factionTypeName;}
	FactionPersonalityType getPersonalityType(int factionIndex) const { return playerStats[factionIndex].personalityType;}
	ControlType getControl(int factionIndex) const				{return playerStats[factionIndex].control;}
	float getResourceMultiplier(int factionIndex) const				{return playerStats[factionIndex].resourceMultiplier;}
	bool getVictory(int factionIndex) const						{return playerStats[factionIndex].victory;}
	int getTeam(int factionIndex) const							{return playerStats[factionIndex].teamIndex;}
	int getKills(int factionIndex) const						{return playerStats[factionIndex].kills;}
	int getDeaths(int factionIndex) const						{return playerStats[factionIndex].deaths;}
	int getUnitsProduced(int factionIndex) const				{return playerStats[factionIndex].unitsProduced;}
	int getResourcesHarvested(int factionIndex) const			{return playerStats[factionIndex].resourcesHarvested;}
	string getPlayerName(int factionIndex) const				{return playerStats[factionIndex].playerName;}
	Vec3f getPlayerColor(int factionIndex) const				{ return playerStats[factionIndex].playerColor;}

	void setDescription(const string& description)							{this->description = description;}
	void setFactionTypeName(int playerIndex, const string& factionTypeName)	{playerStats[playerIndex].factionTypeName= factionTypeName;}
	void setPersonalityType(int playerIndex, FactionPersonalityType value)  { playerStats[playerIndex].personalityType = value;}
	void setControl(int playerIndex, ControlType control)					{playerStats[playerIndex].control= control;}
	void setResourceMultiplier(int playerIndex, float resourceMultiplier)	{playerStats[playerIndex].resourceMultiplier= resourceMultiplier;}
	void setTeam(int playerIndex, int teamIndex)							{playerStats[playerIndex].teamIndex= teamIndex;}
	void setVictorious(int playerIndex);
	void kill(int killerFactionIndex, int killedFactionIndex);
	void die(int diedFactionIndex);
	void produce(int producerFactionIndex);
	void harvest(int harvesterFactionIndex, int amount);
	void setPlayerName(int playerIndex, string value) 	{playerStats[playerIndex].playerName = value; }
	void setPlayerColor(int playerIndex, Vec3f value)	{playerStats[playerIndex].playerColor = value; }

};

}}//end namespace

#endif
