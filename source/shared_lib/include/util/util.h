// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martio Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_UTIL_UTIL_H_
#define _SHARED_UTIL_UTIL_H_

#include <string>
#include <fstream>
#include <map>

using std::string;

namespace Shared{ namespace Util{

class SystemFlags
{
public:

    enum DebugType {
        debugSystem,
        debugNetwork,
		debugPerformance,
		debugWorldSynch
    };

	class SystemFlagsType
	{
	protected:
    	DebugType debugType;

	public:
    	SystemFlagsType() {
    		this->debugType 		= debugSystem;
    		this->enabled			= false;
    		this->fileStream 		= NULL;
    		this->debugLogFileName	= "";
			this->fileStreamOwner	= false;
    	}
    	SystemFlagsType(DebugType debugType) {
    		this->debugType 		= debugType;
    		this->enabled			= false;
    		this->fileStream 		= NULL;
    		this->debugLogFileName	= "";
			this->fileStreamOwner	= false;
    	}
    	SystemFlagsType(DebugType debugType,bool enabled,
						std::ofstream *fileStream,std::string debugLogFileName) {
    		this->debugType 		= debugType;
    		this->enabled			= enabled;
    		this->fileStream 		= fileStream;
    		this->debugLogFileName	= debugLogFileName;
			this->fileStreamOwner	= false;
    	}

    	bool enabled;
    	std::ofstream *fileStream;
    	std::string debugLogFileName;
		bool fileStreamOwner;
	};

protected:

	static int lockFile;
	static string lockfilename;

	static std::map<DebugType,SystemFlagsType> debugLogFileList;

public:

	SystemFlags();
	~SystemFlags();

	static void init();
	static SystemFlagsType & getSystemSettingType(DebugType type) { return debugLogFileList[type]; }
    static void OutputDebug(DebugType type, const char *fmt, ...);
	static void Close();
};

const string sharedLibVersionString= "v0.4.1";

//string fcs
string lastDir(const string &s);
string lastFile(const string &s);
string cutLastFile(const string &s);
string cutLastExt(const string &s);
string ext(const string &s);
string replaceBy(const string &s, char c1, char c2);
string toLower(const string &s);
void copyStringToBuffer(char *buffer, int bufferSize, const string& s);

//numeric fcs
int clamp(int value, int min, int max);
float clamp(float value, float min, float max);
float saturate(float value);
int round(float f);

//misc
bool fileExists(const string &path);

template<typename T>
void deleteValues(T beginIt, T endIt){
	for(T it= beginIt; it!=endIt; ++it){
		delete *it;
	}
}

template<typename T>
void deleteMapValues(T beginIt, T endIt){
	for(T it= beginIt; it!=endIt; ++it){
		delete it->second;
	}
}

}}//end namespace

#endif
