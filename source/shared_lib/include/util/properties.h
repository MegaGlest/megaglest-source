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

class Properties{
private:
	static const int maxLine= 1024;

public:	
	typedef pair<string, string> PropertyPair;
	typedef map<string, string> PropertyMap;
	typedef vector<PropertyPair> PropertyVector;
	
private:
	PropertyVector propertyVector;
	PropertyMap propertyMap;
	string path;

public:
	void clear();
	void load(const string &path);
	void save(const string &path);

	int getPropertyCount()	{return propertyVector.size();}
	string getKey(int i)	{return propertyVector[i].first;}
	string getString(int i)	{return propertyVector[i].second;}

	bool getBool(const string &key) const;
	int getInt(const string &key) const;
	int getInt(const string &key, int min, int max) const;
	float getFloat(const string &key) const;
	float getFloat(const string &key, float min, float max) const;
	const string &getString(const string &key) const;

	void setInt(const string &key, int value);
	void setBool(const string &key, bool value);
	void setFloat(const string &key, float value);
	void setString(const string &key, const string &value);

	string toString();


};

}}//end namespace

#endif
