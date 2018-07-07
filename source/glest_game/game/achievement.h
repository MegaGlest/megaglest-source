// ==============================================================
//	This file is part of Glest (www.megaglest.org)
//
//	Copyright (C) 2001-2008 Titus Tscharntke
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================
#ifndef _GLEST_GAME_ACHIEVEMENT_H_
#define _GLEST_GAME_ACHIEVEMENT_H_

#ifdef WIN32
    #include <winsock2.h>
    #include <winsock.h>
#endif

#include <string>
#include "util.h"
#include "vec.h"
#include "xml_parser.h"
#include "leak_dumper.h"

using Shared::Xml::XmlNode;
using std::vector;

namespace Glest {
namespace Game {

class Game;
class PlayerAchievementsInterface;
// =====================================================
// 	class AchievementBase
///	base achievement class
// =====================================================

class AchievementBase {
private:
	string name;
	string description;
	string pictureName;

public:
	AchievementBase();
	virtual ~AchievementBase();
	virtual void load(const XmlNode *node);

	string getDescription() const {
		return description;
	}

	string getName() const {
		return name;
	}

	string getPictureName() const {
		return pictureName;
	}

	virtual bool checkAchieved(Game *game, PlayerAchievementsInterface *playerStats) {
		return false;
	}

};

// =====================================================
// 	class CounterBasedAchievement
//	achievements based on counters
// =====================================================

class CounterBasedAchievement: public AchievementBase{
private:
	string counterName;
	int minCount;

public:
	CounterBasedAchievement();
	~CounterBasedAchievement();

	void load(const XmlNode *node);

	string getCounterName() const {
		return counterName;
	}

	int getMinCount() const {
		return minCount;
	}

	virtual bool checkAchieved(Game *game, PlayerAchievementsInterface *playerStats);
};

// =====================================================
// 	class Achievements
//	a class holding all achievements
// =====================================================

typedef vector<AchievementBase*> AchievementVector;

class Achievements {
private:
	AchievementVector achievements;
	Achievements();
public:
	static const   AchievementVector* getAchievements();
private:
	 void load(  string xmlFilePath);
};

}
} //end namespace

#endif
