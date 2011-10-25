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
#include "platform_common.h"
#include "conversion.h"
#include "gl_wrap.h"
#include "core_data.h"
#include "renderer.h"
#include "leak_dumper.h"

using namespace std;
using namespace Shared::Util;
using namespace Shared::Platform;

namespace Glest{ namespace Game{

// =====================================================
// 	class Lang
// =====================================================

Lang::Lang() {
	language 			= "";
	is_utf8_language 	= false;
}

Lang &Lang::getInstance() {
	static Lang lang;
	return lang;
}

void Lang::loadStrings(const string &language) {
	bool languageChanged = (language != this->language);
	this->language= language;
	loadStrings(language, strings, true);

	if(languageChanged == true) {
		Font::resetToDefaults();
		Lang &lang = Lang::getInstance();
		if(	lang.hasString("FONT_BASE_SIZE")) {
			Font::baseSize    = strToInt(lang.get("FONT_BASE_SIZE"));
		}

		if(	lang.hasString("FONT_SCALE_SIZE")) {
			Font::scaleFontValue = strToFloat(lang.get("FONT_SCALE_SIZE"));
		}
		if(	lang.hasString("FONT_SCALE_CENTERH_FACTOR")) {
			Font::scaleFontValueCenterHFactor = strToFloat(lang.get("FONT_SCALE_CENTERH_FACTOR"));
		}

		if(	lang.hasString("FONT_CHARCOUNT")) {
			// 256 for English
			// 30000 for Chinese
			Font::charCount    = strToInt(lang.get("FONT_CHARCOUNT"));
		}
		if(	lang.hasString("FONT_TYPENAME")) {
			Font::fontTypeName = lang.get("FONT_TYPENAME");
		}
		if(	lang.hasString("FONT_CHARSET")) {
			// Example values:
			// DEFAULT_CHARSET (English) = 1
			// GB2312_CHARSET (Chinese)  = 134
			Shared::Platform::charSet = strToInt(lang.get("FONT_CHARSET"));
		}
		if(	lang.hasString("FONT_MULTIBYTE")) {
			Font::fontIsMultibyte 	= strToBool(lang.get("FONT_MULTIBYTE"));
		}

		if(	lang.hasString("FONT_RIGHTTOLEFT")) {
			Font::fontIsRightToLeft	= strToBool(lang.get("FONT_RIGHTTOLEFT"));
		}

		if(	lang.hasString("MEGAGLEST_FONT")) {
			//setenv("MEGAGLEST_FONT","/usr/share/fonts/truetype/ttf-japanese-gothic.ttf",0); // Japanese
	#if defined(WIN32)
			string newEnvValue = "MEGAGLEST_FONT=" + lang.get("MEGAGLEST_FONT");
			_putenv(newEnvValue.c_str());
	#else
			setenv("MEGAGLEST_FONT",lang.get("MEGAGLEST_FONT").c_str(),0);
	#endif
		}

	//        if(	lang.hasString("FONT_YOFFSET_FACTOR")) {
	//        	FontMetrics::DEFAULT_Y_OFFSET_FACTOR = strToFloat(lang.get("FONT_YOFFSET_FACTOR"));
	//		}

	#if defined(WIN32)
		// Win32 overrides for fonts (just in case they must be different)

		if(	lang.hasString("FONT_BASE_SIZE_WINDOWS")) {
			// 256 for English
			// 30000 for Chinese
			Font::baseSize    = strToInt(lang.get("FONT_BASE_SIZE_WINDOWS"));
		}

		if(	lang.hasString("FONT_SCALE_SIZE_WINDOWS")) {
			Font::scaleFontValue = strToFloat(lang.get("FONT_SCALE_SIZE_WINDOWS"));
		}
		if(	lang.hasString("FONT_SCALE_CENTERH_FACTOR_WINDOWS")) {
			Font::scaleFontValueCenterHFactor = strToFloat(lang.get("FONT_SCALE_CENTERH_FACTOR_WINDOWS"));
		}

		if(	lang.hasString("FONT_HEIGHT_TEXT_WINDOWS")) {
			Font::langHeightText = config.getString("FONT_HEIGHT_TEXT_WINDOWS",Font::langHeightText.c_str());
		}

		if(	lang.hasString("FONT_CHARCOUNT_WINDOWS")) {
			// 256 for English
			// 30000 for Chinese
			Font::charCount    = strToInt(lang.get("FONT_CHARCOUNT_WINDOWS"));
		}
		if(	lang.hasString("FONT_TYPENAME_WINDOWS")) {
			Font::fontTypeName = lang.get("FONT_TYPENAME_WINDOWS");
		}
		if(	lang.hasString("FONT_CHARSET_WINDOWS")) {
			// Example values:
			// DEFAULT_CHARSET (English) = 1
			// GB2312_CHARSET (Chinese)  = 134
			Shared::Platform::charSet = strToInt(lang.get("FONT_CHARSET_WINDOWS"));
		}
		if(	lang.hasString("FONT_MULTIBYTE_WINDOWS")) {
			Font::fontIsMultibyte 	= strToBool(lang.get("FONT_MULTIBYTE_WINDOWS"));
		}
		if(	lang.hasString("FONT_RIGHTTOLEFT_WINDOWS")) {
			Font::fontIsRightToLeft	= strToBool(lang.get("FONT_RIGHTTOLEFT_WINDOWS"));
		}

		if(	lang.hasString("MEGAGLEST_FONT_WINDOWS")) {
			//setenv("MEGAGLEST_FONT","/usr/share/fonts/truetype/ttf-japanese-gothic.ttf",0); // Japanese
			string newEnvValue = "MEGAGLEST_FONT=" + lang.get("MEGAGLEST_FONT_WINDOWS");
			_putenv(newEnvValue.c_str());
		}

	//        if(	lang.hasString("FONT_YOFFSET_FACTOR_WINDOWS")) {
	//        	FontMetrics::DEFAULT_Y_OFFSET_FACTOR = strToFloat(lang.get("FONT_YOFFSET_FACTOR_WINDOWS"));
	//		}

		// end win32
	#endif

		CoreData &coreData= CoreData::getInstance();
		coreData.loadFonts();
    }
}

void Lang::loadStrings(const string &language, Properties &properties, bool fileMustExist) {
	properties.clear();
	string data_path = getGameReadWritePath(GameConstants::path_data_CacheLookupKey);
	string languageFile = data_path + "data/lang/" + language + ".lng";
	if(fileMustExist == false && fileExists(languageFile) == false) {
		return;
	}
	is_utf8_language = valid_utf8_file(languageFile.c_str());
	properties.load(languageFile);
}

bool Lang::isUTF8Language() const {
	return is_utf8_language;
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

bool Lang::hasString(const string &s, string language) {
	bool hasString = false;
	try {
		if(language != "") {
			if(otherLanguageStrings.find(language) == otherLanguageStrings.end()) {
				loadStrings(language, otherLanguageStrings[language], false);
			}
			string result = otherLanguageStrings[language].getString(s);
			hasString = true;
		}
		else {
			string result = strings.getString(s);
			hasString = true;
		}
	}
	catch(exception &ex) {
		if(strings.getpath() != "") {
			if(SystemFlags::VERBOSE_MODE_ENABLED) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s] for language [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what(),language.c_str());
		}
	}
	return hasString;
}

bool Lang::isLanguageLocal(string compareLanguage) const {
	return (compareLanguage == language);
}

string Lang::get(const string &s, string language) {
	try {
		string result = "";
		if(language != "") {
			if(otherLanguageStrings.find(language) == otherLanguageStrings.end()) {
				loadStrings(language, otherLanguageStrings[language], false);
			}
			result = otherLanguageStrings[language].getString(s);
			replaceAll(result, "\\n", "\n");
		}
		else {
			result = strings.getString(s);
			replaceAll(result, "\\n", "\n");
		}
		return result;
	}
	catch(exception &ex) {
		if(strings.getpath() != "") {
			SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s] language [%s] text [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what(),language.c_str(),s.c_str());
		}
		return "???" + s + "???";
	}
}

string Lang::getScenarioString(const string &s) {
	try{
		string result = scenarioStrings.getString(s);
		replaceAll(result, "\\n", "\n");
		return result;
	}
	catch(exception &ex) {
		if(scenarioStrings.getpath() != "") {
			SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
		}
		return "???" + s + "???";
	}
}

}}//end namespace
