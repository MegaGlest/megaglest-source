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

#ifndef _GLEST_GAME_CONFIG_H_
#define _GLEST_GAME_CONFIG_H_

#include "properties.h"
#include <vector>
#include "game_constants.h"
#include <SDL.h>
#include "leak_dumper.h"

namespace Glest{ namespace Game{

using Shared::Util::Properties;

// =====================================================
// 	class Config
//
//	Game configuration
// =====================================================

enum ConfigType {
    cfgMainGame,
    cfgUserGame,
    cfgMainKeys,
    cfgUserKeys
};

class Config {
private:
	std::pair<Properties,Properties> properties;
	std::pair<ConfigType,ConfigType> cfgType;
	std::pair<string,string> fileNameParameter;
	std::pair<string,string> fileName;
	std::pair<bool,bool> fileLoaded;

	static map<ConfigType,Config> configList;

    static const char *glest_ini_filename;
    static const char *glestuser_ini_filename;

public:
    static const char *glestkeys_ini_filename;
    static const char *glestuserkeys_ini_filename;

protected:
	Config();
	Config(std::pair<ConfigType,ConfigType> type, std::pair<string,string> file, std::pair<bool,bool> fileMustExist);
	bool tryCustomPath(std::pair<ConfigType,ConfigType> &type, std::pair<string,string> &file, string custom_path);
	static void CopyAll(Config *src,Config *dest);
	vector<pair<string,string> > getPropertiesFromContainer(const Properties &propertiesObj) const;

public:
    static Config &getInstance(std::pair<ConfigType,ConfigType> type = std::make_pair(cfgMainGame,cfgUserGame) ,
				std::pair<string,string> file = std::make_pair(glest_ini_filename,glestuser_ini_filename) ,
				std::pair<bool,bool> fileMustExist = std::make_pair(true,false) );
	void save(const string &path="");
	void reload();

	int getInt(const string &key,const char *defaultValueIfNotFound=NULL) const;
	bool getBool(const string &key,const char *defaultValueIfNotFound=NULL) const;
	float getFloat(const string &key,const char *defaultValueIfNotFound=NULL) const;
	const string getString(const string &key,const char *defaultValueIfNotFound=NULL) const;

	int getInt(const char *key,const char *defaultValueIfNotFound=NULL) const;
	bool getBool(const char *key,const char *defaultValueIfNotFound=NULL) const;
	float getFloat(const char *key,const char *defaultValueIfNotFound=NULL) const;
	const string getString(const char *key,const char *defaultValueIfNotFound=NULL) const;
	char getCharKey(const char *key) const;

	void setInt(const string &key, int value);
	void setBool(const string &key, bool value);
	void setFloat(const string &key, float value);
	void setString(const string &key, const string &value);

    vector<string> getPathListForType(PathType type, string scenarioDir = "");

    vector<pair<string,string> > getMergedProperties() const;
    vector<pair<string,string> > getMasterProperties() const;
    vector<pair<string,string> > getUserProperties() const;
    void setUserProperties(const vector<pair<string,string> > &valueList);

    string getFileName(bool userFilename) const;

    char translateStringToCharKey(const string &value) const;
    SDLKey translateSpecialStringToSDLKey(char c) const;

	string toString();
};

}}//end namespace

#endif
