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
#include "thread.h"
#include <curl/curl.h>
#include <cstdio>
#include "leak_dumper.h"

using std::string;
using namespace Shared::Platform;

//#define UNDEF_DEBUG

namespace Shared{ namespace Util{

class SystemFlags
{
public:

	struct httpMemoryStruct {
	  char *memory;
	  size_t size;
	};

    enum DebugType {
        debugSystem,
        debugNetwork,
		debugPerformance,
		debugWorldSynch,
		debugUnitCommands,
		debugPathFinder,
		debugLUA,
		debugSound,
		debugError
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
			this->mutex				= NULL;
    	}
    	SystemFlagsType(DebugType debugType) {
    		this->debugType 		= debugType;
    		this->enabled			= false;
    		this->fileStream 		= NULL;
    		this->debugLogFileName	= "";
			this->fileStreamOwner	= false;
			this->mutex				= NULL;
    	}
		~SystemFlagsType() {
			Close();
		}
    	SystemFlagsType(DebugType debugType,bool enabled,
						std::ofstream *fileStream,std::string debugLogFileName) {
    		this->debugType 		= debugType;
    		this->enabled			= enabled;
    		this->fileStream 		= fileStream;
    		this->debugLogFileName	= debugLogFileName;
			this->fileStreamOwner	= false;
			this->mutex				= NULL;
    	}

		void Close() {
			if(this->fileStreamOwner == true) {
				if( this->fileStream != NULL &&
					this->fileStream->is_open() == true) {
					this->fileStream->close();
				}
				delete this->fileStream;
				delete this->mutex;
			}
			this->fileStream = NULL;
			this->fileStreamOwner = false;
			this->mutex = NULL;
		}

    	bool enabled;
    	std::ofstream *fileStream;
    	std::string debugLogFileName;
		bool fileStreamOwner;
		Mutex *mutex;
	};

protected:

	static int lockFile;
	static string lockfilename;
	static int lockFileCountIndex;

	static std::map<DebugType,SystemFlagsType> *debugLogFileList;
	static bool haveSpecialOutputCommandLineOption;
	static bool curl_global_init_called;

public:

	static CURL *curl_handle;
	static int DEFAULT_HTTP_TIMEOUT;
	static bool VERBOSE_MODE_ENABLED;
	static bool ENABLE_THREADED_LOGGING;
	static bool SHUTDOWN_PROGRAM_MODE;

	SystemFlags();
	~SystemFlags();

	static void init(bool haveSpecialOutputCommandLineOption);
	static SystemFlagsType & getSystemSettingType(DebugType type);
	static size_t httpWriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data);
	static std::string getHTTP(std::string URL,CURL *handle=NULL, int timeOut=-1, CURLcode *savedResult=NULL);
	static std::string escapeURL(std::string URL, CURL *handle=NULL);

	static CURL *initHTTP();
	static void cleanupHTTP(CURL **handle,bool globalCleanup=false);
	static void globalCleanupHTTP();

    static bool getThreadedLoggerRunning();
    static std::size_t getLogEntryBufferCount();

	// Let the macro call into this when require.. NEVER call it automatically.
	static void handleDebug(DebugType type, const char *fmt, ...);
	static void logDebugEntry(DebugType type, string debugEntry, time_t debugTime);

// If logging is enabled then define the logging method
#ifndef UNDEF_DEBUG

#ifndef WIN32
#define OutputDebug(type, fmt, ...) SystemFlags::handleDebug (type, fmt, ##__VA_ARGS__)
// Uncomment the line below to get the compiler to warn us of badly formatted printf like statements which could trash memory
//#define OutputDebug(type, fmt, ...) type; printf(fmt, ##__VA_ARGS__)

#else
#define OutputDebug(type, fmt, ...) handleDebug (type, fmt, ##__VA_ARGS__)
#endif

// stub out logging
#else
// stub out debugging completely
#ifndef WIN32
	#define OutputDebug(type, fmt, ...) type
#else
	static void nothing() {}
	#define OutputDebug(type, fmt, ...) nothing()
#endif

#endif

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
bool checkVersionComptability(string clientVersionString, string serverVersionString);

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
		it->second = NULL;
	}
}

template <typename T, typename U>
class create_map
{
private:
    std::map<T, U> m_map;
public:
    create_map(const T& key, const U& val)
    {
        m_map[key] = val;
    }

    create_map<T, U>& operator()(const T& key, const U& val)
    {
        m_map[key] = val;
        return *this;
    }

    operator std::map<T, U>()
    {
        return m_map;
    }
};
}}//end namespace

#endif
