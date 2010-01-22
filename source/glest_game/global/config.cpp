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

namespace Glest{ namespace Game{

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

int Config::getInt(const string &key) const{
	return properties.getInt(key);
}

bool Config::getBool(const string &key) const{
	return properties.getBool(key);
}

float Config::getFloat(const string &key) const{
	return properties.getFloat(key);
}

const string &Config::getString(const string &key) const{
	return properties.getString(key);
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

}}// end namespace
