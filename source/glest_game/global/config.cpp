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

vector<string> Config::getPathListForType(PathType type) {
    vector<string> pathList;

    switch(type) {
        case ptMaps:
            pathList.push_back(GameConstants::folder_path_maps);
            if(getString("UserData_Maps","") != "") {
                pathList.push_back(getString("UserData_Maps"));
            }
            break;
        case ptScenarios:
            pathList.push_back(GameConstants::folder_path_scenarios);
            if(getString("UserData_Scenarios","") != "") {
                pathList.push_back(getString("UserData_Scenarios"));
            }
            break;
        case ptTechs:
            pathList.push_back(GameConstants::folder_path_techs);
            if(getString("UserData_Techs","") != "") {
                pathList.push_back(getString("UserData_Techs"));
            }
            break;
        case ptTilesets:
            pathList.push_back(GameConstants::folder_path_tilesets);
            if(getString("UserData_Tilesets","") != "") {
                pathList.push_back(getString("UserData_Tilesets"));
            }
            break;
        case ptTutorials:
            pathList.push_back(GameConstants::folder_path_tutorials);
            if(getString("UserData_Tutorials","") != "") {
                pathList.push_back(getString("UserData_Tutorials"));
            }
            break;
    }

    return pathList;
}

}}// end namespace
