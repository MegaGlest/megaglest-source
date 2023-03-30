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

#ifndef _GLEST_GAME_GAMEUTIL_H_
#define _GLEST_GAME_GAMEUTIL_H_

#ifdef WIN32
#include <winsock.h>
#include <winsock2.h>
#endif

#include "leak_dumper.h"
#include "util.h"
#include <string>
#include <vector>

using Shared::Util::sharedLibVersionString;
using std::string;

namespace Glest {
namespace Game {

extern const char *mailString;
extern const string glestVersionString;
extern const string lastCompatibleSaveGameVersionString;
extern const string networkVersionString;

void initSpecialStrings();
string getCrashDumpFileName();
string getPlatformTypeNameString();
string getPlatformArchTypeNameString();
string getPlatformNameString();
string getGITRevisionString();
string getRAWGITRevisionString();
string getCompilerNameString();
string getNetworkVersionString();
string getNetworkVersionGITString();
string getNetworkPlatformFreeVersionString();
string getAboutString1(int i);
string getAboutString2(int i);
string getTeammateName(int i);
string getTeammateRole(int i);
string getCompileDateTime();

string formatString(string str);

string getGameReadWritePath(const string &lookupKey = "");
string getGameCustomCoreDataPath(string originalBasePath,
                                 string uniqueFilePath);

bool upgradeFilesInTemp();

} // namespace Game
} // namespace Glest

#endif
