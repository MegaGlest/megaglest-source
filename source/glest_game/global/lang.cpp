// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martio Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "lang.h"

#include <stdexcept>

#include "logger.h"
#include "util.h"
#include "platform_util.h"
#include "game_constants.h"
#include "game_util.h"
#include "leak_dumper.h"

using namespace std;
using namespace Shared::Util;
using namespace Shared::Platform;

namespace Glest{ namespace Game{

// =====================================================
// 	class Lang
// =====================================================

Lang &Lang::getInstance(){
	static Lang lang;
	return lang;
}

void Lang::loadStrings(const string &language){
	this->language= language;
	strings.clear();
	string data_path = getGameReadWritePath(GameConstants::path_data_CacheLookupKey);
	strings.load(data_path + "data/lang/"+language+".lng");
}

void Lang::loadScenarioStrings(const string &scenarioDir, const string &scenarioName){
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] scenarioDir = [%s] scenarioName = [%s]\n",__FILE__,__FUNCTION__,__LINE__,scenarioDir.c_str(),scenarioName.c_str());

	string currentPath = scenarioDir;
	endPathWithSlash(currentPath);
	string scenarioFolder = currentPath + scenarioName + "/";
	string path = scenarioFolder + scenarioName + "_" + language + ".lng";
	if(EndsWith(scenarioDir, ".xml") == true) {
		scenarioFolder = extractDirectoryPathFromFile(scenarioDir);
		path = scenarioFolder + scenarioName + "_" + language + ".lng";

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] path = [%s]\n",__FILE__,__FUNCTION__,__LINE__,path.c_str());
	}

	scenarioStrings.clear();

	//try to load the current language first
	if(fileExists(path)) {
		scenarioStrings.load(path);
	}
	else{
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] path not found [%s]\n",__FILE__,__FUNCTION__,__LINE__,path.c_str());

		//try english otherwise
		path = scenarioFolder + scenarioName + "_english.lng";
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] path = [%s]\n",__FILE__,__FUNCTION__,__LINE__,path.c_str());

		if(fileExists(path)){
			scenarioStrings.load(path);
		}
	}
}

bool Lang::hasString(const string &s) {
	bool hasString = false;
	try {
		string result = strings.getString(s);
		hasString = true;
	}
	catch(exception &ex) {
		if(strings.getpath() != "") {
			SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		}
	}
	return hasString;
}

string Lang::get(const string &s) {
	try{
		string result = strings.getString(s);
		replaceAll(result, "\\n", "\n");
		return result;
	}
	catch(exception &ex) {
		if(strings.getpath() != "") {
			SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		}
		return "???" + s + "???";
	}
}

string Lang::getScenarioString(const string &s) {
	try{
		return scenarioStrings.getString(s);
	}
	catch(exception &ex) {
		if(scenarioStrings.getpath() != "") {
			SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		}
		return "???" + s + "???";
	}
}

}}//end namespace
