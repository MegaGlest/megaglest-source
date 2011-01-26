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

#include "config.h"

#include "util.h"
#include "game_constants.h"
#include "platform_util.h"
#include "game_util.h"
#include <map>
#include "conversion.h"
#include "window.h"
#include <stdexcept>
#include <fstream>
#include "leak_dumper.h"

using namespace Shared::Platform;
using namespace Shared::Util;
using namespace std;

namespace Glest{ namespace Game{

int GameConstants::networkFramePeriod				= 20;
int GameConstants::updateFps						= 40;
int GameConstants::cameraFps						= 100;

const float GameConstants::normalMultiplier			= 1.0f;
const float GameConstants::easyMultiplier			= 0.5f;
const float GameConstants::ultraMultiplier			= 2.0f;
const float GameConstants::megaMultiplier			= 3.0f;

const char *GameConstants::folder_path_maps         = "maps";
const char *GameConstants::folder_path_scenarios    = "scenarios";
const char *GameConstants::folder_path_techs        = "techs";
const char *GameConstants::folder_path_tilesets     = "tilesets";
const char *GameConstants::folder_path_tutorials    = "tutorials";

const char *GameConstants::NETWORK_SLOT_UNCONNECTED_SLOTNAME = "???";

const char *GameConstants::folder_path_screenshots	= "screens/";

const char *GameConstants::OBSERVER_SLOTNAME		= "*Observer*";
const char *GameConstants::RANDOMFACTION_SLOTNAME	= "*Random*";

const char *GameConstants::playerTextureCacheLookupKey  		= "playerTextureCache";
const char *GameConstants::factionPreviewTextureCacheLookupKey  = "factionPreviewTextureCache";
const char *GameConstants::application_name				= "MegaGlest";

const char *GameConstants::pathCacheLookupKey           = "pathCache_";
const char *GameConstants::path_data_CacheLookupKey     = "data";
const char *GameConstants::path_ini_CacheLookupKey      = "ini";
const char *GameConstants::path_logs_CacheLookupKey     = "logs";

const char *Config::glest_ini_filename                  = "glest.ini";
const char *Config::glestuser_ini_filename              = "glestuser.ini";


// =====================================================
// 	class Config
// =====================================================

const string defaultNotFoundValue = "~~NOT FOUND~~";

map<ConfigType,Config> Config::configList;

Config::Config() {
	fileLoaded.first = false;
	fileLoaded.second = false;
	cfgType.first = cfgMainGame;
	cfgType.second = cfgUserGame;
	fileName.first = "";
	fileName.second = "";
	fileLoaded.first = false;
	fileLoaded.second = false;
}

Config::Config(std::pair<ConfigType,ConfigType> type, std::pair<string,string> file, std::pair<bool,bool> fileMustExist) {
	fileLoaded.first = false;
	fileLoaded.second = false;
	cfgType = type;

	fileName = file;
    if(getGameReadWritePath(GameConstants::path_ini_CacheLookupKey) != "") {
    	fileName.first = getGameReadWritePath(GameConstants::path_ini_CacheLookupKey) + fileName.first;
    	fileName.second = getGameReadWritePath(GameConstants::path_ini_CacheLookupKey) + fileName.second;
    }

    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] cfgFile.first = [%s]\n",__FILE__,__FUNCTION__,__LINE__,fileName.first.c_str());

    if(fileMustExist.first == true ||
    	(fileMustExist.first == false && fileExists(fileName.first) == true)) {
    	properties.first.load(fileName.first);
    	fileLoaded.first = true;

    	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] cfgFile.first = [%s]\n",__FILE__,__FUNCTION__,__LINE__,fileName.first.c_str());
    }
    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] cfgFile.first = [%s]\n",__FILE__,__FUNCTION__,__LINE__,fileName.first.c_str());

    if(properties.first.getString("UserOverrideFile", defaultNotFoundValue.c_str()) != defaultNotFoundValue) {
    	fileName.second = properties.first.getString("UserOverrideFile");
    }

    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] cfgFile.second = [%s]\n",__FILE__,__FUNCTION__,__LINE__,fileName.second.c_str());

    if(fileMustExist.second == true ||
    	(fileMustExist.second == false && fileExists(fileName.second) == true)) {
    	properties.second.load(fileName.second);
    	fileLoaded.second = true;

    	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] cfgFile.second = [%s]\n",__FILE__,__FUNCTION__,__LINE__,fileName.second.c_str());
    }

    try {
		if(fileName.second != "" && fileExists(fileName.second) == false) {
			if(SystemFlags::VERBOSE_MODE_ENABLED) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] attempting to auto-create cfgFile.second = [%s]\n",__FILE__,__FUNCTION__,__LINE__,fileName.second.c_str());

			std::ofstream userFile;
			userFile.open(fileName.second.c_str(), ios_base::out | ios_base::trunc);
			userFile.close();
		}
    }
    catch(const exception &ex) {
    	SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
    	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] ERROR trying to auto-create cfgFile.second = [%s]\n",__FILE__,__FUNCTION__,__LINE__,fileName.second.c_str());
    }

    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] cfgFile.second = [%s]\n",__FILE__,__FUNCTION__,__LINE__,fileName.second.c_str());
}

Config &Config::getInstance(std::pair<ConfigType,ConfigType> type, std::pair<string,string> file, std::pair<bool,bool> fileMustExist) {
	if(configList.find(type.first) == configList.end()) {
		if(SystemFlags::VERBOSE_MODE_ENABLED) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		Config config(type, file, fileMustExist);

		if(SystemFlags::VERBOSE_MODE_ENABLED) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		configList.insert(map<ConfigType,Config>::value_type(type.first,config));

		if(SystemFlags::VERBOSE_MODE_ENABLED) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}

	return configList.find(type.first)->second;
}

void Config::CopyAll(Config *src, Config *dest) {

	dest->properties	= src->properties;
	dest->cfgType		= src->cfgType;
	dest->fileName		= src->fileName;
	dest->fileLoaded	= src->fileLoaded;
}

void Config::reload() {
	if(SystemFlags::VERBOSE_MODE_ENABLED) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	std::pair<ConfigType,ConfigType> type = std::make_pair(cfgMainGame,cfgUserGame);
	Config newconfig(type, std::make_pair(glest_ini_filename,glestuser_ini_filename), std::make_pair(true,false));

	if(SystemFlags::VERBOSE_MODE_ENABLED) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	Config &oldconfig = configList.find(type.first)->second;
	CopyAll(&newconfig, &oldconfig);

	if(SystemFlags::VERBOSE_MODE_ENABLED) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void Config::save(const string &path){
	if(fileLoaded.second == true) {
		if(path != "") {
			fileName.second = path;
		}
		properties.second.save(fileName.second);
		return;
	}

	if(path != "") {
		fileName.first = path;
	}
	properties.first.save(fileName.first);
}

int Config::getInt(const char *key,const char *defaultValueIfNotFound) const {
	if(fileLoaded.second == true &&
		properties.second.getString(key, defaultNotFoundValue.c_str()) != defaultNotFoundValue) {
		return properties.second.getInt(key,defaultValueIfNotFound);
	}
    return properties.first.getInt(key,defaultValueIfNotFound);
}

bool Config::getBool(const char *key,const char *defaultValueIfNotFound) const {
	if(fileLoaded.second == true &&
		properties.second.getString(key, defaultNotFoundValue.c_str()) != defaultNotFoundValue) {
		return properties.second.getBool(key,defaultValueIfNotFound);
	}

	return properties.first.getBool(key,defaultValueIfNotFound);
}

float Config::getFloat(const char *key,const char *defaultValueIfNotFound) const {
	if(fileLoaded.second == true &&
		properties.second.getString(key, defaultNotFoundValue.c_str()) != defaultNotFoundValue) {
		return properties.second.getFloat(key,defaultValueIfNotFound);
	}

	return properties.first.getFloat(key,defaultValueIfNotFound);
}

const string Config::getString(const char *key,const char *defaultValueIfNotFound) const {
	if(fileLoaded.second == true &&
		properties.second.getString(key, defaultNotFoundValue.c_str()) != defaultNotFoundValue) {
		return properties.second.getString(key,defaultValueIfNotFound);
	}

	return properties.first.getString(key,defaultValueIfNotFound);
}

int Config::getInt(const string &key,const char *defaultValueIfNotFound) const{
	if(fileLoaded.second == true &&
		properties.second.getString(key, defaultNotFoundValue.c_str()) != defaultNotFoundValue) {
		return properties.second.getInt(key,defaultValueIfNotFound);
	}

	return properties.first.getInt(key,defaultValueIfNotFound);
}

bool Config::getBool(const string &key,const char *defaultValueIfNotFound) const{
	if(fileLoaded.second == true &&
		properties.second.getString(key, defaultNotFoundValue.c_str()) != defaultNotFoundValue) {
		return properties.second.getBool(key,defaultValueIfNotFound);
	}

	return properties.first.getBool(key,defaultValueIfNotFound);
}

float Config::getFloat(const string &key,const char *defaultValueIfNotFound) const{
	if(fileLoaded.second == true &&
		properties.second.getString(key, defaultNotFoundValue.c_str()) != defaultNotFoundValue) {
		return properties.second.getFloat(key,defaultValueIfNotFound);
	}

	return properties.first.getFloat(key,defaultValueIfNotFound);
}

const string Config::getString(const string &key,const char *defaultValueIfNotFound) const{
	if(fileLoaded.second == true &&
		properties.second.getString(key, defaultNotFoundValue.c_str()) != defaultNotFoundValue) {
		return properties.second.getString(key,defaultValueIfNotFound);
	}

	return properties.first.getString(key,defaultValueIfNotFound);
}

char Config::translateStringToCharKey(const string &value) const {
	char result = 0;

	if(IsNumeric(value.c_str()) == true) {
		result = strToInt(value);
	}
	else if(value.substr(0,2) == "vk") {
		if(value == "vkLeft") {
			result = vkLeft;
		}
		else if(value == "vkRight") {
			result = vkRight;
		}
		else if(value == "vkUp") {
			result = vkUp;
		}
		else if(value == "vkDown") {
			result = vkDown;
		}
		else if(value == "vkAdd") {
			result = vkAdd;
		}
		else if(value == "vkSubtract") {
			result = vkSubtract;
		}
		else if(value == "vkEscape") {
			result = vkEscape;
		}
		else if(value == "vkF1") {
			result = vkF1;
		}
		else if(value == "vkF2") {
			result = vkF2;
		}
		else if(value == "vkF3") {
			result = vkF3;
		}
		else if(value == "vkF4") {
			result = vkF4;
		}
		else if(value == "vkF5") {
			result = vkF5;
		}
		else if(value == "vkF6") {
			result = vkF6;
		}
		else if(value == "vkF7") {
			result = vkF7;
		}
		else if(value == "vkF8") {
			result = vkF8;
		}
		else if(value == "vkF9") {
			result = vkF9;
		}
		else if(value == "vkF10") {
			result = vkF10;
		}
		else if(value == "vkF11") {
			result = vkF11;
		}
		else if(value == "vkF12") {
			result = vkF12;
		}
		else if(value == "vkPrint") {
			result = vkPrint;
		}
		else if(value == "vkPause") {
			result = vkPause;
		}

		else {
			string sError = "Unsupported key translation [" + value + "]";
			throw runtime_error(sError.c_str());
		}
	}
	else if(value.length() >= 1) {
		if(value.length() == 3 && value[0] == '\'' && value[2] == '\'') {
			result = value[1];
		}
		else {
			result = value[0];
		}
	}
	else {
		string sError = "Unsupported key translation" + value;
		throw runtime_error(sError.c_str());
	}

	return result;
}

char Config::getCharKey(const char *key) const {
	if(fileLoaded.second == true &&
		properties.second.getString(key, defaultNotFoundValue.c_str()) != defaultNotFoundValue) {

		string value = properties.second.getString(key);
		return translateStringToCharKey(value);
	}
	string value = properties.first.getString(key);
	return translateStringToCharKey(value);
}

void Config::setInt(const string &key, int value){
	if(fileLoaded.second == true) {
		properties.second.setInt(key, value);
		return;
	}
	properties.first.setInt(key, value);
}

void Config::setBool(const string &key, bool value){
	if(fileLoaded.second == true) {
		properties.second.setBool(key, value);
		return;
	}

	properties.first.setBool(key, value);
}

void Config::setFloat(const string &key, float value){
	if(fileLoaded.second == true) {
		properties.second.setFloat(key, value);
		return;
	}

	properties.first.setFloat(key, value);
}

void Config::setString(const string &key, const string &value){
	if(fileLoaded.second == true) {
		properties.second.setString(key, value);
		return;
	}

	properties.first.setString(key, value);
}

vector<pair<string,string> > Config::getPropertiesFromContainer(const Properties &propertiesObj) const {
    vector<pair<string,string> > result;

	int count = propertiesObj.getPropertyCount();
	for(int i = 0; i < count; ++i) {
	    pair<string,string> property;
        property.first  = propertiesObj.getKey(i);
        property.second = propertiesObj.getString(i);
        result.push_back(property);
	}

    return result;
}

vector<pair<string,string> > Config::getMergedProperties() const {
    vector<pair<string,string> > result = getMasterProperties();
    vector<pair<string,string> > resultUser = getUserProperties();
    for(int i = 0; i < resultUser.size(); ++i) {
        const pair<string,string> &propertyUser = resultUser[i];
        bool overrideProperty = false;
        for(int j = 0; j < result.size(); ++j) {
            pair<string,string> &property = result[j];
            // Take the user property and override the original value
            if(property.first == propertyUser.first) {
                overrideProperty = true;
                property.second = propertyUser.second;
                break;
            }
        }
        if(overrideProperty == false) {
            result.push_back(propertyUser);
        }
    }

    return result;
}

vector<pair<string,string> > Config::getMasterProperties() const {
    return getPropertiesFromContainer(properties.first);
}

vector<pair<string,string> > Config::getUserProperties() const {
    return getPropertiesFromContainer(properties.second);
}

string Config::toString(){
	return properties.first.toString();
}

vector<string> Config::getPathListForType(PathType type, string scenarioDir) {
    vector<string> pathList;

//#include "game_constants.h"
//#include "game_util.h"
    string data_path = getGameReadWritePath(GameConstants::path_data_CacheLookupKey);

    string userData = getString("UserData_Root","");
    if(userData != "") {
        if(userData[userData.size()-1] != '/' && userData[userData.size()-1] != '\\') {
            userData += '/';
        }
        userData=data_path+userData;
        if(isdir(userData.c_str()) == false) {
        	createDirectoryPaths(userData);
        }

        string userDataMaps = userData + GameConstants::folder_path_maps;
        if(isdir(userDataMaps.c_str()) == false) {
        	createDirectoryPaths(userDataMaps);
        }
        string userDataScenarios = userData + GameConstants::folder_path_scenarios;
        if(isdir(userDataScenarios.c_str()) == false) {
        	createDirectoryPaths(userDataScenarios);
        }
        string userDataTechs = userData + GameConstants::folder_path_techs;
        if(isdir(userDataTechs.c_str()) == false) {
        	createDirectoryPaths(userDataTechs);
        }
        string userDataTilesets = userData + GameConstants::folder_path_tilesets;
        if(isdir(userDataTilesets.c_str()) == false) {
        	createDirectoryPaths(userDataTilesets);
        }
        string userDataTutorials = userData + GameConstants::folder_path_tutorials;
        if(isdir(userDataTutorials.c_str()) == false) {
        	createDirectoryPaths(userDataTutorials);
        }
    }
    if(scenarioDir != "") {
        pathList.push_back(data_path+scenarioDir);
    }

    switch(type) {
        case ptMaps:
            pathList.push_back(data_path+GameConstants::folder_path_maps);
            if(userData != "") {
                pathList.push_back(userData + string(GameConstants::folder_path_maps));
            }
            break;
        case ptScenarios:
            pathList.push_back(data_path+GameConstants::folder_path_scenarios);
            if(userData != "") {
                pathList.push_back(userData + string(GameConstants::folder_path_scenarios));
            }
            break;
        case ptTechs:
            pathList.push_back(data_path+GameConstants::folder_path_techs);
            if(userData != "") {
                pathList.push_back(userData + string(GameConstants::folder_path_techs));
            }
            break;
        case ptTilesets:
            pathList.push_back(data_path+GameConstants::folder_path_tilesets);
            if(userData != "") {
                pathList.push_back(userData + string(GameConstants::folder_path_tilesets));
            }
            break;
        case ptTutorials:
            pathList.push_back(data_path+GameConstants::folder_path_tutorials);
            if(userData != "") {
                pathList.push_back(userData + string(GameConstants::folder_path_tutorials));
            }
            break;
    }

    return pathList;
}

}}// end namespace
