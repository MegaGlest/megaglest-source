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

#ifndef _GLEST_GAME_LANG_H_
#define _GLEST_GAME_LANG_H_

#include "properties.h"

namespace Glest{ namespace Game{

using Shared::Util::Properties;

// =====================================================
// 	class Lang
//
//	String table
// =====================================================

class Lang{
private:
	string language;
	Properties strings;
	Properties scenarioStrings;

private:
	Lang(){};

public:
	static Lang &getInstance();    
	void loadStrings(const string &language);
	void loadScenarioStrings(const string &scenarioDir, const string &scenarioName);
	string get(const string &s);
	string getScenarioString(const string &s);
};

}}//end namespace

#endif
