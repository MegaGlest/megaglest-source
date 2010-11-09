// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2005 Matthias Braun <matze@braunis.de>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================
#ifndef _SHARED_PLATFORM_PLATFORMCOMMON_H_
#define _SHARED_PLATFORM_PLATFORMCOMMON_H_

#ifdef WIN32

#include <windows.h>

#endif

#include <string>
#include <vector>
#include <stdexcept>
#include <list>

#include "types.h"
#include "checksum.h"
#include <utility>
#include <SDL.h>
#include "leak_dumper.h"

using std::string;
using std::vector;
using std::list;
using std::exception;

using Shared::Platform::int64;
using Shared::Util::Checksum;

namespace Shared { namespace PlatformCommon {

// =====================================================
//	class PerformanceTimer
// =====================================================

class PerformanceTimer {
private:
	Uint32 lastTicks;
	Uint32 updateTicks;

	int times;			// number of consecutive times
	int maxTimes;		// maximum number consecutive times

public:
	void init(float fps, int maxTimes= -1);

	bool isTime();
	void reset();
};

// =====================================================
//	class Chrono
// =====================================================

class Chrono {
private:
	Uint32 startCount;
	Uint32 accumCount;
	Uint32 freq;
	bool stopped;

	Uint32 lastStartCount;
	Uint32 lastTickCount;
	int64  lastResult;
	int lastMultiplier;
	bool lastStopped;

public:
	Chrono();
	void start();
	void stop();
	int64 getMicros();
	int64 getMillis();
	int64 getSeconds();

    static int64 getCurTicks();
    static int64 getCurMillis();

private:
	int64 queryCounter(int multiplier);
};

// =====================================================
//	class ModeInfo
// =====================================================
class ModeInfo {
public:
	int width;
	int height;
	int depth;

	ModeInfo(int width, int height, int depth);

	string getString() const;
};

// =====================================================
//	Misc
// =====================================================
void Tokenize(const string& str,vector<string>& tokens,const string& delimiters = " ");
bool isdir(const char *path);
void findDirs(const vector<string> &paths, vector<string> &results, bool errorOnNotFound=false);
void findAll(const vector<string> &paths, const string &fileFilter, vector<string> &results, bool cutExtension=false, bool errorOnNotFound=true);
void findAll(const string &path, vector<string> &results, bool cutExtension=false, bool errorOnNotFound=true);

int32 getFolderTreeContentsCheckSumRecursively(vector<string> paths, string pathSearchString, const string filterFileExt, Checksum *recursiveChecksum);
int32 getFolderTreeContentsCheckSumRecursively(const string &path, const string &filterFileExt, Checksum *recursiveChecksum);
vector<std::pair<string,int32> > getFolderTreeContentsCheckSumListRecursively(vector<string> paths, string pathSearchString, const string filterFileExt, vector<std::pair<string,int32> > *recursiveMap);
vector<std::pair<string,int32> > getFolderTreeContentsCheckSumListRecursively(const string &path, const string &filterFileExt, vector<std::pair<string,int32> > *recursiveMap);

void createDirectoryPaths(string  Path);
string extractFileFromDirectoryPath(string filename);
string extractDirectoryPathFromFile(string filename);
string extractExtension(const string& filename);

void getFullscreenVideoModes(list<ModeInfo> *modeinfos);
void getFullscreenVideoInfo(int &colorBits,int &screenWidth,int &screenHeight);
bool changeVideoMode(int resH, int resW, int colorBits, int refreshFrequency);
void restoreVideoMode(bool exitingApp=false);

bool StartsWith(const std::string &str, const std::string &key);
bool EndsWith(const string &str, const string& key);

string replaceAll(string& context, const string& from, const string& to);

int getScreenW();
int getScreenH();

void sleep(int millis);

bool isCursorShowing();
void showCursor(bool b);
bool isKeyDown(int virtualKey);
string getCommandLine();

#define SPACES " "

inline string trim_right (const string & s, const string & t = SPACES)  {
  string d (s);
  string::size_type i (d.find_last_not_of (t));
  if (i == string::npos)
    return "";
  else
   return d.erase (d.find_last_not_of (t) + 1) ;
}  // end of trim_right

inline string trim_left (const string & s, const string & t = SPACES) {
  string d (s);
  return d.erase (0, s.find_first_not_of (t)) ;
}  // end of trim_left

inline string trim (const string & s, const string & t = SPACES) {
  string d (s);
  return trim_left (trim_right (d, t), t) ;
}  // end of trim

}}//end namespace

#endif
