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

	int getInt(const string &key) const;
	bool getBool(const string &key) const;
	float getFloat(const string &key) const;
	const string &getString(const string &key) const;

	void setInt(const string &key, int value);
	void setBool(const string &key, bool value);
	void setFloat(const string &key, float value);
	void setString(const string &key, const string &value);

	string toString();
};

}}//end namespace

#endif
