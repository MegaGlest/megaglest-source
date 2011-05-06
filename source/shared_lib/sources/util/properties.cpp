// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2007 Martio Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "properties.h"

#include <fstream>
#include <stdexcept>
#include <cstring>

#include "conversion.h"
#include "util.h"
#include "platform_common.h"

#ifdef WIN32
#include <shlwapi.h>
#include <shlobj.h>
#endif

#include "leak_dumper.h"

using namespace std;
using namespace Shared::PlatformCommon;

namespace Shared{ namespace Util{

string Properties::applicationPath = "";

// =====================================================
//	class Properties
// =====================================================

void Properties::load(const string &path, bool clearCurrentProperties) {

	ifstream fileStream;
	char lineBuffer[maxLine]="";
	string line, key, value;
	size_t pos=0;

	this->path= path;

	fileStream.open(path.c_str(), ios_base::in);
	if(fileStream.fail()){
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] path = [%s]\n",__FILE__,__FUNCTION__,__LINE__,path.c_str());
		throw runtime_error("Can't open propertyMap file: " + path);
	}

	if(clearCurrentProperties == true) {
		propertyMap.clear();
	}
	while(!fileStream.eof()){
		fileStream.getline(lineBuffer, maxLine);
		lineBuffer[maxLine-1]='\0';

		//process line if it it not a comment
		if(lineBuffer[0]!=';'){

			// gracefully handle win32 \r\n line endings
			size_t len= strlen(lineBuffer);
   			if(len > 0 && lineBuffer[len-1] == '\r'){
				lineBuffer[len-1]= 0;
			}

			line= lineBuffer;
			pos= line.find('=');

			if(pos != string::npos){
				key= line.substr(0, pos);
				value= line.substr(pos+1);

				if(applyTagsToValue(value) == true) {
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Property key [%s] now has value [%s]\n",key.c_str(),value.c_str());
				}

				bool replaceExisting = false;
				if(propertyMap.find(key) != propertyMap.end()) {
					replaceExisting = true;
				}
				propertyMap[key] = value;

				if(replaceExisting == false) {
					propertyVector.push_back(PropertyPair(key, value));
				}
				else {
					for(unsigned int i = 0; i < propertyVector.size(); ++i) {
						PropertyPair &currentPair = propertyVector[i];
						if(currentPair.first == key) {
							currentPair.second = value;
							break;
						}
					}
				}
			}
		}
	}

	fileStream.close();
}

std::map<string,string> Properties::getTagReplacementValues(std::map<string,string> *mapExtraTagReplacementValues) {
	std::map<string,string> mapTagReplacementValues;

	//
	// #1
	// First add the standard tags
	//
	char *homeDir = NULL;
#ifdef WIN32
	homeDir = getenv("USERPROFILE");
#else
	homeDir = getenv("HOME");
#endif

	mapTagReplacementValues["~/"] = (homeDir != NULL ? homeDir : "");
	mapTagReplacementValues["$HOME"] = (homeDir != NULL ? homeDir : "");
	mapTagReplacementValues["%%HOME%%"] = (homeDir != NULL ? homeDir : "");
	mapTagReplacementValues["%%USERPROFILE%%"] = (homeDir != NULL ? homeDir : "");
	mapTagReplacementValues["%%HOMEPATH%%"] = (homeDir != NULL ? homeDir : "");

	// For win32 we allow use of the appdata variable since that is the recommended
	// place for application data in windows platform
#ifdef WIN32
	TCHAR szPath[MAX_PATH]="";
   // Get path for each computer, non-user specific and non-roaming data.
   if ( SUCCEEDED( SHGetFolderPath( NULL, CSIDL_APPDATA,
                                    NULL, 0, szPath))) {
	   string appPath = szPath;
	   mapTagReplacementValues["$APPDATA"] = appPath;
	   mapTagReplacementValues["%%APPDATA%%"] = appPath;
   }
#endif

	char *username = NULL;
	username = getenv("USERNAME");

	mapTagReplacementValues["$USERNAME"] = (username != NULL ? username : "");
	mapTagReplacementValues["%%USERNAME%%"] = (username != NULL ? username : "");

	mapTagReplacementValues["$APPLICATIONPATH"] = Properties::applicationPath;
	mapTagReplacementValues["%%APPLICATIONPATH%%"] = Properties::applicationPath;

#if defined(CUSTOM_DATA_INSTALL_PATH)
	mapTagReplacementValues["$APPLICATIONDATAPATH"] = CUSTOM_DATA_INSTALL_PATH;
	mapTagReplacementValues["%%APPLICATIONDATAPATH%%"] = CUSTOM_DATA_INSTALL_PATH;

	//mapTagReplacementValues["$COMMONDATAPATH", 	string(CUSTOM_DATA_INSTALL_PATH) + "/commondata/");
	//mapTagReplacementValues["%%COMMONDATAPATH%%",	string(CUSTOM_DATA_INSTALL_PATH) + "/commondata/");

#else
	mapTagReplacementValues["$APPLICATIONDATAPATH"] = Properties::applicationPath;
	mapTagReplacementValues["%%APPLICATIONDATAPATH%%"] = Properties::applicationPath;

	//mapTagReplacementValues["$COMMONDATAPATH", 	Properties::applicationPath + "/commondata/");
	//mapTagReplacementValues["%%COMMONDATAPATH%%",	Properties::applicationPath + "/commondata/");
#endif

	//
	// #2
	// Next add the extra tags if passed in
	//
	if(mapExtraTagReplacementValues != NULL) {
		for(std::map<string,string>::iterator iterMap = mapExtraTagReplacementValues->begin();
				iterMap != mapExtraTagReplacementValues->end(); ++iterMap) {
			mapTagReplacementValues[iterMap->first] = iterMap->second;
		}
	}

	return mapTagReplacementValues;
}

bool Properties::applyTagsToValue(string &value, std::map<string,string> *mapTagReplacementValues) {
	string originalValue = value;

	if(mapTagReplacementValues != NULL) {
		for(std::map<string,string>::iterator iterMap = mapTagReplacementValues->begin();
				iterMap != mapTagReplacementValues->end(); ++iterMap) {
			replaceAll(value, iterMap->first, iterMap->second);
		}
	}
	else {
	char *homeDir = NULL;
#ifdef WIN32
	homeDir = getenv("USERPROFILE");
#else
	homeDir = getenv("HOME");
#endif

	replaceAll(value, "~/", 			(homeDir != NULL ? homeDir : ""));
	replaceAll(value, "$HOME", 			(homeDir != NULL ? homeDir : ""));
	replaceAll(value, "%%HOME%%", 		(homeDir != NULL ? homeDir : ""));
	replaceAll(value, "%%USERPROFILE%%",(homeDir != NULL ? homeDir : ""));
	replaceAll(value, "%%HOMEPATH%%",	(homeDir != NULL ? homeDir : ""));

	// For win32 we allow use of the appdata variable since that is the recommended
	// place for application data in windows platform
#ifdef WIN32
	TCHAR szPath[MAX_PATH]="";
   // Get path for each computer, non-user specific and non-roaming data.
   if ( SUCCEEDED( SHGetFolderPath( NULL, CSIDL_APPDATA, 
                                    NULL, 0, szPath))) {
	   string appPath = szPath;
       replaceAll(value, "$APPDATA", appPath);
	   replaceAll(value, "%%APPDATA%%", appPath);
   }
#endif

	char *username = NULL;
	username = getenv("USERNAME");
	replaceAll(value, "$USERNAME", 		(username != NULL ? username : ""));
	replaceAll(value, "%%USERNAME%%", 	(username != NULL ? username : ""));

	replaceAll(value, "$APPLICATIONPATH", 		Properties::applicationPath);
	replaceAll(value, "%%APPLICATIONPATH%%",	Properties::applicationPath);

#if defined(CUSTOM_DATA_INSTALL_PATH)
	replaceAll(value, "$APPLICATIONDATAPATH", 		CUSTOM_DATA_INSTALL_PATH);
	replaceAll(value, "%%APPLICATIONDATAPATH%%",	CUSTOM_DATA_INSTALL_PATH);

	//replaceAll(value, "$COMMONDATAPATH", 	string(CUSTOM_DATA_INSTALL_PATH) + "/commondata/");
	//replaceAll(value, "%%COMMONDATAPATH%%",	string(CUSTOM_DATA_INSTALL_PATH) + "/commondata/");

#else
	replaceAll(value, "$APPLICATIONDATAPATH", 		Properties::applicationPath);
	replaceAll(value, "%%APPLICATIONDATAPATH%%",	Properties::applicationPath);

	//replaceAll(value, "$COMMONDATAPATH", 	Properties::applicationPath + "/commondata/");
	//replaceAll(value, "%%COMMONDATAPATH%%",	Properties::applicationPath + "/commondata/");
#endif

	}

	//printf("\nBEFORE SUBSTITUTE [%s] AFTER [%s]\n",originalValue.c_str(),value.c_str());
	return (originalValue != value);
}

void Properties::save(const string &path){
	ofstream fileStream;

	fileStream.open(path.c_str(), ios_base::out | ios_base::trunc);

	fileStream << "; === propertyMap File === \n";
	fileStream << '\n';

	for(PropertyMap::iterator pi= propertyMap.begin(); pi!=propertyMap.end(); ++pi){
		fileStream << pi->first << '=' << pi->second << '\n';
	}

	fileStream.close();
}

void Properties::clear(){
	propertyMap.clear();
	propertyVector.clear();
}

bool Properties::getBool(const string &key, const char *defaultValueIfNotFound) const{
	try{
		return strToBool(getString(key,defaultValueIfNotFound));
	}
	catch(exception &e){
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		throw runtime_error("Error accessing value: " + key + " in: " + path+"\n[" + e.what() + "]");
	}
}

int Properties::getInt(const string &key,const char *defaultValueIfNotFound) const{
	try{
		return strToInt(getString(key,defaultValueIfNotFound));
	}
	catch(exception &e){
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		throw runtime_error("Error accessing value: " + key + " in: " + path + "\n[" + e.what() + "]");
	}
}

int Properties::getInt(const string &key, int min, int max,const char *defaultValueIfNotFound) const{
	int i= getInt(key,defaultValueIfNotFound);
	if(i<min || i>max){
		throw runtime_error("Value out of range: " + key + ", min: " + intToStr(min) + ", max: " + intToStr(max));
	}
	return i;
}

float Properties::getFloat(const string &key, const char *defaultValueIfNotFound) const{
	try{
		return strToFloat(getString(key,defaultValueIfNotFound));
	}
	catch(exception &e){
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		throw runtime_error("Error accessing value: " + key + " in: " + path + "\n[" + e.what() + "]");
	}
}

float Properties::getFloat(const string &key, float min, float max, const char *defaultValueIfNotFound) const{
	float f= getFloat(key,defaultValueIfNotFound);
	if(f<min || f>max){
		throw runtime_error("Value out of range: " + key + ", min: " + floatToStr(min) + ", max: " + floatToStr(max));
	}
	return f;
}

const string Properties::getString(const string &key, const char *defaultValueIfNotFound) const{
	PropertyMap::const_iterator it;
	it= propertyMap.find(key);
	if(it==propertyMap.end()){
	    if(defaultValueIfNotFound != NULL) {
	        //printf("In [%s::%s - %d]defaultValueIfNotFound = [%s]\n",__FILE__,__FUNCTION__,__LINE__,defaultValueIfNotFound);
	        return string(defaultValueIfNotFound);
	    }
	    else {
            throw runtime_error("Value not found in propertyMap: " + key + ", loaded from: " + path);
	    }
	}
	else{
		return (it->second != "" ? it->second : (defaultValueIfNotFound != NULL ? defaultValueIfNotFound : it->second));
	}
}

void Properties::setInt(const string &key, int value){
	setString(key, intToStr(value));
}

void Properties::setBool(const string &key, bool value){
	setString(key, boolToStr(value));
}

void Properties::setFloat(const string &key, float value){
	setString(key, floatToStr(value));
}

void Properties::setString(const string &key, const string &value){
	propertyMap.erase(key);
	propertyMap.insert(PropertyPair(key, value));
}

string Properties::toString(){
	string rStr;

	for(PropertyMap::iterator pi= propertyMap.begin(); pi!=propertyMap.end(); pi++)
		rStr+= pi->first + "=" + pi->second + "\n";

	return rStr;
}




bool Properties::getBool(const char *key, const char *defaultValueIfNotFound) const{
	try{
		return strToBool(getString(key,defaultValueIfNotFound));
	}
	catch(exception &e){
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		throw runtime_error("Error accessing value: " + string(key) + " in: " + path+"\n[" + e.what() + "]");
	}
}

int Properties::getInt(const char *key,const char *defaultValueIfNotFound) const{
	try{
		return strToInt(getString(key,defaultValueIfNotFound));
	}
	catch(exception &e){
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		throw runtime_error("Error accessing value: " + string(key) + " in: " + path + "\n[" + e.what() + "]");
	}
}

float Properties::getFloat(const char *key, const char *defaultValueIfNotFound) const{
	try{
		return strToFloat(getString(key,defaultValueIfNotFound));
	}
	catch(exception &e){
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		throw runtime_error("Error accessing value: " + string(key) + " in: " + path + "\n[" + e.what() + "]");
	}
}

const string Properties::getString(const char *key, const char *defaultValueIfNotFound) const{
	PropertyMap::const_iterator it;
	it= propertyMap.find(key);
	if(it==propertyMap.end()){
	    if(defaultValueIfNotFound != NULL) {
	        //printf("In [%s::%s - %d]defaultValueIfNotFound = [%s]\n",__FILE__,__FUNCTION__,__LINE__,defaultValueIfNotFound);
	        return string(defaultValueIfNotFound);
	    }
	    else {
            throw runtime_error("Value not found in propertyMap: " + string(key) + ", loaded from: " + path);
	    }
	}
	else{
		return (it->second != "" ? it->second : (defaultValueIfNotFound != NULL ? defaultValueIfNotFound : it->second));
	}
}


}}//end namepsace
