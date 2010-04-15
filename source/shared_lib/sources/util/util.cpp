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
#include "conversion.h"

#include "leak_dumper.h"

using namespace std;
using namespace Shared::Platform;

namespace Shared{ namespace Util{

bool SystemFlags::enableDebugText				= false;
bool SystemFlags::enableNetworkDebugInfo		= false;
bool SystemFlags::enablePerformanceDebugInfo	= false;

const char * SystemFlags::debugLogFile			= NULL;
ofstream SystemFlags::fileStream;
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
   return true == ::LockFile(hFile, 0, 0, 0, -0x10000);
#endif
}

SystemFlags::SystemFlags() {

}
SystemFlags::~SystemFlags() {
	SystemFlags::Close();
}

void SystemFlags::Close() {
	printf("Closing logfile\n");

	if(fileStream.is_open() == true) {
		SystemFlags::fileStream.close();
	}
	if(SystemFlags::lockfilename != "") {
#ifndef WIN32
		close(SystemFlags::lockFile);
#else
		_close(SystemFlags::lockFile);
#endif
		remove(SystemFlags::lockfilename.c_str());
		SystemFlags::lockfilename = "";
	}
}

void SystemFlags::OutputDebug(DebugType type, const char *fmt, ...) {
    if((type == debugSystem		 && SystemFlags::enableDebugText == false) ||
       (type == debugNetwork	 && SystemFlags::enableNetworkDebugInfo == false ||
	    type == debugPerformance && SystemFlags::enablePerformanceDebugInfo == false)) {
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
    if(SystemFlags::debugLogFile != NULL && SystemFlags::debugLogFile[0] != 0) {
        if(fileStream.is_open() == false) {
        	string debugLog = SystemFlags::debugLogFile;
            printf("Opening logfile [%s]\n",debugLog.c_str());

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

            fileStream.open(debugLog.c_str(), ios_base::out | ios_base::trunc);
        }

        //printf("Logfile is open [%s]\n",SystemFlags::debugLogFile);

        //printf("writing to logfile [%s]\n",szBuf);
        fileStream << "[" << szBuf2 << "] " << szBuf;
        fileStream.flush();
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
