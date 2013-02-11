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

#ifndef _GLEST_GAME_CONFIG_H_
#define _GLEST_GAME_CONFIG_H_

#ifdef WIN32
    #include <winsock2.h>
    #include <winsock.h>
#endif

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
    cfgTempGame,
    cfgMainKeys,
    cfgUserKeys,
    cfgTempKeys
};

class Config {
private:

	std::pair<Properties,Properties> properties;
	Properties tempProperties;
	std::pair<ConfigType,ConfigType> cfgType;
	std::pair<string,string> fileNameParameter;
	std::pair<string,string> fileName;
	std::pair<bool,bool> fileLoaded;

	static map<ConfigType,Config> configList;

    static const char *glest_ini_filename;
    static const char *glestuser_ini_filename;

    static map<string,string> customRuntimeProperties;

public:

    static const char *glestkeys_ini_filename;
    static const char *glestuserkeys_ini_filename;

    static const char *ACTIVE_MOD_PROPERTY_NAME;

    static const char *colorPicking;
    static const char *selectBufPicking;
    static const char *frustumPicking;

protected:

	Config();
	Config(std::pair<ConfigType,ConfigType> type, std::pair<string,string> file, std::pair<bool,bool> fileMustExist);
	bool tryCustomPath(std::pair<ConfigType,ConfigType> &type, std::pair<string,string> &file, string custom_path);
	static void CopyAll(Config *src,Config *dest);
	vector<pair<string,string> > getPropertiesFromContainer(const Properties &propertiesObj) const;
	static bool replaceFileWithLocalFile(const vector<string> &dirList, string fileNamePart, string &resultToReplace);

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
	//char getCharKey(const char *key) const;
	SDLKey getSDLKey(const char *key) const;

	void setInt(const string &key, int value, bool tempBuffer=false);
	void setBool(const string &key, bool value, bool tempBuffer=false);
	void setFloat(const string &key, float value, bool tempBuffer=false);
	void setString(const string &key, const string &value, bool tempBuffer=false);

    vector<string> getPathListForType(PathType type, string scenarioDir = "");

    vector<pair<string,string> > getMergedProperties() const;
    vector<pair<string,string> > getMasterProperties() const;
    vector<pair<string,string> > getUserProperties() const;
    void setUserProperties(const vector<pair<string,string> > &valueList);

    string getFileName(bool userFilename) const;

    SDLKey translateStringToSDLKey(const string &value) const;

	string toString();

	static string getCustomRuntimeProperty(string key) 				{ return customRuntimeProperties[key]; }
	static void setCustomRuntimeProperty(string key, string value) 	{ customRuntimeProperties[key] = value; }

	static string findValidLocalFileFromPath(string fileName);
};

}}//end namespace

#endif
