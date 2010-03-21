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

#include "config.h"

#include "util.h"

#include "leak_dumper.h"
#include "game_constants.h"
#include "platform_util.h"

using namespace Shared::Platform;

namespace Glest{ namespace Game{


const char *GameConstants::folder_path_maps         = "maps";
const char *GameConstants::folder_path_scenarios    = "scenarios";
const char *GameConstants::folder_path_techs        = "techs";
const char *GameConstants::folder_path_tilesets     = "tilesets";
const char *GameConstants::folder_path_tutorials    = "tutorials";


// =====================================================
// 	class Config
// =====================================================

Config::Config(){
	properties.load("glest.ini");
}

Config &Config::getInstance(){
	static Config config;
	return config;
}

void Config::save(const string &path){
	properties.save(path);
}

int Config::getInt(const char *key,const char *defaultValueIfNotFound) const {
    return properties.getInt(key,defaultValueIfNotFound);
}

bool Config::getBool(const char *key,const char *defaultValueIfNotFound) const {
    return properties.getBool(key,defaultValueIfNotFound);
}

float Config::getFloat(const char *key,const char *defaultValueIfNotFound) const {
    return properties.getFloat(key,defaultValueIfNotFound);
}

const string Config::getString(const char *key,const char *defaultValueIfNotFound) const {
    return properties.getString(key,defaultValueIfNotFound);
}

int Config::getInt(const string &key,const char *defaultValueIfNotFound) const{
	return properties.getInt(key,defaultValueIfNotFound);
}

bool Config::getBool(const string &key,const char *defaultValueIfNotFound) const{
	return properties.getBool(key,defaultValueIfNotFound);
}

float Config::getFloat(const string &key,const char *defaultValueIfNotFound) const{
	return properties.getFloat(key,defaultValueIfNotFound);
}

const string Config::getString(const string &key,const char *defaultValueIfNotFound) const{
	return properties.getString(key,defaultValueIfNotFound);
}

void Config::setInt(const string &key, int value){
	properties.setInt(key, value);
}

void Config::setBool(const string &key, bool value){
	properties.setBool(key, value);
}

void Config::setFloat(const string &key, float value){
	properties.setFloat(key, value);
}

void Config::setString(const string &key, const string &value){
	properties.setString(key, value);
}

string Config::toString(){
	return properties.toString();
}

vector<string> Config::getPathListForType(PathType type, string scenarioDir) {
    vector<string> pathList;

    string userData = getString("UserData_Root","");
    if(userData != "") {
        if(userData[userData.size()-1] != '/' && userData[userData.size()-1] != '\\') {
            userData += '/';
        }
    }
    if(scenarioDir != "") {
        pathList.push_back(scenarioDir);
    }

    switch(type) {
        case ptMaps:
            pathList.push_back(GameConstants::folder_path_maps);
            if(userData != "") {
                pathList.push_back(userData + string(GameConstants::folder_path_maps));
            }
            break;
        case ptScenarios:
            pathList.push_back(GameConstants::folder_path_scenarios);
            if(userData != "") {
                pathList.push_back(userData + string(GameConstants::folder_path_scenarios));
            }
            break;
        case ptTechs:
            pathList.push_back(GameConstants::folder_path_techs);
            if(userData != "") {
                pathList.push_back(userData + string(GameConstants::folder_path_techs));
            }
            break;
        case ptTilesets:
            pathList.push_back(GameConstants::folder_path_tilesets);
            if(userData != "") {
                pathList.push_back(userData + string(GameConstants::folder_path_tilesets));
            }
            break;
        case ptTutorials:
            pathList.push_back(GameConstants::folder_path_tutorials);
            if(userData != "") {
                pathList.push_back(userData + string(GameConstants::folder_path_tutorials));
            }
            break;
    }

    return pathList;
}

}}// end namespace
