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

#include "leak_dumper.h"

namespace Glest{ namespace Game{

PlayerStats::PlayerStats(){
	victory= false;

	kills= 0;
	deaths= 0;
	unitsProduced= 0;
	resourcesHarvested= 0;
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

void Stats::kill(int killerFactionIndex, int killedFactionIndex){
	playerStats[killerFactionIndex].kills++;
	playerStats[killedFactionIndex].deaths++;
}

void Stats::produce(int producerFactionIndex){
	playerStats[producerFactionIndex].unitsProduced++;
}

void Stats::harvest(int harvesterFactionIndex, int amount){
	playerStats[harvesterFactionIndex].resourcesHarvested+= amount;
}

}}//end namespace
