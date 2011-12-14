// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_UTIL_PROPERTIES_H_
#define _SHARED_UTIL_PROPERTIES_H_

#include <string>
#include <map>
#include <vector>
#include "leak_dumper.h"

using std::map;
using std::vector;
using std::string;
using std::pair;

namespace Shared{ namespace Util{

// =====================================================
//	class Properties
//
///	ini-like file loader
// =====================================================

class Properties {
private:
	static const int maxLine= 4096;

public:
	typedef pair<string, string> PropertyPair;
	typedef map<string, string> PropertyMap;
	typedef vector<PropertyPair> PropertyVector;

private:
	PropertyVector propertyVector;
	PropertyMap propertyMap;
	string path;
	static string applicationPath;
	static string gameVersion;

public:
	static void setApplicationPath(string value) { applicationPath=value; }
	static string getApplicationPath() { return applicationPath; }

	static void setGameVersion(string value) { gameVersion=value; }
	static string getGameVersion() { return gameVersion; }

	void clear();
	void load(const string &path,bool clearCurrentProperties=true);
	void save(const string &path);

	int getPropertyCount() const	{return (int)propertyVector.size();}
	string getKey(int i) const	    {return propertyVector[i].first;}
	string getString(int i) const	{return propertyVector[i].second;}

	bool getBool(const string &key, const char *defaultValueIfNotFound=NULL) const;
	int getInt(const string &key, const char *defaultValueIfNotFound=NULL) const;
	int getInt(const string &key, int min, int max, const char *defaultValueIfNotFound=NULL) const;
	float getFloat(const string &key, const char *defaultValueIfNotFound=NULL) const;
	float getFloat(const string &key, float min, float max, const char *defaultValueIfNotFound=NULL) const;

	const string getString(const string &key, const char *defaultValueIfNotFound=NULL) const;

	int getInt(const char *key,const char *defaultValueIfNotFound=NULL) const;
	bool getBool(const char *key,const char *defaultValueIfNotFound=NULL) const;
	float getFloat(const char *key,const char *defaultValueIfNotFound=NULL) const;
	const string getString(const char *key,const char *defaultValueIfNotFound=NULL) const;

	void setInt(const string &key, int value);
	void setBool(const string &key, bool value);
	void setFloat(const string &key, float value);
	void setString(const string &key, const string &value);

	static bool applyTagsToValue(string &value, std::map<string,string> *mapTagReplacementValues=NULL);
	static std::map<string,string> getTagReplacementValues(std::map<string,string> *mapExtraTagReplacementValues=NULL);

	string getpath() const { return path;}

	string toString();
};

}}//end namespace

#endif
