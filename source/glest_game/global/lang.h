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

#ifndef _GLEST_GAME_LANG_H_
#define _GLEST_GAME_LANG_H_

#include "properties.h"
#include "leak_dumper.h"

namespace Glest{ namespace Game{

using Shared::Util::Properties;

// =====================================================
// 	class Lang
//
//	String table
// =====================================================

class Lang {
private:
	string language;
	bool is_utf8_language;

	Properties strings;
	Properties scenarioStrings;

	std::map<string,Properties> otherLanguageStrings;

private:
	Lang();
	void loadStrings(string language, Properties &properties, bool fileMustExist,bool fallbackToDefault=false);
	string getLanguageFile(string uselanguage);
	bool fileMatchesISO630Code(string uselanguage, string testLanguageFile);
	string getNativeLanguageName(string uselanguage, string testLanguageFile);

	string parseResult(const string &key, const string &value);

public:
	static Lang &getInstance();    

	void loadStrings(string uselanguage, bool loadFonts=true, bool fallbackToDefault=false);
	void loadScenarioStrings(string scenarioDir, string scenarioName);

	string get(const string &s,string uselanguage="", bool fallbackToDefault=false);
	bool hasString(const string &s, string uselanguage="", bool fallbackToDefault=false);
	string getScenarioString(const string &s);

	string getLanguage() const { return language; }
	bool isLanguageLocal(string compareLanguage) const;
	bool isUTF8Language() const;
	string getDefaultLanguage() const;

	map<string,string> getDiscoveredLanguageList(bool searchKeyIsLangName=false);
	pair<string,string> getNavtiveNameFromLanguageName(string langName);
};

}}//end namespace

#endif
