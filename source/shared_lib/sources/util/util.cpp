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

#include "util.h"

#include <ctime>
#include <cassert>
#include <stdexcept>
#include <cstring>
#include <cstdio>
#include <stdarg.h>
#include <time.h>
#include <fcntl.h> // for open()

#ifdef WIN32
  #include <io.h> // for open()
#endif

#include <sys/stat.h> // for open()
#include "platform_util.h"
#include "platform_common.h"
#include "conversion.h"
#include "simple_threads.h"

#include "leak_dumper.h"

using namespace std;
using namespace Shared::Platform;
using namespace Shared::PlatformCommon;

namespace Shared{ namespace Util{

// Init statics
std::map<SystemFlags::DebugType,SystemFlags::SystemFlagsType> *SystemFlags::debugLogFileList = NULL;
int SystemFlags::lockFile								= -1;
int SystemFlags::lockFileCountIndex						= -1;
string SystemFlags::lockfilename						= "";
bool SystemFlags::haveSpecialOutputCommandLineOption 	= false;
CURL *SystemFlags::curl_handle							= NULL;
bool SystemFlags::curl_global_init_called				= false;
int SystemFlags::DEFAULT_HTTP_TIMEOUT					= 10;
bool SystemFlags::VERBOSE_MODE_ENABLED  				= false;
bool SystemFlags::ENABLE_THREADED_LOGGING 				= false;
static LogFileThread *threadLogger 						= NULL;
bool SystemFlags::SHUTDOWN_PROGRAM_MODE                 = false;
//

static void *myrealloc(void *ptr, size_t size)
{
  /* There might be a realloc() out there that doesn't like reallocing
     NULL pointers, so we take care of it here */
  if(ptr)
    return realloc(ptr, size);
  else
    return malloc(size);
}

bool SystemFlags::getThreadedLoggerRunning() {
    return (threadLogger != NULL && threadLogger->getRunningStatus() == true);
}

std::size_t SystemFlags::getLogEntryBufferCount() {
    std::size_t ret = 0;
    if(threadLogger != NULL) {
        ret = threadLogger->getLogEntryBufferCount();
    }
    return ret;
}

size_t SystemFlags::httpWriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
  size_t realsize = size * nmemb;
  struct httpMemoryStruct *mem = (struct httpMemoryStruct *)data;

  mem->memory = (char *)myrealloc(mem->memory, mem->size + realsize + 1);
  if (mem->memory) {
    memcpy(&(mem->memory[mem->size]), ptr, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
  }
  return realsize;
}

std::string SystemFlags::escapeURL(std::string URL, CURL *handle) {
	string result = URL;

	if(handle == NULL) {
		handle = SystemFlags::curl_handle;
	}
	char *escaped=curl_easy_escape(handle,URL.c_str(),0);
	if(escaped != NULL) {
		result = escaped;
		curl_free(escaped);
	}
	return result;
}

std::string SystemFlags::getHTTP(std::string URL,CURL *handle,int timeOut) {
	if(handle == NULL) {
		handle = SystemFlags::curl_handle;
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	curl_easy_setopt(handle, CURLOPT_URL, URL.c_str());

	/* send all data to this function  */
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, SystemFlags::httpWriteMemoryCallback);

	struct SystemFlags::httpMemoryStruct chunk;
	chunk.memory=NULL; /* we expect realloc(NULL, size) to work */
	chunk.size = 0;    /* no data at this point */

	/* we pass our 'chunk' struct to the callback function */
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)&chunk);

	/* some servers don't like requests that are made without a user-agent
	   field, so we provide one */
	curl_easy_setopt(handle, CURLOPT_USERAGENT, "megaglest-agent/1.0");

	/* follow HTTP redirects (status 3xx), 20 at most */
	curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(handle, CURLOPT_MAXREDIRS, 20);

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] handle = %p\n",__FILE__,__FUNCTION__,__LINE__,handle);

	if(getSystemSettingType(SystemFlags::debugNetwork).enabled == true) {
		curl_easy_setopt(handle, CURLOPT_VERBOSE, 1);
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	char errbuf[CURL_ERROR_SIZE]="";
	curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, errbuf);

	// max X seconds to connect to the URL
	if(timeOut < 0) {
		timeOut = SystemFlags::DEFAULT_HTTP_TIMEOUT;
	}
	curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, timeOut);

	/* get contents from the URL */
	CURLcode result = curl_easy_perform(handle);
	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] return code [%d] [%s]\n",__FILE__,__FUNCTION__,__LINE__,result,errbuf);

	std::string serverResponse = (chunk.memory != NULL ? chunk.memory : "");
	if(chunk.memory) {
		free(chunk.memory);
	}
	if(result != CURLE_OK) {
		serverResponse = errbuf;
	}

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] serverResponse [%s]\n",__FILE__,__FUNCTION__,__LINE__,serverResponse.c_str());

	return serverResponse;
}

CURL *SystemFlags::initHTTP() {
	if(SystemFlags::curl_global_init_called == false) {
		SystemFlags::curl_global_init_called = true;
		CURLcode result = curl_global_init(CURL_GLOBAL_ALL);
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] curl_global_init called and returned: result %d [%s]\n",__FILE__,__FUNCTION__,__LINE__,result,curl_easy_strerror(result));
		//printf("In [%s::%s Line %d] curl_global_init called and returned: result %d [%s]\n",__FILE__,__FUNCTION__,__LINE__,result,curl_easy_strerror(result));
	}
	CURL *handle = curl_easy_init();
	if(handle == NULL) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] ERROR handle = NULL\n",__FILE__,__FUNCTION__,__LINE__);
	}
	curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1);
	return handle;
}

void SystemFlags::init(bool haveSpecialOutputCommandLineOption) {
    //printf("SystemFlags::init CALLED, SystemFlags::debugLogFileList.size() = %d\n",SystemFlags::debugLogFileList.size());

	SystemFlags::haveSpecialOutputCommandLineOption = haveSpecialOutputCommandLineOption;
	//if(SystemFlags::debugLogFileList.size() == 0) {
	if(SystemFlags::debugLogFileList == NULL) {
		SystemFlags::debugLogFileList = new std::map<SystemFlags::DebugType,SystemFlags::SystemFlagsType>();

		(*SystemFlags::debugLogFileList)[SystemFlags::debugSystem] 	 		= SystemFlags::SystemFlagsType(SystemFlags::debugSystem);
		(*SystemFlags::debugLogFileList)[SystemFlags::debugNetwork] 	 	= SystemFlags::SystemFlagsType(SystemFlags::debugNetwork);
		(*SystemFlags::debugLogFileList)[SystemFlags::debugPerformance] 	= SystemFlags::SystemFlagsType(SystemFlags::debugPerformance);
		(*SystemFlags::debugLogFileList)[SystemFlags::debugWorldSynch]  	= SystemFlags::SystemFlagsType(SystemFlags::debugWorldSynch);
		(*SystemFlags::debugLogFileList)[SystemFlags::debugUnitCommands]  	= SystemFlags::SystemFlagsType(SystemFlags::debugUnitCommands);
		(*SystemFlags::debugLogFileList)[SystemFlags::debugPathFinder]  	= SystemFlags::SystemFlagsType(SystemFlags::debugPathFinder);
		(*SystemFlags::debugLogFileList)[SystemFlags::debugLUA]  			= SystemFlags::SystemFlagsType(SystemFlags::debugLUA);
		(*SystemFlags::debugLogFileList)[SystemFlags::debugSound]  			= SystemFlags::SystemFlagsType(SystemFlags::debugSound);
		(*SystemFlags::debugLogFileList)[SystemFlags::debugError]  			= SystemFlags::SystemFlagsType(SystemFlags::debugError);
	}

    if(threadLogger == NULL && SystemFlags::SHUTDOWN_PROGRAM_MODE == false) {
        threadLogger = new LogFileThread();
        threadLogger->start();
        sleep(1);
    }

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(curl_handle == NULL) {

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d]\n",__FILE__,__FUNCTION__,__LINE__);

		curl_handle = SystemFlags::initHTTP();

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] curl_handle = %p\n",__FILE__,__FUNCTION__,__LINE__,curl_handle);
	}
}

inline bool acquire_file_lock(int hnd)
{
#ifndef WIN32
   struct ::flock lock;
   lock.l_type    = F_WRLCK;
   lock.l_whence  = SEEK_SET;
   lock.l_start   = 0;
   lock.l_len     = 0;
   return -1 != ::fcntl(hnd, F_SETLK, &lock);
#else
   HANDLE hFile = (HANDLE)_get_osfhandle(hnd);
   return TRUE == ::LockFile(hFile, 0, 0, 0, -0x10000);
#endif
}

SystemFlags::SystemFlags() {

}

void SystemFlags::cleanupHTTP(CURL **handle, bool globalCleanup) {
	if(handle != NULL && *handle != NULL) {
		curl_easy_cleanup(*handle);
		*handle = NULL;

		if(globalCleanup == true) {
			if(SystemFlags::curl_global_init_called == true) {
				SystemFlags::curl_global_init_called = false;
				curl_global_cleanup();
				//printf("In [%s::%s Line %d] curl_global_cleanup called\n",__FILE__,__FUNCTION__,__LINE__);
			}
		}
	}
}

SystemFlags::~SystemFlags() {
	SystemFlags::Close();

	if(curl_handle != NULL) {
		SystemFlags::cleanupHTTP(&curl_handle, true);
		curl_handle = NULL;
	}
}

void SystemFlags::Close() {
    if(threadLogger != NULL) {
        SystemFlags::ENABLE_THREADED_LOGGING = false;
        //SystemFlags::SHUTDOWN_PROGRAM_MODE=true;
        threadLogger->signalQuit();
        threadLogger->shutdownAndWait();
        delete threadLogger;
        threadLogger = NULL;
    }

	if(SystemFlags::debugLogFileList != NULL) {
		if(SystemFlags::haveSpecialOutputCommandLineOption == false) {
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("START Closing logfiles\n");
		}

		for(std::map<SystemFlags::DebugType,SystemFlags::SystemFlagsType>::iterator iterMap = SystemFlags::debugLogFileList->begin();
			iterMap != SystemFlags::debugLogFileList->end(); iterMap++) {
			SystemFlags::SystemFlagsType &currentDebugLog = iterMap->second;
			currentDebugLog.Close();
		}

		delete SystemFlags::debugLogFileList;
		SystemFlags::debugLogFileList = NULL;
	}
	if(SystemFlags::lockFile != -1) {
#ifndef WIN32
		close(SystemFlags::lockFile);
#else
		_close(SystemFlags::lockFile);
#endif
		SystemFlags::lockFile = -1;
		SystemFlags::lockFileCountIndex = -1;

		if(SystemFlags::lockfilename != "") {
			remove(SystemFlags::lockfilename.c_str());
			SystemFlags::lockfilename = "";
		}
	}

	if(SystemFlags::debugLogFileList != NULL) {
		if(SystemFlags::haveSpecialOutputCommandLineOption == false) {
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("END Closing logfiles\n");
		}
	}
}

void SystemFlags::handleDebug(DebugType type, const char *fmt, ...) {
	if(SystemFlags::debugLogFileList == NULL) {
		SystemFlags::init(false);
	}
	SystemFlags::SystemFlagsType &currentDebugLog = (*SystemFlags::debugLogFileList)[type];
    if(currentDebugLog.enabled == false) {
        return;
    }

    va_list argList;
    va_start(argList, fmt);

    const int max_debug_buffer_size = 8096;
    char szBuf[max_debug_buffer_size]="";
    vsnprintf(szBuf,max_debug_buffer_size-1,fmt, argList);
    va_end(argList);

    if(SystemFlags::ENABLE_THREADED_LOGGING &&
       threadLogger != NULL &&
       threadLogger->getRunningStatus() == true) {
        threadLogger->addLogEntry(type, szBuf);
    }
    else {
        // Get the current time.
        time_t curtime = time (NULL);
        // Convert it to local time representation.
        struct tm *loctime = localtime (&curtime);
        char szBuf2[100]="";
        strftime(szBuf2,100,"%Y-%m-%d %H:%M:%S",loctime);

        logDebugEntry(type, szBuf, curtime);
    }
}


void SystemFlags::logDebugEntry(DebugType type, string debugEntry, time_t debugTime) {
	if(SystemFlags::debugLogFileList == NULL) {
		SystemFlags::init(false);
	}
	SystemFlags::SystemFlagsType &currentDebugLog = (*SystemFlags::debugLogFileList)[type];
    if(currentDebugLog.enabled == false) {
        return;
    }

    // Get the current time.
//    time_t curtime = time (NULL);
    // Convert it to local time representation.
    struct tm *loctime = localtime (&debugTime);
    char szBuf2[100]="";
    strftime(szBuf2,100,"%Y-%m-%d %H:%M:%S",loctime);
/*
    va_list argList;
    va_start(argList, fmt);

    const int max_debug_buffer_size = 8096;
    char szBuf[max_debug_buffer_size]="";
    vsnprintf(szBuf,max_debug_buffer_size-1,fmt, argList);
*/
    // Either output to a logfile or
    if(currentDebugLog.debugLogFileName != "") {
        if( currentDebugLog.fileStream == NULL ||
        	currentDebugLog.fileStream->is_open() == false) {

        	// If the file is already open (shared) by another debug type
        	// do not over-write the file but share the stream pointer
        	for(std::map<SystemFlags::DebugType,SystemFlags::SystemFlagsType>::iterator iterMap = SystemFlags::debugLogFileList->begin();
        		iterMap != SystemFlags::debugLogFileList->end(); iterMap++) {
        		SystemFlags::SystemFlagsType &currentDebugLog2 = iterMap->second;

        		if(	iterMap->first != type &&
        			currentDebugLog.debugLogFileName == currentDebugLog2.debugLogFileName &&
        			currentDebugLog2.fileStream != NULL) {
					currentDebugLog.fileStream = currentDebugLog2.fileStream;
					currentDebugLog.fileStreamOwner = false;
					currentDebugLog.mutex			= currentDebugLog2.mutex;
        			break;
        		}
        	}

        	string debugLog = currentDebugLog.debugLogFileName;

			if(SystemFlags::lockFile == -1) {
				const string lock_file_name = "debug.lck";
				string lockfile = extractDirectoryPathFromFile(debugLog);
				lockfile += lock_file_name;
				SystemFlags::lockfilename = lockfile;

#ifndef WIN32
				//SystemFlags::lockFile = open(lockfile.c_str(), O_WRONLY | O_CREAT | O_EXCL, S_IRUSR|S_IWUSR);
				SystemFlags::lockFile = open(lockfile.c_str(), O_WRONLY | O_CREAT, S_IREAD | S_IWRITE);
#else
				SystemFlags::lockFile = _open(lockfile.c_str(), O_WRONLY | O_CREAT, S_IREAD | S_IWRITE);
#endif
				if (SystemFlags::lockFile < 0 || acquire_file_lock(SystemFlags::lockFile) == false) {
            		string newlockfile = lockfile;
            		int idx = 1;
            		for(idx = 1; idx <= 100; ++idx) {
						newlockfile = lockfile + intToStr(idx);
						//SystemFlags::lockFile = open(newlockfile.c_str(), O_WRONLY | O_CREAT | O_EXCL, S_IRUSR|S_IWUSR);
#ifndef WIN32
	                    SystemFlags::lockFile = open(newlockfile.c_str(), O_WRONLY | O_CREAT, S_IREAD | S_IWRITE);
#else
		                SystemFlags::lockFile = _open(newlockfile.c_str(), O_WRONLY | O_CREAT, S_IREAD | S_IWRITE);

#endif
						if(SystemFlags::lockFile >= 0 && acquire_file_lock(SystemFlags::lockFile) == true) {
                    		break;
						}
            		}

            		SystemFlags::lockFileCountIndex = idx;
            		SystemFlags::lockfilename = newlockfile;
            		debugLog += intToStr(idx);

            		if(SystemFlags::haveSpecialOutputCommandLineOption == false) {
            			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Opening additional logfile [%s]\n",debugLog.c_str());
            		}
				}
			}
			else if(SystemFlags::lockFileCountIndex > 0) {
        		debugLog += intToStr(SystemFlags::lockFileCountIndex);

        		if(SystemFlags::haveSpecialOutputCommandLineOption == false) {
        			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Opening additional logfile [%s]\n",debugLog.c_str());
        		}
			}

            if(currentDebugLog.fileStream == NULL) {
            	currentDebugLog.fileStream = new std::ofstream();
            	currentDebugLog.fileStream->open(debugLog.c_str(), ios_base::out | ios_base::trunc);
				currentDebugLog.fileStreamOwner = true;
				currentDebugLog.mutex			= new Mutex();
            }

            if(SystemFlags::haveSpecialOutputCommandLineOption == false) {
            	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Opening logfile [%s] type = %d, currentDebugLog.fileStreamOwner = %d, file stream open = %d\n",debugLog.c_str(),type, currentDebugLog.fileStreamOwner,currentDebugLog.fileStream->is_open());
            }

            if(currentDebugLog.fileStream->is_open() == true) {
				MutexSafeWrapper safeMutex(currentDebugLog.mutex,string(__FILE__) + "_" + intToStr(__LINE__));

				(*currentDebugLog.fileStream) << "Starting Mega-Glest logging for type: " << type << "\n";
				(*currentDebugLog.fileStream).flush();

				safeMutex.ReleaseLock();
            }
        }

        assert(currentDebugLog.fileStream != NULL);

        if(currentDebugLog.fileStream->is_open() == true) {
			MutexSafeWrapper safeMutex(currentDebugLog.mutex,string(__FILE__) + "_" + intToStr(__LINE__));

			// All items in the if clause we don't want timestamps
			if (type != debugPathFinder && type != debugError && type != debugWorldSynch) {
				(*currentDebugLog.fileStream) << "[" << szBuf2 << "] " << debugEntry.c_str();
			}
			else if (type == debugError) {
				(*currentDebugLog.fileStream) << "[" << szBuf2 << "] *ERROR* " << debugEntry.c_str();
			}
			else {
				(*currentDebugLog.fileStream) << debugEntry.c_str();
			}
			(*currentDebugLog.fileStream).flush();

			safeMutex.ReleaseLock();
        }
    }

    // output to console
    if(	currentDebugLog.debugLogFileName == "" ||
    	(currentDebugLog.debugLogFileName != "" &&
        (currentDebugLog.fileStream == NULL ||
         currentDebugLog.fileStream->is_open() == false))) {

		if (type != debugPathFinder && type != debugError) {
			printf("[%s] %s", szBuf2, debugEntry.c_str());
		}
		else if (type == debugError) {
			printf("[%s] *ERROR* %s", szBuf2, debugEntry.c_str());
		}
		else {
			printf("%s", debugEntry.c_str());
		}
    }
}


string lastDir(const string &s){
	size_t i= s.find_last_of('/');
	size_t j= s.find_last_of('\\');
	size_t pos;

	if(i==string::npos){
		pos= j;
	}
	else if(j==string::npos){
		pos= i;
	}
	else{
		pos= i<j? j: i;
	}

	if (pos==string::npos){
		throw runtime_error(string(__FILE__)+" lastDir - i==string::npos");
	}

	return (s.substr(pos+1, s.length()));
}

string lastFile(const string &s){
	return lastDir(s);
}

string cutLastFile(const string &s){
	size_t i= s.find_last_of('/');
	size_t j= s.find_last_of('\\');
	size_t pos;

	if(i==string::npos){
		pos= j;
	}
	else if(j==string::npos){
		pos= i;
	}
	else{
		pos= i<j? j: i;
	}

	if (pos==string::npos){
		throw runtime_error(string(__FILE__)+"cutLastFile - i==string::npos");
	}

	return (s.substr(0, pos));
}

string cutLastExt(const string &s){
     size_t i= s.find_last_of('.');

	 if (i==string::npos){
          throw runtime_error(string(__FILE__)+"cutLastExt - i==string::npos");
	 }

     return (s.substr(0, i));
}

string ext(const string &s){
     size_t i;

     i=s.find_last_of('.')+1;

	 if (i==string::npos){
          throw runtime_error(string(__FILE__)+"cutLastExt - i==string::npos");
	 }

     return (s.substr(i, s.size()-i));
}

string replaceBy(const string &s, char c1, char c2){
	string rs= s;

	for(size_t i=0; i<s.size(); ++i){
		if (rs[i]==c1){
			rs[i]=c2;
		}
	}

	return rs;
}

string toLower(const string &s){
	string rs= s;

	for(size_t i=0; i<s.size(); ++i){
		rs[i]= tolower(s[i]);
	}

	return rs;
}

void copyStringToBuffer(char *buffer, int bufferSize, const string& s){
	strncpy(buffer, s.c_str(), bufferSize-1);
	buffer[bufferSize-1]= '\0';
}

// ==================== numeric fcs ====================

float saturate(float value){
	if (value<0.f){
        return 0.f;
	}
	if (value>1.f){
        return 1.f;
	}
    return value;
}

int clamp(int value, int min, int max){
	if (value<min){
        return min;
	}
	if (value>max){
        return max;
	}
    return value;
}

float clamp(float value, float min, float max){
	if (value<min){
        return min;
	}
	if (value>max){
        return max;
	}
    return value;
}

int round(float f){
     return (int) f;
}

// ==================== misc ====================

bool fileExists(const string &path){
	FILE* file= fopen(path.c_str(), "rb");
	if(file!=NULL){
		fclose(file);
		return true;
	}
	return false;
}

bool checkVersionComptability(string clientVersionString, string serverVersionString) {
	bool compatible = (clientVersionString == serverVersionString);
	if(compatible == false) {
		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] clientVersionString [%s], serverVersionString [%s]\n",__FILE__,__FUNCTION__,__LINE__,clientVersionString.c_str(),serverVersionString.c_str());

		vector<string> tokens;
		vector<string> tokensServer;
		Tokenize(clientVersionString,tokens,".");
		Tokenize(serverVersionString,tokensServer,".");

		// only check the first 3 sections with . to compare makor versions #'s
		compatible = (tokens.size() >= 3 && tokensServer.size() >= 3);

		SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] clientVersionString [%s], serverVersionString [%s] compatible [%d]\n",__FILE__,__FUNCTION__,__LINE__,clientVersionString.c_str(),serverVersionString.c_str(),compatible);

		for(int i = 0; compatible == true && i < 3; ++i) {
			if(tokens[i] != tokensServer[i]) {
				compatible = false;

				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] tokens[i] = [%s], tokensServer[i] = [%s] compatible [%d]\n",__FILE__,__FUNCTION__,__LINE__,tokens[i].c_str(),tokensServer[i].c_str(),compatible);
			}
		}
	}

	return compatible;
}

}}//end namespace
