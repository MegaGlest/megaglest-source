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

#include "stats.h"
#include "lang.h"
#include "leak_dumper.h"

namespace Glest{ namespace Game{

PlayerStats::PlayerStats() {
	control = ctClosed;
	resourceMultiplier=1.0f;
	//factionTypeName = "";
	personalityType = fpt_Normal;
	teamIndex = 0;
	victory = false;
	kills = 0;
	enemykills = 0;
	deaths = 0;
	unitsProduced = 0;
	resourcesHarvested = 0;
	//playerName = "";
	playerLeftBeforeEnd = false;
	timePlayerLeft = -1;
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
			break;
		};
	}

	if((control != ctHuman && control != ctNetwork) ||
			(control == ctNetwork && playerLeftBeforeEnd == true)) {
		controlString += " x " + floatToStr(resourceMultiplier,1);
	}

	result += playerName + " (" + controlString + ") ";
	if(control == ctNetwork && playerLeftBeforeEnd==true ) {
		result += "player left before end ";
	}
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

void Stats::init(int factionCount, int thisFactionIndex, const string& description, const string &techName) {
	this->thisFactionIndex= thisFactionIndex;
	this->factionCount= factionCount;
	this->description= description;
	this->techName = techName;
}

void Stats::setVictorious(int playerIndex){
	playerStats[playerIndex].victory= true;
}

void Stats::kill(int killerFactionIndex, int killedFactionIndex, bool isEnemy, bool isDeathCounted, bool isKillCounted) {
	if(isKillCounted == true){
		playerStats[killerFactionIndex].kills++;
	}
	if(isDeathCounted == true){
		playerStats[killedFactionIndex].deaths++;
	}
	if(isEnemy == true && isKillCounted == true) {
		playerStats[killerFactionIndex].enemykills++;
	}
}

void Stats::die(int diedFactionIndex, bool isDeathCounted){
	if(isDeathCounted == true){
		playerStats[diedFactionIndex].deaths++;
	}
}

void Stats::produce(int producerFactionIndex, bool isProductionCounted){
	if(isProductionCounted == true){
		playerStats[producerFactionIndex].unitsProduced++;
	}
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

void Stats::saveGame(XmlNode *rootNode) {
	std::map<string,string> mapTagReplacements;
	XmlNode *statsNode = rootNode->addChild("Stats");

//	PlayerStats playerStats[GameConstants::maxPlayers];
	for(unsigned int i = 0; i < GameConstants::maxPlayers; ++i) {
		PlayerStats &stat = playerStats[i];

		XmlNode *statsNodePlayer = statsNode->addChild("Player");

//		ControlType control;
		statsNodePlayer->addAttribute("control",intToStr(stat.control), mapTagReplacements);
//		float resourceMultiplier;
		statsNodePlayer->addAttribute("resourceMultiplier",floatToStr(stat.resourceMultiplier,16), mapTagReplacements);
//		string factionTypeName;
		statsNodePlayer->addAttribute("factionTypeName",stat.factionTypeName, mapTagReplacements);
//		FactionPersonalityType personalityType;
		statsNodePlayer->addAttribute("personalityType",intToStr(stat.personalityType), mapTagReplacements);
//		int teamIndex;
		statsNodePlayer->addAttribute("teamIndex",intToStr(stat.teamIndex), mapTagReplacements);
//		bool victory;
		statsNodePlayer->addAttribute("victory",intToStr(stat.victory), mapTagReplacements);
//		int kills;
		statsNodePlayer->addAttribute("kills",intToStr(stat.kills), mapTagReplacements);
//		int enemykills;
		statsNodePlayer->addAttribute("enemykills",intToStr(stat.enemykills), mapTagReplacements);
//		int deaths;
		statsNodePlayer->addAttribute("deaths",intToStr(stat.deaths), mapTagReplacements);
//		int unitsProduced;
		statsNodePlayer->addAttribute("unitsProduced",intToStr(stat.unitsProduced), mapTagReplacements);
//		int resourcesHarvested;
		statsNodePlayer->addAttribute("resourcesHarvested",intToStr(stat.resourcesHarvested), mapTagReplacements);
//		string playerName;
		statsNodePlayer->addAttribute("playerName",stat.playerName, mapTagReplacements);
//		Vec3f playerColor;
		statsNodePlayer->addAttribute("playerColor",stat.playerColor.getString(), mapTagReplacements);
	}
//	string description;
	statsNode->addAttribute("description",description, mapTagReplacements);
//	int factionCount;
	statsNode->addAttribute("factionCount",intToStr(factionCount), mapTagReplacements);
//	int thisFactionIndex;
	statsNode->addAttribute("thisFactionIndex",intToStr(thisFactionIndex), mapTagReplacements);
//
//	float worldTimeElapsed;
	statsNode->addAttribute("worldTimeElapsed",floatToStr(worldTimeElapsed,16), mapTagReplacements);
//	int framesPlayed;
	statsNode->addAttribute("framesPlayed",intToStr(framesPlayed), mapTagReplacements);
//	int framesToCalculatePlaytime;
	statsNode->addAttribute("framesToCalculatePlaytime",intToStr(framesToCalculatePlaytime), mapTagReplacements);
//	time_t timePlayed;
	statsNode->addAttribute("timePlayed",intToStr(timePlayed), mapTagReplacements);
//	int maxConcurrentUnitCount;
	statsNode->addAttribute("maxConcurrentUnitCount",intToStr(maxConcurrentUnitCount), mapTagReplacements);
//	int totalEndGameConcurrentUnitCount;
	statsNode->addAttribute("totalEndGameConcurrentUnitCount",intToStr(totalEndGameConcurrentUnitCount), mapTagReplacements);
//	bool isMasterserverMode;
}

void Stats::loadGame(const XmlNode *rootNode) {
	const XmlNode *statsNode = rootNode->getChild("Stats");

	//	PlayerStats playerStats[GameConstants::maxPlayers];

	vector<XmlNode *> statsNodePlayerList = statsNode->getChildList("Player");
	for(unsigned int i = 0; i < statsNodePlayerList.size(); ++i) {
		XmlNode *statsNodePlayer = statsNodePlayerList[i];
		PlayerStats &stat = playerStats[i];

	//		ControlType control;
		stat.control = static_cast<ControlType>(statsNodePlayer->getAttribute("control")->getIntValue());
	//		float resourceMultiplier;
		stat.resourceMultiplier = statsNodePlayer->getAttribute("resourceMultiplier")->getFloatValue();
	//		string factionTypeName;
		stat.factionTypeName = statsNodePlayer->getAttribute("factionTypeName")->getValue();
	//		FactionPersonalityType personalityType;
		stat.personalityType = static_cast<FactionPersonalityType>(statsNodePlayer->getAttribute("personalityType")->getIntValue());
	//		int teamIndex;
		stat.teamIndex = statsNodePlayer->getAttribute("teamIndex")->getIntValue();
	//		bool victory;
		stat.victory = statsNodePlayer->getAttribute("victory")->getIntValue() != 0;
	//		int kills;
		stat.kills = statsNodePlayer->getAttribute("kills")->getIntValue();
	//		int enemykills;
		stat.enemykills = statsNodePlayer->getAttribute("enemykills")->getIntValue();
	//		int deaths;
		stat.deaths = statsNodePlayer->getAttribute("deaths")->getIntValue();
	//		int unitsProduced;
		stat.unitsProduced = statsNodePlayer->getAttribute("unitsProduced")->getIntValue();
	//		int resourcesHarvested;
		stat.resourcesHarvested = statsNodePlayer->getAttribute("resourcesHarvested")->getIntValue();
	//		string playerName;
		stat.playerName = statsNodePlayer->getAttribute("playerName")->getValue();
	//		Vec3f playerColor;
		stat.playerColor = Vec3f::strToVec3(statsNodePlayer->getAttribute("playerColor")->getValue());
	}
	//	string description;
	//statsNode->addAttribute("description",description, mapTagReplacements);
	description = statsNode->getAttribute("description")->getValue();
	//	int factionCount;
	factionCount = statsNode->getAttribute("factionCount")->getIntValue();
	//	int thisFactionIndex;
	thisFactionIndex = statsNode->getAttribute("thisFactionIndex")->getIntValue();
	//
	//	float worldTimeElapsed;
	worldTimeElapsed = statsNode->getAttribute("worldTimeElapsed")->getFloatValue();
	//	int framesPlayed;
	framesPlayed = statsNode->getAttribute("framesPlayed")->getIntValue();
	//	int framesToCalculatePlaytime;
	framesToCalculatePlaytime = statsNode->getAttribute("framesToCalculatePlaytime")->getIntValue();
	//	time_t timePlayed;
	timePlayed = statsNode->getAttribute("timePlayed")->getIntValue();
	//	int maxConcurrentUnitCount;
	maxConcurrentUnitCount = statsNode->getAttribute("maxConcurrentUnitCount")->getIntValue();
	//	int totalEndGameConcurrentUnitCount;
	totalEndGameConcurrentUnitCount = statsNode->getAttribute("totalEndGameConcurrentUnitCount")->getIntValue();
	//	bool isMasterserverMode;
}
}}//end namespace
