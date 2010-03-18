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

#ifndef _SHARED_PLATFORM_PLATFORMUTIL_H_
#define _SHARED_PLATFORM_PLATFORMUTIL_H_

#include <windows.h>

#include <string>
#include <vector>
#include <stdexcept>

#include "types.h"
#include "checksum.h"
#include <utility>

using std::string;
using std::vector;
using std::exception;

using Shared::Platform::int64;
using Shared::Util::Checksum;

namespace Shared{ namespace Platform{

// =====================================================
//	class PerformanceTimer
// =====================================================

class PerformanceTimer{
private:
	int64 thisTicks;
	int64 lastTicks;
	int64 updateTicks;

	int times;			// number of consecutive times
	int maxTimes;		// maximum number consecutive times

public:
	void init(int fps, int maxTimes= -1);

	bool isTime();
	void reset();
};

// =====================================================
//	class Chrono
// =====================================================

class Chrono{
private:
	int64 startCount;
	int64 accumCount;
	int64 freq;
	bool stopped;

public:
	Chrono();
	void start();
	void stop();
	int64 getMicros() const;
	int64 getMillis() const;
	int64 getSeconds() const;

private:
	int64 queryCounter(int multiplier) const;
};

// =====================================================
//	class PlatformExceptionHandler
// =====================================================

LONG WINAPI UnhandledExceptionFilter2(struct _EXCEPTION_POINTERS *ExceptionInfo);

class PlatformExceptionHandler{
private:
	static PlatformExceptionHandler *thisPointer;

private:
	static LONG WINAPI handler(LPEXCEPTION_POINTERS pointers);
	string dumpFileName;

public:
	void install(string dumpFileName);
	virtual void handle()=0;
	static string codeToStr(DWORD code);
};

// =====================================================
//	Misc
// =====================================================
bool isdir(const char *path);
void findDirs(const vector<string> &paths, vector<string> &results, bool errorOnNotFound=false);
void findAll(const vector<string> &paths, const string &fileFilter, vector<string> &results, bool cutExtension=false, bool errorOnNotFound=true);
void findAll(const string &path, vector<string> &results, bool cutExtension=false, bool errorOnNotFound=true);
int32 getFolderTreeContentsCheckSumRecursively(const string &path, const string &filterFileExt, Checksum *recursiveChecksum);
vector<std::pair<string,int32> > getFolderTreeContentsCheckSumListRecursively(const string &path, const string &filterFileExt, vector<std::pair<string,int32> > *recursiveMap);
void createDirectoryPaths(string  Path);
string extractDirectoryPathFromFile(string filename);

bool changeVideoMode(int resH, int resW, int colorBits, int refreshFrequency);
void restoreVideoMode();

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
