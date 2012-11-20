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

#include "game_util.h"

#include "util.h"
#include "lang.h"
#include "game_constants.h"
#include "config.h"
#include <stdlib.h>
#include "platform_util.h"
#include "conversion.h"
#include "cache_manager.h"
#include "leak_dumper.h"

using namespace Shared::Util;
using namespace Shared::Platform;

namespace Glest { namespace Game {

const char *mailString				= " http://bugs.megaglest.org";
const string glestVersionString 	= "v3.7.1-dev";
#if defined(SVNVERSION)
const string SVN_Rev 			= string("Rev: ") + string(SVNVERSION);
#elif defined(SVNVERSIONHEADER)
#include "svnversion.h"
const string SVN_Rev 			= string("Rev: ") + string(SVNVERSION);
#else
const string SVN_Rev 			= "$Rev$";
#endif

string getCrashDumpFileName(){
	return "megaglest" + glestVersionString + ".dmp";
}

string getPlatformNameString() {
	static string platform;
	if(platform == "") {
#if defined(WIN32)

	#if defined(__MINGW32__)
	platform = "W-Ming32";
	#else
	platform = "Windows";
	#endif

#elif defined(__FreeBSD__)
	platform = "FreeBSD";
#elif defined(__NetBSD__)
	platform = "NetBSD";
#elif defined(__OpenBSD__)
	platform = "OpenBSD";

#elif defined(__APPLE__)
	platform = "MacOSX";
#elif defined(_AIX)
	platform = "AIX";
#elif defined(__ANDROID__)
	platform = "Android";
#elif defined(__BEOS__)
	platform = "BEOS";
#elif defined(__gnu_linux__)
	platform = "Linux";
#elif defined(__sun)
	platform = "Solaris";

#elif defined(__GNUC__)

	#if defined(__MINGW32__)
	platform = "L-Ming32";
	#else
	platform = "GNU";
	#endif

#else
	platform = "???";
#endif

#if defined(_M_X64) || defined(_M_AMD64) || defined(__x86_64__) || defined(_WIN64)
	platform += "-X64";
#elif defined(_M_ALPHA) || defined(__alpha__)
	platform += "-ALPHA";
#elif defined(_M_IA64) || defined(__ia64__)
	platform += "-IA64";
#elif defined(_M_MRX000) || defined(__mips__)
	platform += "-MIPS";
#elif defined(_M_PPC) || defined(__powerpc__)
	platform += "-POWERPC";
#elif defined(__sparc__)
	platform += "-SPARC";
#elif defined(_M_ARM_FP) || defined(__arm__) || defined(_M_ARM)
	platform += "-ARM";

#endif
	}
	return platform;
}

string getSVNRevisionString() {
	return SVN_Rev;
}

string getCompilerNameString() {
	static string version = "";
	if(version == "") {
#if defined(WIN32) && defined(_MSC_VER)
	version = "VC++: " + intToStr(_MSC_VER);

#elif defined(__GNUC__)
	#if defined(__GNUC__)
	# if defined(__GNUC_PATCHLEVEL__)
	#  define __GNUC_VERSION__ (__GNUC__ * 10000 \
								+ __GNUC_MINOR__ * 100 \
								+ __GNUC_PATCHLEVEL__)
	# else
	#  define __GNUC_VERSION__ (__GNUC__ * 10000 \
								+ __GNUC_MINOR__ * 100)
	# endif
	#endif
	version = "GNUC";

	#if defined(__MINGW32__)
	version += "-MINGW";
	#endif

	version += ": " + intToStr(__GNUC_VERSION__);

#else
	version = "???";
#endif

#if defined(DEBUG) || defined(_DEBUG)
version += " [DEBUG]";
#endif

#if defined(_M_X64) || defined(_M_IA64) || defined(_M_AMD64) || defined(__x86_64__) || defined(_WIN64)
	version += " [64bit]";
#endif
	}
	return version;
}

string getNetworkVersionString() {
	static string version = "";
	if(version == "") {
		version = glestVersionString+"-"+getCompilerNameString()+"-"+getCompileDateTime();
	}
	return version;
}

string getNetworkVersionSVNString() {
	static string version = "";
	if(version == "") {
			version = glestVersionString + "-" + getCompilerNameString() + "-" + getSVNRevisionString();
	}
	return version;
}

string getCompileDateTime() {
	static string result = "";
	if(result == "") {
		result = string(__DATE__) + " " + string(__TIME__);
	}
	return result;
}

string getNetworkPlatformFreeVersionString() {
	return glestVersionString;
}

string getAboutString1(int i) {
	switch(i) {
	case 0: return "MegaGlest " + glestVersionString + " (" + "Shared Library " + sharedLibVersionString + ")";
	case 1: return "Built: " + string(__DATE__) + " " + SVN_Rev;
	case 2: return "Copyright 2001-2012 The MegaGlest Team";
	}
	return "";
}

string getAboutString2(int i) {
	switch(i) {
	case 0: return "Web: http://www.megaglest.org";
	case 1: return "Bug reports: " + string(mailString);
	case 2: return "Irc: irc://irc.freenode.net/megaglest";
	}
	return "";
}

string getTeammateName(int i) {
	switch(i) {
    case 0: return "Martiño Figueroa";
	//case 0: return "Martino Figueroa";
	case 1: return "José Luis González";
	//case 1: return "Jose Luis Gonzalez";
	case 2: return "Tucho Fernández";
	//case 2: return "Tucho Fernandez";
	case 3: return "José Zanni";
	//case 3: return "Jose Zanni";
	case 4: return "Félix Menéndez";
	//case 4: return "Felix Menendez";
	case 5: return "Marcos Caruncho";
	case 6: return "Matthias Braun";
	case 7: return "Titus Tscharntke";
	case 8: return "Mark Vejvoda";
	}
	return "";
}

string getTeammateRole(int i) {
	Lang &l= Lang::getInstance();

	switch(i){
	case 0: return l.get("Programming");
	case 1: return l.get("SoundAndMusic");
	case 2: return l.get("3dAnd2dArt");
	case 3: return l.get("2dArtAndWeb");
	case 4: return l.get("Animation");
	case 5: return l.get("3dArt");
	case 6: return l.get("LinuxPort");
	case 7: return l.get("Megaglest3d2dProgramming");
	case 8: return l.get("MegaglestProgramming");
	}
	return "";
}

string formatString(string str) {
	string outStr = str;

	if(!outStr.empty()){
		outStr[0]= toupper(outStr[0]);
	}

	bool afterSeparator= false;
	for(int i= 0; i<str.size(); ++i){
		if(outStr[i]=='_'){
			outStr[i]= ' ';
		}
		else if(afterSeparator){
			outStr[i]= toupper(outStr[i]);
			afterSeparator= false;
		}
		if(outStr[i]=='\n' || outStr[i]=='(' || outStr[i]==' '){
			afterSeparator= true;
		}
	}
	return outStr;
}

string getGameCustomCoreDataPath(string originalBasePath, string uniqueFilePath) {
	// original file path setup
    if(originalBasePath != "") {
    	endPathWithSlash(originalBasePath);
    }
    //

	// mydata user data override
	Config &config = Config::getInstance();
	string data_path = config.getString("UserData_Root","");
    if(data_path != "") {
    	endPathWithSlash(data_path);
    }
    //

    // if set this is the current active mod
    string custom_mod_path = config.getCustomRuntimeProperty(Config::ACTIVE_MOD_PROPERTY_NAME);
    if(custom_mod_path != "") {
    	endPathWithSlash(custom_mod_path);
    }
    //

    // decide which file to use
    string result = "";

	if(custom_mod_path != "" &&
		(uniqueFilePath == "" || fileExists(custom_mod_path + uniqueFilePath) == true)) {
		result = custom_mod_path + uniqueFilePath;
	}
	else if(data_path != "" &&
		(uniqueFilePath == "" || fileExists(data_path + uniqueFilePath) == true)) {
		result = data_path + uniqueFilePath;
	}
	else {
		result = originalBasePath + uniqueFilePath;
	}

	//printf("data_path [%s] result [%s]\n",data_path.c_str(),result.c_str());
    return result;
}

string getGameReadWritePath(string lookupKey) {
	string path = "";

	if(lookupKey != "") {
        std::map<string,string> &pathCache = CacheManager::getCachedItem< std::map<string,string> >(GameConstants::pathCacheLookupKey);
        std::map<string,string>::const_iterator iterFind = pathCache.find(lookupKey);
        if(iterFind != pathCache.end()) {
            path = iterFind->second;

            if(path != "" && EndsWith(path, "/") == false && EndsWith(path, "\\") == false) {
                path += "/";
            }

            //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] path to be used for [%s] files [%s]\n",__FILE__,__FUNCTION__,__LINE__,lookupKey.c_str(),path.c_str());
        }
	}

    if(path == "" && getenv("GLESTHOME") != NULL) {
        path = getenv("GLESTHOME");
        if(path != "" && EndsWith(path, "/") == false && EndsWith(path, "\\") == false) {
            path += "/";
        }

        //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] path to be used for read/write files [%s]\n",__FILE__,__FUNCTION__,__LINE__,path.c_str());
    }

    return path;
}

void initSpecialStrings() {
	getCrashDumpFileName();
	getPlatformNameString();
	getSVNRevisionString();
	getCompilerNameString();
	getNetworkVersionString();
	getNetworkVersionSVNString();
	getNetworkPlatformFreeVersionString();
	getCompileDateTime();
}

}}//end namespace
