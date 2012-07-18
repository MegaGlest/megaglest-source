// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_SCENARIO_H_
#define _GLEST_GAME_SCENARIO_H_

#ifdef WIN32
    #include <winsock2.h>
    #include <winsock.h>
#endif

#include <string>
#include <vector>
#include "xml_parser.h"
#include "checksum.h"
#include "game_settings.h"

#include "leak_dumper.h"

using std::string;
using std::vector;
using std::pair;

using Shared::Xml::XmlNode;
using namespace Shared::Util;

namespace Glest { namespace Game {

enum Difficulty {
    dVeryEasy,
    dEasy,
    dMedium,
    dHard,
    dVeryHard,
    dInsane
};

class ScenarioInfo {

public:
	ScenarioInfo() {
		difficulty = 0;
		for(unsigned int i = 0; i < GameConstants::maxPlayers; ++i) {
			factionControls[i] = ctClosed;
			teams[i] = 0;
			factionTypeNames[i] = "";
			resourceMultipliers[i] = 0;
		}

	    mapName = "";
	    tilesetName = "";
	    techTreeName = "";

		defaultUnits = false;
		defaultResources = false;
		defaultVictoryConditions = false;

	    desc = "";

	    fogOfWar = false;
	    fogOfWar_exploredFlag = false;

	    file = "";
	    name = "";
	}
	int difficulty;
    ControlType factionControls[GameConstants::maxPlayers];
    int teams[GameConstants::maxPlayers];
    string factionTypeNames[GameConstants::maxPlayers];
    float resourceMultipliers[GameConstants::maxPlayers];

    string mapName;
    string tilesetName;
    string techTreeName;

	bool defaultUnits;
	bool defaultResources;
	bool defaultVictoryConditions;

    string desc;

    bool fogOfWar;
    bool fogOfWar_exploredFlag;

    string file;
    string name;
};

// =====================================================
//	class Script
// =====================================================

class Script {
private:
	string name;
	string code;

public:
	Script(const string &name, const string &code)	{this->name= name; this->code= code;}

	const string &getName() const	{return name;}
	const string &getCode() const	{return code;}
};

// =====================================================
//	class Scenario
// =====================================================

class Scenario {
private:
	typedef pair<string, string> NameScriptPair;
	typedef vector<Script> Scripts;

	ScenarioInfo info;
	Scripts scripts;
	Checksum checksumValue;

public:
    ~Scenario();
	Checksum load(const string &path);
	Checksum * getChecksumValue() { return &checksumValue; }

	int getScriptCount() const				{return scripts.size();}
	const Script* getScript(int i) const	{return &scripts[i];}

	ScenarioInfo getInfo() const { return info; }

	static string getScenarioPath(const vector<string> dir, const string &scenarioName, bool getMatchingRootScenarioPathOnly=false);
	static string getScenarioPath(const string &dir, const string &scenarioName);
	static int getScenarioPathIndex(const vector<string> dirList, const string &scenarioName);
	static string getScenarioDir(const vector<string> dir, const string &scenarioName);

	static void loadScenarioInfo(string file, ScenarioInfo *scenarioInfo);
	static ControlType strToControllerType(const string &str);
	static string controllerTypeToStr(const ControlType &ct);

	static void loadGameSettings(const vector<string> &dirList, const ScenarioInfo *scenarioInfo, GameSettings *gameSettings, string scenarioDescription);

private:
	string getFunctionName(const XmlNode *scriptNode);
};

}}//end namespace

#endif
