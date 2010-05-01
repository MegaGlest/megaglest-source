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
  #include <sys/stat.h> // for open()
#endif

#include "platform_util.h"
#include "platform_common.h"
#include "conversion.h"

#include "leak_dumper.h"

using namespace std;
using namespace Shared::Platform;
using namespace Shared::PlatformCommon;

namespace Shared{ namespace Util{

std::map<SystemFlags::DebugType,SystemFlags::SystemFlagsType> SystemFlags::debugLogFileList;

void SystemFlags::init() {
	SystemFlags::debugLogFileList[SystemFlags::debugSystem] 	 = SystemFlags::SystemFlagsType(SystemFlags::debugSystem);
	SystemFlags::debugLogFileList[SystemFlags::debugNetwork] 	 = SystemFlags::SystemFlagsType(SystemFlags::debugNetwork);
	SystemFlags::debugLogFileList[SystemFlags::debugPerformance] = SystemFlags::SystemFlagsType(SystemFlags::debugPerformance);
	SystemFlags::debugLogFileList[SystemFlags::debugWorldSynch]  = SystemFlags::SystemFlagsType(SystemFlags::debugWorldSynch);
}

int SystemFlags::lockFile						= -1;
string SystemFlags::lockfilename				= "";

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
SystemFlags::~SystemFlags() {
	SystemFlags::Close();
}

void SystemFlags::Close() {
	printf("START Closing logfiles\n");

	for(std::map<SystemFlags::DebugType,SystemFlags::SystemFlagsType>::iterator iterMap = SystemFlags::debugLogFileList.begin();
		iterMap != SystemFlags::debugLogFileList.end(); iterMap++) {
		SystemFlags::SystemFlagsType &currentDebugLog = iterMap->second;

		if(currentDebugLog.fileStreamOwner == true) {
			if( currentDebugLog.fileStream != NULL &&
				currentDebugLog.fileStream->is_open() == true) {
				currentDebugLog.fileStream->close();
			}
			delete currentDebugLog.fileStream;
			//delete currentDebugLog.mutex;
		}
		currentDebugLog.fileStream = NULL;
		currentDebugLog.fileStreamOwner = false;
		currentDebugLog.mutex = NULL;
	}

	if(SystemFlags::lockFile != -1) {
#ifndef WIN32
		close(SystemFlags::lockFile);
#else
		_close(SystemFlags::lockFile);
#endif
		SystemFlags::lockFile = -1;

		if(SystemFlags::lockfilename != "") {
			remove(SystemFlags::lockfilename.c_str());
			SystemFlags::lockfilename = "";
		}
	}

	printf("END Closing logfiles\n");
}

void SystemFlags::OutputDebug(DebugType type, const char *fmt, ...) {
	SystemFlags::SystemFlagsType &currentDebugLog = SystemFlags::debugLogFileList[type];
    if(currentDebugLog.enabled == false) {
        return;
    }

    /* Get the current time.  */
    time_t curtime = time (NULL);
    /* Convert it to local time representation.  */
    struct tm *loctime = localtime (&curtime);
    char szBuf2[100]="";
    strftime(szBuf2,100,"%Y-%m-%d %H:%M:%S",loctime);

    va_list argList;
    va_start(argList, fmt);

    char szBuf[1024]="";
    vsprintf(szBuf,fmt, argList);

    // Either output to a logfile or
    if(currentDebugLog.debugLogFileName != "") {
        if( currentDebugLog.fileStream == NULL ||
        	currentDebugLog.fileStream->is_open() == false) {

        	// If the file is already open (shared) by another debug type
        	// do not over-write the file but share the stream pointer
        	for(std::map<SystemFlags::DebugType,SystemFlags::SystemFlagsType>::iterator iterMap = SystemFlags::debugLogFileList.begin();
        		iterMap != SystemFlags::debugLogFileList.end(); iterMap++) {
        		SystemFlags::SystemFlagsType &currentDebugLog2 = iterMap->second;

        		if(	iterMap->first != type &&
        			currentDebugLog.debugLogFileName == currentDebugLog2.debugLogFileName &&
        			currentDebugLog2.fileStream != NULL) {
					currentDebugLog.fileStream = currentDebugLog2.fileStream;
					currentDebugLog.fileStreamOwner = false;
					//currentDebugLog.mutex			= currentDebugLog2.mutex;
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

            		SystemFlags::lockfilename = newlockfile;
            		debugLog += intToStr(idx);

            		printf("Opening additional logfile [%s]\n",debugLog.c_str());
				}
			}

            if(currentDebugLog.fileStream == NULL) {
            	currentDebugLog.fileStream = new std::ofstream();
            	currentDebugLog.fileStream->open(debugLog.c_str(), ios_base::out | ios_base::trunc);
				currentDebugLog.fileStreamOwner = true;
				//currentDebugLog.mutex			= new Mutex();
            }

            printf("Opening logfile [%s] type = %d, currentDebugLog.fileStreamOwner = %d\n",debugLog.c_str(),type, currentDebugLog.fileStreamOwner);

			//currentDebugLog.mutex->p();

            (*currentDebugLog.fileStream) << "Starting Mega-Glest logging for type: " << type << "\n";
            (*currentDebugLog.fileStream).flush();

			//currentDebugLog.mutex->v();
        }

        //printf("Logfile is open [%s]\n",SystemFlags::debugLogFile);

        //printf("writing to logfile [%s]\n",szBuf);

        assert(currentDebugLog.fileStream != NULL);

		//currentDebugLog.mutex->p();

        (*currentDebugLog.fileStream) << "[" << szBuf2 << "] " << szBuf;
        (*currentDebugLog.fileStream).flush();

		//currentDebugLog.mutex->v();
    }
    // output to console
    else {
        printf("[%s] %s", szBuf2, szBuf);
    }

    va_end(argList);
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

}}//end namespace
