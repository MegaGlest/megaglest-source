// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Martio Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_SCENARIO_H_
#define _GLEST_GAME_SCENARIO_H_

#include <string>
#include <vector>
#include "xml_parser.h"
#include "checksum.h"
#include "leak_dumper.h"

using std::string;
using std::vector;
using std::pair;

using Shared::Xml::XmlNode;
using namespace Shared::Util;

namespace Glest { namespace Game {

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

	Scripts scripts;
	Checksum checksumValue;

public:
    ~Scenario();
	Checksum load(const string &path);
	Checksum * getChecksumValue() { return &checksumValue; }

	int getScriptCount() const				{return scripts.size();}
	const Script* getScript(int i) const	{return &scripts[i];}

	static string getScenarioPath(const vector<string> dir, const string &scenarioName, bool getMatchingRootScenarioPathOnly=false);
	static string getScenarioPath(const string &dir, const string &scenarioName);
	static int getScenarioPathIndex(const vector<string> dirList, const string &scenarioName);

private:
	string getFunctionName(const XmlNode *scriptNode);
};

}}//end namespace

#endif
