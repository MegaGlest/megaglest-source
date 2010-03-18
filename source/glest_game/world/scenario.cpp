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

#include "scenario.h"

#include <stdexcept>

#include "logger.h"
#include "xml_parser.h"
#include "util.h"
#include "game_util.h"
#include "leak_dumper.h"
#include <stdio.h>

using namespace Shared::Xml;
using namespace Shared::Util;
using namespace std;

namespace Glest{ namespace Game{

// =====================================================
//	class Scenario
// =====================================================

Scenario::~Scenario(){

}

void Scenario::load(const string &path){
	try{
		string name= cutLastExt(lastDir(path));
		Logger::getInstance().add("Scenario: "+formatString(name), true);

		//parse xml
		XmlTree xmlTree;
		xmlTree.load(path);
		const XmlNode *scenarioNode= xmlTree.getRootNode();
		const XmlNode *scriptsNode= scenarioNode->getChild("scripts");

		for(int i= 0; i<scriptsNode->getChildCount(); ++i){
			const XmlNode *scriptNode = scriptsNode->getChild(i);

			scripts.push_back(Script(getFunctionName(scriptNode), scriptNode->getText()));
		}
	}
	//Exception handling (conversions and so on);
	catch(const exception &e){
		throw runtime_error("Error: " + path + "\n" + e.what());
	}
}

int Scenario::getScenarioPathIndex(const vector<string> dirList, const string &scenarioName) {
    int iIndex = 0;
    for(int idx = 0; idx < dirList.size(); idx++) {
        string scenarioFile = dirList[idx] + "/" + scenarioName + "/" + scenarioName + ".xml";
        if(fileExists(scenarioFile) == true) {
            iIndex = idx;
            break;
        }
    }

    return iIndex;
}

string Scenario::getScenarioPath(const vector<string> dirList, const string &scenarioName){
    string scenarioFile = "";
    for(int idx = 0; idx < dirList.size(); idx++) {
        scenarioFile = dirList[idx] + "/" + scenarioName + "/" + scenarioName + ".xml";
        if(fileExists(scenarioFile) == true) {
            break;
        }
    }

    return scenarioFile;
}

string Scenario::getScenarioPath(const string &dir, const string &scenarioName){
    string scenarioFile = dir + "/" + scenarioName + "/" + scenarioName + ".xml";
    //printf("dir [%s] scenarioName [%s] scenarioFile [%s]\n",dir.c_str(),scenarioName.c_str(),scenarioFile.c_str());

    return scenarioFile;
}

string Scenario::getFunctionName(const XmlNode *scriptNode){
	string name= scriptNode->getName();

	for(int i= 0; i<scriptNode->getAttributeCount(); ++i){
		name+= "_" + scriptNode->getAttribute(i)->getValue();
	}
	return name;
}

}}//end namespace
