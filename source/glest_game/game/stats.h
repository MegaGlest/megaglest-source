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

#ifndef _GLEST_GAME_STATS_H_
#define _GLEST_GAME_STATS_H_

#include <string>

#include "game_constants.h"
#include "faction.h"

using std::string;

namespace Glest{ namespace Game{

struct PlayerStats{
	PlayerStats();

	ControlType control;
	string factionTypeName;
	int teamIndex;
	bool victory;
	int kills;
	int deaths;
	int unitsProduced;
	int resourcesHarvested;
};

// =====================================================
// 	class Stats
//
///	Player statistics that are shown after the game ends
// =====================================================

class Stats{
private:
	PlayerStats playerStats[GameConstants::maxPlayers];
	
	string description;
	int factionCount;
	int thisFactionIndex;

public:
	void init(int factionCount, int thisFactionIndex, const string &description);

	string getDescription() const	{return description;}
	int getThisFactionIndex() const	{return thisFactionIndex;}
	int getFactionCount() const		{return factionCount;}

	const string &getFactionTypeName(int factionIndex) const	{return playerStats[factionIndex].factionTypeName;}
	ControlType getControl(int factionIndex) const				{return playerStats[factionIndex].control;}
	bool getVictory(int factionIndex) const						{return playerStats[factionIndex].victory;}
	int getTeam(int factionIndex) const							{return playerStats[factionIndex].teamIndex;}
	int getKills(int factionIndex) const						{return playerStats[factionIndex].kills;}
	int getDeaths(int factionIndex) const						{return playerStats[factionIndex].deaths;}
	int getUnitsProduced(int factionIndex) const				{return playerStats[factionIndex].unitsProduced;}
	int getResourcesHarvested(int factionIndex) const			{return playerStats[factionIndex].resourcesHarvested;}

	void setDescription(const string& description)							{this->description = description;}
	void setFactionTypeName(int playerIndex, const string& factionTypeName)	{playerStats[playerIndex].factionTypeName= factionTypeName;}
	void setControl(int playerIndex, ControlType control)					{playerStats[playerIndex].control= control;}
	void setTeam(int playerIndex, int teamIndex)							{playerStats[playerIndex].teamIndex= teamIndex;}
	void setVictorious(int playerIndex);
	void kill(int killerFactionIndex, int killedFactionIndex);
	void produce(int producerFactionIndex);
	void harvest(int harvesterFactionIndex, int amount);
};

}}//end namespace

#endif
