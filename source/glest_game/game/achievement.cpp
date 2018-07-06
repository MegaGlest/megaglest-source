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

#include "achievement.h"
#include "game.h"
#include "world.h"
#include "steam.h"
#include "leak_dumper.h"
#include "config.h"
#include "core_data.h"


namespace Glest{ namespace Game{

AchievementBase::AchievementBase() {
	name = "";
	description = "";
	pictureName = "";
}

AchievementBase::~AchievementBase(){
}

void AchievementBase::load(const XmlNode *node) {
	name= node->getAttribute("name")->getRestrictedValue();
   description = node->getAttribute("description")->getRestrictedValue();
   pictureName = node->getAttribute("pictureName")->getRestrictedValue();
}
// =====================================================
// 	class CounterBasedAchievement
// =====================================================

CounterBasedAchievement::CounterBasedAchievement():AchievementBase(){
	counterName="";
	minCount=0;
}
CounterBasedAchievement::~CounterBasedAchievement(){
}

void  CounterBasedAchievement::load(const XmlNode *node){
	AchievementBase::load(node);
	counterName= node->getAttribute("counterName")->getRestrictedValue();
	minCount= node->getAttribute("minCount")->getIntValue();
	printf("achievementName=%s\n",getName().c_str());
}

bool CounterBasedAchievement::checkAchieved(Game *game, PlayerAchievementsInterface *playerStats) {
	if (playerStats->getStatAsInt(counterName.c_str()) >= minCount)
		return true;
	else
		return false;
}

// =====================================================
// 	class Achievements
// =====================================================
Achievements::Achievements(){
	Config &config=Config::getInstance();
	string dataPath= getGameReadWritePath(GameConstants::path_data_CacheLookupKey);
	string filepath=getGameCustomCoreDataPath(dataPath, "data/achievements/achievements.xml");
	load(filepath);
}

const AchievementVector* Achievements::getAchievements(){
	static Achievements instance;
	return &(instance.achievements);
}

void Achievements::load( string xmlFilePath){
	XmlTree	xmlTree;
	xmlTree.load(xmlFilePath,Properties::getTagReplacementValues());
	const XmlNode *node= xmlTree.getRootNode();
	for (int i=0; i<node->getChildCount();++i){
		XmlNode *currentNode=node->getChild(i);
		if("counterBasedAchievement"==currentNode->getName()){
			CounterBasedAchievement a;
			a.load(currentNode);
			achievements.push_back(&a);
		}
	}

}

}}//end namespace
