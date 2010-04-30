// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
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
#include "leak_dumper.h"

using namespace Shared::Util;
using namespace Shared::Platform;

namespace Glest{ namespace Game{

const string mailString= "contact_game@glest.org";
const string glestVersionString= "v3.3.5-dev";

string getCrashDumpFileName(){
	return "glest" + glestVersionString + ".dmp";
}

string getNetworkVersionString() {
	return glestVersionString + " built: " + string(__DATE__) + " " + string(__TIME__);
}

string getNetworkPlatformFreeVersionString() {
	return glestVersionString;
}

string getAboutString1(int i){
	switch(i){
	case 0: return "Glest " + glestVersionString + " (" + "Shared Library " + sharedLibVersionString + ")";
	case 1: return "Built: " + string(__DATE__);
	case 2: return "Copyright 2001-2010 The Glest Team";
	}
	return "";
}

string getAboutString2(int i){
	switch(i){
	case 0: return "Web: http://sourceforge.net/projects/megaglest  http://glest.org";
	case 1: return "Mail: " + mailString;
	case 2: return "Irc: irc://irc.freenode.net/glest";
	}
	return "";
}

string getTeammateName(int i){
	switch(i){
	case 0: return "Marti�o Figueroa";
	case 1: return "Jos� Luis Gonz�lez";
	case 2: return "Tucho Fern�ndez";
	case 3: return "Jos� Zanni";
	case 4: return "F�lix Men�ndez";
	case 5: return "Marcos Caruncho";
	case 6: return "Matthias Braun";
	case 7: return "Titus Tscharntke";
	case 8: return "Mark Vejvoda";
	}
	return "";
}

string getTeammateRole(int i){
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

string formatString(const string &str){
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

string getGameReadWritePath() {
	string path         = "";
    if(getenv("GLESTHOME") != NULL) {
        path = getenv("GLESTHOME");
        if(path != "" && EndsWith(path, "/") == false && EndsWith(path, "\\") == false) {
            path += "/";
        }

        SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] path to be used for read/write files [%s]\n",__FILE__,__FUNCTION__,__LINE__,path.c_str());
    }

    return path;
}

}}//end namespace
