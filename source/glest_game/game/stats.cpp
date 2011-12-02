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

#include "stats.h"
#include "lang.h"
#include "leak_dumper.h"

namespace Glest{ namespace Game{

PlayerStats::PlayerStats() {
	control = ctClosed;
	resourceMultiplier=1.0f;
	factionTypeName = "";
	personalityType = fpt_Normal;
	teamIndex = 0;
	victory = false;
	kills = 0;
	enemykills = 0;
	deaths = 0;
	unitsProduced = 0;
	resourcesHarvested = 0;
	playerName = "";
	playerColor = Vec3f(0,0,0);
}

string PlayerStats::getStats() const {
	string result = "";

	Lang &lang= Lang::getInstance();
	string controlString = "";

	if(personalityType == fpt_Observer) {
		controlString= GameConstants::OBSERVER_SLOTNAME;
	}
	else {
		switch(control) {
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
			printf("Error control = %d\n",control);
			assert(false);
		};
	}

	if(control != ctHuman && control != ctNetwork ) {
		controlString += " x " + floatToStr(resourceMultiplier,1);
	}

	result += playerName + " (" + controlString + ") ";
	result += "faction: " + factionTypeName + " ";
	result += "Team: " + intToStr(teamIndex) + " ";
	result += "victory: " + boolToStr(victory) + " ";
	result += "# kills: " + intToStr(kills) + " ";
	result += "# enemy kills: " + intToStr(enemykills) + " ";
	result += "# deaths: " + intToStr(deaths) + "\n";
	result += "# units produced: " + intToStr(unitsProduced) + " ";
	result += "# resources harvested: " + intToStr(resourcesHarvested);

	return result;
}

// =====================================================
// class Stats
// =====================================================

void Stats::init(int factionCount, int thisFactionIndex, const string& description){
	this->thisFactionIndex= thisFactionIndex;
	this->factionCount= factionCount;
	this->description= description;
}

void Stats::setVictorious(int playerIndex){
	playerStats[playerIndex].victory= true;
}

void Stats::kill(int killerFactionIndex, int killedFactionIndex, bool isEnemy) {
	playerStats[killerFactionIndex].kills++;
	playerStats[killedFactionIndex].deaths++;
	if(isEnemy == true) {
		playerStats[killerFactionIndex].enemykills++;
	}
}

void Stats::die(int diedFactionIndex){
	playerStats[diedFactionIndex].deaths++;
}

void Stats::produce(int producerFactionIndex){
	playerStats[producerFactionIndex].unitsProduced++;
}

void Stats::harvest(int harvesterFactionIndex, int amount){
	playerStats[harvesterFactionIndex].resourcesHarvested+= amount;
}

string Stats::getStats() const {
	string result = "";

	result += "Description: " + description + "\n";
	result += "Faction count: " + intToStr(factionCount) + "\n";

	result += "Game duration (mins): " + intToStr(getFramesToCalculatePlaytime()/GameConstants::updateFps/60) + "\n";
	result += "Max Concurrent Units: " + intToStr(maxConcurrentUnitCount) + "\n";
	result += "Total EndGame Concurrent Unit Count: " + intToStr(totalEndGameConcurrentUnitCount) + "\n";

	for(unsigned int i = 0; i < factionCount; ++i) {
		const PlayerStats &player = playerStats[i];

		result += "player #" + intToStr(i) + " " + player.getStats() + "\n";
	}

	return result;
}

}}//end namespace
