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

#include "properties.h"
#include <vector>
#include "game_constants.h"

namespace Glest{ namespace Game{

using Shared::Util::Properties;

// =====================================================
// 	class Config
//
//	Game configuration
// =====================================================

class Config{
private:
	Properties properties;

private:
	Config();

public:
    static Config &getInstance();
	void save(const string &path="glest.ini");

	int getInt(const string &key,const char *defaultValueIfNotFound=NULL) const;
	bool getBool(const string &key,const char *defaultValueIfNotFound=NULL) const;
	float getFloat(const string &key,const char *defaultValueIfNotFound=NULL) const;
	const string getString(const string &key,const char *defaultValueIfNotFound=NULL) const;

	int getInt(const char *key,const char *defaultValueIfNotFound=NULL) const;
	bool getBool(const char *key,const char *defaultValueIfNotFound=NULL) const;
	float getFloat(const char *key,const char *defaultValueIfNotFound=NULL) const;
	const string getString(const char *key,const char *defaultValueIfNotFound=NULL) const;

	void setInt(const string &key, int value);
	void setBool(const string &key, bool value);
	void setFloat(const string &key, float value);
	void setString(const string &key, const string &value);

    vector<string> getPathListForType(PathType type, string scenarioDir = "");

	string toString();
};

}}//end namespace

#endif
