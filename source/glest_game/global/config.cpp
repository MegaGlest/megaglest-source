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

const char *Config::glestkeys_ini_filename              = "glestkeys.ini";
const char *Config::glestuserkeys_ini_filename          = "glestuserkeys.ini";

// =====================================================
// 	class Config
// =====================================================

const string defaultNotFoundValue = "~~NOT FOUND~~";

map<ConfigType,Config> Config::configList;

Config::Config() {
	fileLoaded.first 			= false;
	fileLoaded.second 			= false;
	cfgType.first 				= cfgMainGame;
	cfgType.second 				= cfgUserGame;
	fileName.first 				= "";
	fileName.second 			= "";
	fileNameParameter.first 	= "";
	fileNameParameter.second 	= "";
	fileLoaded.first 			= false;
	fileLoaded.second 			= false;
}

bool Config::tryCustomPath(std::pair<ConfigType,ConfigType> &type, std::pair<string,string> &file, string custom_path) {
	bool wasFound = false;
    if((type.first == cfgMainGame && type.second == cfgUserGame &&
        file.first == glest_ini_filename && file.second == glestuser_ini_filename) ||
       (type.first == cfgMainKeys && type.second == cfgUserKeys &&
       	file.first == glestkeys_ini_filename && file.second == glestuserkeys_ini_filename)) {

    	string linuxPath = custom_path;
    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("-=-=-=-=-=-=-= looking for file in possible location  [%s]\n",linuxPath.c_str());
    	if(fileExists(linuxPath + file.first) == true) {
        	file.first = linuxPath + file.first;
        	file.second = linuxPath + file.second;
        	wasFound = true;
    	}
    }
	return wasFound;
}

Config::Config(std::pair<ConfigType,ConfigType> type, std::pair<string,string> file, std::pair<bool,bool> fileMustExist) {
	fileLoaded.first 			= false;
	fileLoaded.second 			= false;
	cfgType 					= type;
	fileName 					= file;
	fileNameParameter 			= file;

    if(getGameReadWritePath(GameConstants::path_ini_CacheLookupKey) != "") {
    	fileName.first = getGameReadWritePath(GameConstants::path_ini_CacheLookupKey) + fileName.first;
    	fileName.second = getGameReadWritePath(GameConstants::path_ini_CacheLookupKey) + fileName.second;
    }

    string currentpath = extractDirectoryPathFromFile(Properties::getApplicationPath());
  	bool foundPath = tryCustomPath(cfgType, fileName, currentpath);

#if defined(CUSTOM_DATA_INSTALL_PATH)
  	if(foundPath == false) {
  		foundPath = tryCustomPath(cfgType, fileName, CUSTOM_DATA_INSTALL_PATH);
  	}
#endif

    // Look in standard linux shared paths for ini files
#if defined(__linux__)
    if(foundPath == false) {
    	foundPath = tryCustomPath(cfgType, fileName, "/usr/share/megaglest/");
    }
    if(foundPath == false) {
    	foundPath = tryCustomPath(cfgType, fileName, "/usr/share/games/megaglest/");
    }
    if(foundPath == false) {
    	foundPath = tryCustomPath(cfgType, fileName, "/usr/local/share/megaglest/");
    }
    if(foundPath == false) {
    	foundPath = tryCustomPath(cfgType, fileName, "/usr/local/share/games/megaglest/");
    }
#endif

    if(fileMustExist.first == true && fileExists(fileName.first) == false) {
    	//string currentpath = extractDirectoryPathFromFile(Properties::getApplicationPath());
    	fileName.first = currentpath + fileName.first;
    }
    if(SystemFlags::VERBOSE_MODE_ENABLED) printf("-=-=-=-=-=-=-= About to load fileName.first = [%s]\n",fileName.first.c_str());

    if(fileMustExist.first == true ||
    	(fileMustExist.first == false && fileExists(fileName.first) == true)) {
    	properties.first.load(fileName.first);
    	fileLoaded.first = true;
    }

    if(cfgType.first == cfgMainGame) {
        if( properties.first.getString("UserData_Root", defaultNotFoundValue.c_str()) != defaultNotFoundValue) {
        	string userData = properties.first.getString("UserData_Root");
        	if(userData != "") {
        		endPathWithSlash(userData);
        	}
        	fileName.second = userData + fileNameParameter.second;
        }
        else if(properties.first.getString("UserOverrideFile", defaultNotFoundValue.c_str()) != defaultNotFoundValue) {
        	string userData = properties.first.getString("UserOverrideFile");
        	if(userData != "") {
        		endPathWithSlash(userData);
        	}
        	fileName.second = userData + fileNameParameter.second;
        }
    }
    else if(cfgType.first == cfgMainKeys) {
        Config &mainCfg = Config::getInstance();
    	if( mainCfg.getString("UserData_Root", defaultNotFoundValue.c_str()) != defaultNotFoundValue) {
        	string userData = mainCfg.getString("UserData_Root");
        	if(userData != "") {
        		endPathWithSlash(userData);
        	}
        	fileName.second = userData + fileNameParameter.second;
        }
        else if(mainCfg.getString("UserOverrideFile", defaultNotFoundValue.c_str()) != defaultNotFoundValue) {
        	string userData = mainCfg.getString("UserOverrideFile");
        	if(userData != "") {
        		endPathWithSlash(userData);
        	}
        	fileName.second = userData + fileNameParameter.second;
        }
    }

    if(SystemFlags::VERBOSE_MODE_ENABLED) printf("-=-=-=-=-=-=-= About to load fileName.second = [%s]\n",fileName.second.c_str());

    if(fileMustExist.second == true ||
    	(fileMustExist.second == false && fileExists(fileName.second) == true)) {
    	properties.second.load(fileName.second);
    	fileLoaded.second = true;
    }

    try {
		if(fileName.second != "" && fileExists(fileName.second) == false) {
			if(SystemFlags::VERBOSE_MODE_ENABLED) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] attempting to auto-create cfgFile.second = [%s]\n",__FILE__,__FUNCTION__,__LINE__,fileName.second.c_str());

#if defined(WIN32) && !defined(__MINGW32__)
			wstring wstr = utf8_decode(fileName.second);
			FILE *fp = _wfopen(wstr.c_str(), L"w");
			std::ofstream userFile(fp);
#else
			std::ofstream userFile;
			userFile.open(fileName.second.c_str(), ios_base::out | ios_base::trunc);
#endif
			userFile.close();
#if defined(WIN32) && !defined(__MINGW32__)
			if(fp) {
				fclose(fp);
			}
#endif
			fileLoaded.second = true;
			properties.second.load(fileName.second);
		}
    }
    catch(const exception &ex) {
    	SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
    	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] ERROR trying to auto-create cfgFile.second = [%s]\n",__FILE__,__FUNCTION__,__LINE__,fileName.second.c_str());
    }
}

Config &Config::getInstance(std::pair<ConfigType,ConfigType> type, std::pair<string,string> file, std::pair<bool,bool> fileMustExist) {
	if(configList.find(type.first) == configList.end()) {
		if(SystemFlags::VERBOSE_MODE_ENABLED) if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		Config config(type, file, fileMustExist);

		if(SystemFlags::VERBOSE_MODE_ENABLED) if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		configList.insert(map<ConfigType,Config>::value_type(type.first,config));

		if(SystemFlags::VERBOSE_MODE_ENABLED) if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}

	return configList.find(type.first)->second;
}

void Config::CopyAll(Config *src, Config *dest) {

	dest->properties	= src->properties;
	dest->cfgType		= src->cfgType;
	dest->fileName		= src->fileName;
	dest->fileNameParameter = src->fileNameParameter;
	dest->fileLoaded	= src->fileLoaded;
}

void Config::reload() {
	if(SystemFlags::VERBOSE_MODE_ENABLED) if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	std::pair<ConfigType,ConfigType> type = std::make_pair(cfgType.first,cfgType.second);
	Config newconfig(type, std::make_pair(fileNameParameter.first,fileNameParameter.second), std::make_pair(true,false));

	if(SystemFlags::VERBOSE_MODE_ENABLED) if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	Config &oldconfig = configList.find(type.first)->second;
	CopyAll(&newconfig, &oldconfig);

	if(SystemFlags::VERBOSE_MODE_ENABLED) if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void Config::save(const string &path){
	if(fileLoaded.second == true) {
		if(path != "") {
			fileName.second = path;
		}
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] save file [%s]\n",__FILE__,__FUNCTION__,__LINE__,fileName.second.c_str());
		properties.second.save(fileName.second);
		return;
	}

	if(path != "") {
		fileName.first = path;
	}
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] save file [%s]\n",__FILE__,__FUNCTION__,__LINE__,fileName.first.c_str());
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

/*
SDLKey Config::translateSpecialStringToSDLKey(char c) const {
	SDLKey result = SDLK_UNKNOWN;
	if(c < 0) {
		switch(c) {
			case vkAdd:
				result = SDLK_PLUS;
				break;
			case vkSubtract:
				result = SDLK_MINUS;
				break;

			case vkAlt:
				result = SDLK_RALT;
				break;

			case vkControl:
				result = SDLK_RCTRL;
				break;

			case vkShift:
				result = SDLK_RSHIFT;
				break;

			case vkEscape:
				result = SDLK_ESCAPE;
				break;

			case vkUp:
				result = SDLK_UP;
				break;

			case vkLeft:
				result = SDLK_LEFT;
				break;

			case vkRight:
				result = SDLK_RIGHT;
				break;

			case vkDown:
				result = SDLK_DOWN;
				break;

			case vkReturn:
				result = SDLK_RETURN;
				break;

			case vkBack:
				result = SDLK_BACKSPACE;
				break;

			case vkTab:
				result = SDLK_TAB;
				break;

			case vkF1:
				result = SDLK_F1;
				break;

			case vkF2:
				result = SDLK_F2;
				break;

			case vkF3:
				result = SDLK_F3;
				break;

			case vkF4:
				result = SDLK_F4;
				break;

			case vkF5:
				result = SDLK_F5;
				break;

			case vkF6:
				result = SDLK_F6;
				break;

			case vkF7:
				result = SDLK_F7;
				break;

			case vkF8:
				result = SDLK_F8;
				break;

			case vkF9:
				result = SDLK_F9;
				break;

			case vkF10:
				result = SDLK_F10;
				break;

			case vkF11:
				result = SDLK_F11;
				break;

			case vkF12:
				result = SDLK_F12;
				break;

			case vkDelete:
				result = SDLK_DELETE;
				break;

			case vkPrint:
				result = SDLK_PRINT;
				break;

			case vkPause:
				result = SDLK_PAUSE;
				break;
		}
	}
	else {
		result = static_cast<SDLKey>(c);
	}

	return result;
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
			bool foundKey = false;
			if(value.length() > 1) {
				for(int i = SDLK_UNKNOWN; i < SDLK_LAST; ++i) {
					SDLKey key = static_cast<SDLKey>(i);
					string keyName = SDL_GetKeyName(key);
					if(value == keyName) {
						if(key > 255) {
							if(value == "left") {
								result = vkLeft;
							}
							else if(value == "right") {
								result = vkRight;
							}
							else if(value == "up") {
								result = vkUp;
							}
							else if(value == "down") {
								result = vkDown;
							}
							else if(value == "add") {
								result = vkAdd;
							}
							else if(value == "subtract") {
								result = vkSubtract;
							}
							else if(value == "escape") {
								result = vkEscape;
							}
							else if(value == "f1") {
								result = vkF1;
							}
							else if(value == "f2") {
								result = vkF2;
							}
							else if(value == "f3") {
								result = vkF3;
							}
							else if(value == "f4") {
								result = vkF4;
							}
							else if(value == "f5") {
								result = vkF5;
							}
							else if(value == "f6") {
								result = vkF6;
							}
							else if(value == "f7") {
								result = vkF7;
							}
							else if(value == "f8") {
								result = vkF8;
							}
							else if(value == "f9") {
								result = vkF9;
							}
							else if(value == "f10") {
								result = vkF10;
							}
							else if(value == "f11") {
								result = vkF11;
							}
							else if(value == "f12") {
								result = vkF12;
							}
							else if(value == "print-screen") {
								result = vkPrint;
							}
							else if(value == "pause") {
								result = vkPause;
							}
							else {
								result = -key;
							}
						}
						else {
							result = key;
						}
						foundKey = true;
						break;
					}
				}
			}

			if(foundKey == false) {
				result = value[0];
			}
		}
	}
	else {
		string sError = "Unsupported key translation" + value;
		throw runtime_error(sError.c_str());
	}

	// Because SDL is based on lower Ascii
	result = tolower(result);
	return result;
}
*/

SDLKey Config::translateStringToSDLKey(const string &value) const {
	SDLKey result = SDLK_UNKNOWN;

	if(IsNumeric(value.c_str()) == true) {
		result = (SDLKey)strToInt(value);
	}
	else if(value.substr(0,2) == "vk") {
		if(value == "vkLeft") {
			result = SDLK_LEFT;
		}
		else if(value == "vkRight") {
			result = SDLK_RIGHT;
		}
		else if(value == "vkUp") {
			result = SDLK_UP;
		}
		else if(value == "vkDown") {
			result = SDLK_DOWN;
		}
		else if(value == "vkAdd") {
			result = SDLK_PLUS;
		}
		else if(value == "vkSubtract") {
			result = SDLK_MINUS;
		}
		else if(value == "vkEscape") {
			result = SDLK_ESCAPE;
		}
		else if(value == "vkF1") {
			result = SDLK_F1;
		}
		else if(value == "vkF2") {
			result = SDLK_F2;
		}
		else if(value == "vkF3") {
			result = SDLK_F3;
		}
		else if(value == "vkF4") {
			result = SDLK_F4;
		}
		else if(value == "vkF5") {
			result = SDLK_F5;
		}
		else if(value == "vkF6") {
			result = SDLK_F6;
		}
		else if(value == "vkF7") {
			result = SDLK_F7;
		}
		else if(value == "vkF8") {
			result = SDLK_F8;
		}
		else if(value == "vkF9") {
			result = SDLK_F9;
		}
		else if(value == "vkF10") {
			result = SDLK_F10;
		}
		else if(value == "vkF11") {
			result = SDLK_F11;
		}
		else if(value == "vkF12") {
			result = SDLK_F12;
		}
		else if(value == "vkPrint") {
			result = SDLK_PRINT;
		}
		else if(value == "vkPause") {
			result = SDLK_PAUSE;
		}
		else {
			string sError = "Unsupported key translation [" + value + "]";
			throw runtime_error(sError.c_str());
		}
	}
	else if(value.length() >= 1) {
		if(value.length() == 3 && value[0] == '\'' && value[2] == '\'') {
			result = (SDLKey)value[1];
		}
		else {
			bool foundKey = false;
			if(value.length() > 1) {
				for(int i = SDLK_UNKNOWN; i < SDLK_LAST; ++i) {
					SDLKey key = static_cast<SDLKey>(i);
					string keyName = SDL_GetKeyName(key);
					if(value == keyName) {
						result = key;
						foundKey = true;
						break;
					}
				}
			}

			if(foundKey == false) {
				result = (SDLKey)value[0];
			}
		}
	}
	else {
		string sError = "Unsupported key translation" + value;
		throw runtime_error(sError.c_str());
	}

	// Because SDL is based on lower Ascii
	//result = tolower(result);
	return result;
}

SDLKey Config::getSDLKey(const char *key) const {
	if(fileLoaded.second == true &&
		properties.second.getString(key, defaultNotFoundValue.c_str()) != defaultNotFoundValue) {

		string value = properties.second.getString(key);
		return translateStringToSDLKey(value);
	}
	string value = properties.first.getString(key);
	return translateStringToSDLKey(value);
}

//char Config::getCharKey(const char *key) const {
//	if(fileLoaded.second == true &&
//		properties.second.getString(key, defaultNotFoundValue.c_str()) != defaultNotFoundValue) {
//
//		string value = properties.second.getString(key);
//		return translateStringToCharKey(value);
//	}
//	string value = properties.first.getString(key);
//	return translateStringToCharKey(value);
//}

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
    for(unsigned int i = 0; i < resultUser.size(); ++i) {
        const pair<string,string> &propertyUser = resultUser[i];
        bool overrideProperty = false;
        for(unsigned int j = 0; j < result.size(); ++j) {
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

void Config::setUserProperties(const vector<pair<string,string> > &valueList) {
	Properties &propertiesObj = properties.second;

	for(unsigned int idx = 0; idx < valueList.size(); ++ idx) {
		const pair<string,string> &nameValuePair = valueList[idx];
		propertiesObj.setString(nameValuePair.first,nameValuePair.second);
	}
}

string Config::getFileName(bool userFilename) const {
    string result = fileName.second;
	if(userFilename == false) {
		result = fileName.first;
    }

	return result;
}

string Config::toString(){
	return properties.first.toString();
}

vector<string> Config::getPathListForType(PathType type, string scenarioDir) {
    vector<string> pathList;
    string data_path = getGameReadWritePath(GameConstants::path_data_CacheLookupKey);

    string userData = getString("UserData_Root","");
    if(userData != "") {
    	endPathWithSlash(userData);
        //if(data_path == "") {
        //	userData = userData;
        //}
        //else {
        //	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("extractLastDirectoryFromPath(userData) [%s] from userData [%s]\n",extractLastDirectoryFromPath(userData).c_str(),userData.c_str());
        //	userData = data_path + extractLastDirectoryFromPath(userData);
        //}
        //if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] userData path [%s]\n",__FILE__,__FUNCTION__,__LINE__,userData.c_str());

        if(isdir(userData.c_str()) == false) {
        	createDirectoryPaths(userData);
        	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] creating path [%s]\n",__FILE__,__FUNCTION__,__LINE__,userData.c_str());
        }

        string userDataMaps = userData + GameConstants::folder_path_maps;
        if(isdir(userDataMaps.c_str()) == false) {
        	createDirectoryPaths(userDataMaps);
        	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] creating path [%s]\n",__FILE__,__FUNCTION__,__LINE__,userDataMaps.c_str());
        }
        string userDataScenarios = userData + GameConstants::folder_path_scenarios;
        if(isdir(userDataScenarios.c_str()) == false) {
        	createDirectoryPaths(userDataScenarios);
        	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] creating path [%s]\n",__FILE__,__FUNCTION__,__LINE__,userDataScenarios.c_str());
        }
        string userDataTechs = userData + GameConstants::folder_path_techs;
        if(isdir(userDataTechs.c_str()) == false) {
        	createDirectoryPaths(userDataTechs);
        	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] creating path [%s]\n",__FILE__,__FUNCTION__,__LINE__,userDataTechs.c_str());
        }
        string userDataTilesets = userData + GameConstants::folder_path_tilesets;
        if(isdir(userDataTilesets.c_str()) == false) {
        	createDirectoryPaths(userDataTilesets);
        	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] creating path [%s]\n",__FILE__,__FUNCTION__,__LINE__,userDataTilesets.c_str());
        }
        string userDataTutorials = userData + GameConstants::folder_path_tutorials;
        if(isdir(userDataTutorials.c_str()) == false) {
        	createDirectoryPaths(userDataTutorials);
        	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] creating path [%s]\n",__FILE__,__FUNCTION__,__LINE__,userDataTutorials.c_str());
        }
    }
    if(scenarioDir != "") {
    	//string scenarioLocation = data_path + scenarioDir;
    	string scenarioLocation = scenarioDir;
    	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Scenario path [%s]\n",scenarioLocation.c_str());
        pathList.push_back(scenarioLocation);
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
