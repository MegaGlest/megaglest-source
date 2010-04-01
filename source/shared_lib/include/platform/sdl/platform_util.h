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
#ifndef _SHARED_PLATFORM_PLATFORMUTIL_H_
#define _SHARED_PLATFORM_PLATFORMUTIL_H_

#include <string>
#include <vector>
#include <stdexcept>
#include <list>


#include <SDL.h>

#include "types.h"
#include "checksum.h"
#include <utility>

using std::string;
using std::vector;
using std::list;
using std::exception;

using Shared::Platform::int64;

using Shared::Util::Checksum;

namespace Shared{ namespace Platform{

// =====================================================
//	class PerformanceTimer
// =====================================================

class PerformanceTimer{
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

public:
	Chrono();
	void start();
	void stop();
	int64 getMicros() const;
	int64 getMillis() const;
	int64 getSeconds() const;

    static int64 getCurTicks();
    static int64 getCurMillis();

private:
	int64 queryCounter(int multiplier) const;
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
//	class PlatformExceptionHandler
// =====================================================

class PlatformExceptionHandler {
public:
	virtual ~PlatformExceptionHandler() {}
	void install(string dumpFileName) {}
	virtual void handle()=0;
};

// =====================================================
//	Misc
// =====================================================
int MessageBox(int handle, const char *msg, const char *title, int buttons);
bool isdir(const char *path);
void findDirs(const vector<string> &paths, vector<string> &results, bool errorOnNotFound=false);
void findAll(const vector<string> &paths, const string &fileFilter, vector<string> &results, bool cutExtension=false, bool errorOnNotFound=true);
void findAll(const string &path, vector<string> &results, bool cutExtension=false, bool errorOnNotFound=true);
int32 getFolderTreeContentsCheckSumRecursively(const string &path, const string &filterFileExt, Checksum *recursiveChecksum);
vector<std::pair<string,int32> > getFolderTreeContentsCheckSumListRecursively(const string &path, const string &filterFileExt, vector<std::pair<string,int32> > *recursiveMap);
void createDirectoryPaths(string  Path);
string extractDirectoryPathFromFile(string filename);
string extractExtension(const string& filename);

void getFullscreenVideoModes(list<ModeInfo> *modeinfos);
void getFullscreenVideoInfo(int &colorBits,int &screenWidth,int &screenHeight);
bool changeVideoMode(int resH, int resW, int colorBits, int refreshFrequency);
void restoreVideoMode(bool exitingApp=false);

bool EndsWith(const string &str, const string& key);
void message(string message);
bool ask(string message);
void exceptionMessage(const exception &excp);

int getScreenW();
int getScreenH();

void sleep(int millis);

void showCursor(bool b);
bool isKeyDown(int virtualKey);
string getCommandLine();

}}//end namespace

#endif
