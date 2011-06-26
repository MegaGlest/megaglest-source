// ==============================================================
//This file is part of Glest Shared Library (www.glest.org)
//Copyright (C) 2005 Matthias Braun <matze@braunis.de>

//You can redistribute this code and/or modify it under
//the terms of the GNU General Public License as published by the Free Software
//Foundation; either version 2 of the License, or (at your option) any later
//version.
#include "platform_common.h"
#include "cache_manager.h"

#ifdef WIN32

#include <io.h>
#include <dbghelp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>

#else

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#endif

#ifdef WIN32
 #define S_ISDIR(mode) (((mode) & _S_IFDIR) == _S_IFDIR)
#elif defined(__GNUC__)

#else
#error "Your compiler needs to support S_IFDIR!"
#endif

#include <iostream>
#include <sstream>
#include <cassert>

#include <glob.h>
#include <errno.h>
#include <string.h>

#include <SDL.h>

#include "util.h"
#include "conversion.h"
#include "leak_dumper.h"
#include "sdl_private.h"
#include "window.h"
#include "noimpl.h"

#include "checksum.h"
#include "socket.h"
#include <algorithm>
#include <map>
#include "randomgen.h"
#include <algorithm>
#include "platform_util.h"
#include "utf8.h"
#include "leak_dumper.h"

using namespace Shared::Platform;
using namespace Shared::Util;
using namespace std;

#define _DISABLE_MEMORY_VAULT_CHECKS 1

namespace Shared { namespace PlatformCommon {

const time_t REFRESH_CRC_DAY_SECONDS = 60 * 60 * 24;
static string crcCachePath = "";

namespace Private {

bool shouldBeFullscreen = false;
int ScreenWidth = 800;
int ScreenHeight = 600;

}

// =====================================
//          PerformanceTimer
// =====================================

void PerformanceTimer::init(float fps, int maxTimes){
	times= 0;
	this->maxTimes= maxTimes;

	lastTicks= SDL_GetTicks();

	updateTicks= static_cast<int>(1000./fps);
}

bool PerformanceTimer::isTime(){
	Uint32 thisTicks = SDL_GetTicks();

	if((thisTicks-lastTicks)>=updateTicks && times<maxTimes){
		lastTicks+= updateTicks;
		times++;
		return true;
	}
	times= 0;
	return false;
}

void PerformanceTimer::reset(){
	lastTicks= SDL_GetTicks();
}

// =====================================
//         Chrono
// =====================================

Chrono::Chrono() {
	freq = 1000;
	stopped= true;
	accumCount= 0;

	lastStartCount = 0;
	lastTickCount = 0;
	lastResult = 0;
	lastMultiplier = 0;
	lastStopped = false;
}

void Chrono::start() {
	stopped= false;
	startCount = SDL_GetTicks();
}

void Chrono::stop() {
	Uint32 endCount;
	endCount = SDL_GetTicks();
	accumCount += endCount-startCount;
	stopped= true;
}

int64 Chrono::getMicros() {
	return queryCounter(1000000);
}

int64 Chrono::getMillis()  {
	return queryCounter(1000);
}

int64 Chrono::getSeconds() {
	return queryCounter(1);
}

int64 Chrono::queryCounter(int multiplier) {

	if(	multiplier == lastMultiplier &&
		stopped == lastStopped &&
		lastStartCount == startCount) {

		if(stopped) {
			return lastResult;
		}
		else {
			Uint32 endCount = SDL_GetTicks();
			if(lastTickCount == endCount) {
				return lastResult;
			}
		}
	}

	int64 result = 0;
	if(stopped) {
		result = multiplier*accumCount/freq;
	}
	else {
		Uint32 endCount = SDL_GetTicks();
		result = multiplier*(accumCount+endCount-startCount)/freq;
		lastTickCount = endCount;
	}

	lastStartCount = startCount;
	lastResult = result;
	lastMultiplier = multiplier;
	lastStopped = stopped;

	return result;
}

int64 Chrono::getCurMillis() {
    return SDL_GetTicks();
}
int64 Chrono::getCurTicks() {
    return SDL_GetTicks();
}



// =====================================
//         Misc
// =====================================

void Tokenize(const string& str,vector<string>& tokens,const string& delimiters) {

/*
    // Skip delimiters at beginning.
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    string::size_type pos     = str.find_first_of(delimiters, lastPos);

    while (string::npos != pos || string::npos != lastPos) {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
*/

	// Assume textLine contains the line of text to parse.
	string textLine = str;

	size_t pos = 0;
	while( true ) {
		size_t nextPos = textLine.find( delimiters, pos );
		if( nextPos == textLine.npos ) {
			tokens.push_back(textLine.substr(pos, textLine.length( )));
			break;
		}
		tokens.push_back( string( textLine.substr( pos, nextPos - pos ) ) );
		pos = nextPos + 1;
	}

}

void findDirs(string path, vector<string> &results, bool errorOnNotFound,bool keepDuplicates) {
    results.clear();
	string currentPath = path;
	endPathWithSlash(currentPath);

	string searchpath = currentPath + "*.";
	vector<string> current_results;
	findAll(searchpath, current_results, false, errorOnNotFound);
	if(current_results.size() > 0) {
		for(unsigned int folder_index = 0; folder_index < current_results.size(); folder_index++) {
			const string current_folder = current_results[folder_index];
			const string current_folder_path = currentPath + current_folder;

			if(isdir(current_folder_path.c_str()) == true) {
				if(keepDuplicates == true || std::find(results.begin(),results.end(),current_folder) == results.end()) {
					results.push_back(current_folder);
				}
			}
		}
	}

    std::sort(results.begin(),results.end());
}

void findDirs(const vector<string> &paths, vector<string> &results, bool errorOnNotFound,bool keepDuplicates) {
    results.clear();
    size_t pathCount = paths.size();
    for(unsigned int idx = 0; idx < pathCount; idx++) {
    	string currentPath = paths[idx];
    	endPathWithSlash(currentPath);
        string path = currentPath + "*.";
        vector<string> current_results;
        findAll(path, current_results, false, errorOnNotFound);
        if(current_results.size() > 0) {
            for(unsigned int folder_index = 0; folder_index < current_results.size(); folder_index++) {
                const string current_folder = current_results[folder_index];
                const string current_folder_path = currentPath + current_folder;

                if(isdir(current_folder_path.c_str()) == true) {
                    if(keepDuplicates == true || std::find(results.begin(),results.end(),current_folder) == results.end()) {
                        results.push_back(current_folder);
                    }
                }
            }
        }
    }

    std::sort(results.begin(),results.end());
}

void findAll(const vector<string> &paths, const string &fileFilter, vector<string> &results, bool cutExtension, bool errorOnNotFound, bool keepDuplicates) {
    results.clear();
    size_t pathCount = paths.size();
    for(unsigned int idx = 0; idx < pathCount; idx++) {
    	string currentPath = paths[idx];
    	endPathWithSlash(currentPath);

    	string path = currentPath + fileFilter;
        vector<string> current_results;
        findAll(path, current_results, cutExtension, errorOnNotFound);
        if(current_results.size() > 0) {
            for(unsigned int folder_index = 0; folder_index < current_results.size(); folder_index++) {
                string current_file = current_results[folder_index];
                if(keepDuplicates == true || std::find(results.begin(),results.end(),current_file) == results.end()) {
                    results.push_back(current_file);
                }
            }
        }
    }

    std::sort(results.begin(),results.end());
}

//finds all filenames like path and stores them in results
void findAll(const string &path, vector<string> &results, bool cutExtension, bool errorOnNotFound) {
	results.clear();

	std::string mypath = path;
	/** Stupid win32 is searching for all files without extension when *. is
	 * specified as wildcard
	 */
	if(mypath.size() >= 2 && mypath.compare(max((size_t)0,mypath.size() - 2), 2, "*.") == 0) {
		mypath = mypath.substr(0, max((size_t)0,mypath.size() - 2));
		mypath += "*";
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] scanning [%s]\n",__FILE__,__FUNCTION__,mypath.c_str());

	glob_t globbuf;

	int res = glob(mypath.c_str(), 0, 0, &globbuf);
	if(res < 0 && errorOnNotFound == true) {
		if(errorOnNotFound) {
			std::stringstream msg;
			msg << "#1 Couldn't scan directory '" << mypath << "': " << strerror(errno);
			throw runtime_error(msg.str());
		}
	}
	else {
		for(int i = 0; i < globbuf.gl_pathc; ++i) {
			const char* p = globbuf.gl_pathv[i];
			const char* begin = p;
			for( ; *p != 0; ++p) {
				// strip the path component
				if(*p == '/')
					begin = p+1;
			}
			if(!(strcmp(".", begin)==0 || strcmp("..", begin)==0 || strcmp(".svn", begin)==0)) {
				results.push_back(begin);
			}
			else {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] SKIPPED SPECIAL FILENAME [%s]\n",__FILE__,__FUNCTION__,__LINE__,begin);
			}
		}

		globfree(&globbuf);

		if(results.size() == 0 && errorOnNotFound == true) {
			throw runtime_error("No files found in: " + mypath);
		}

		if(cutExtension) {
			for (size_t i=0; i<results.size(); ++i){
				results.at(i)=cutLastExt(results.at(i));
			}
		}
	}
}

bool isdir(const char *path)
{
  string friendly_path = path;

#ifdef WIN32
  replaceAll(friendly_path, "/", "\\");
  if(EndsWith(friendly_path, "\\") == true) {
	  friendly_path.erase(friendly_path.begin() + friendly_path.length() -1);
  }
#endif

#ifdef WIN32
  #if defined(__MINGW32__)
  struct _stat stats;
  #else
  struct _stat64i32 stats;
  #endif
  int result = _wstat(utf8_decode(friendly_path).c_str(), &stats);
#else
  struct stat stats;
  int result = stat(friendly_path.c_str(), &stats);
#endif
  bool ret = (result == 0);
  if(ret == true) {
	  ret = S_ISDIR(stats.st_mode); // #define S_ISDIR(mode) ((mode) & _S_IFDIR)

	  if(ret == false) {
		//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] path [%s] ret = %d\n",__FILE__,__FUNCTION__,__LINE__,friendly_path.c_str(),ret);
	  }
  }
  else {
	  //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] path [%s] ret = %d, result = %d, errno = %d\n",__FILE__,__FUNCTION__,__LINE__,friendly_path.c_str(),ret,result,errno);
  }
  //if(ret == false) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] NOT a path [%s]\n",__FILE__,__FUNCTION__,__LINE__,path);

  return ret;
}

void removeFolder(const string path) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] path [%s]\n",__FILE__,__FUNCTION__,__LINE__,path.c_str());

    string deletePath = path;
    endPathWithSlash(deletePath);
    deletePath += "{,.}*";
    vector<string> results = getFolderTreeContentsListRecursively(deletePath, "", true);
    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] path [%s] results.size() = %d\n",__FILE__,__FUNCTION__,__LINE__,path.c_str(),results.size());

    // First delete files
    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] DELETE FILES\n",__FILE__,__FUNCTION__,__LINE__);

    for(int i = results.size() -1; i >= 0; --i) {
        string item = results[i];

        //if(item.find(".svn") != string::npos) {
        //	printf("!!!!!!!!!!!!!!!!!! FOUND SPECIAL FOLDER [%s] in [%s]\n",item.c_str(),path.c_str());
        //}

        if(isdir(item.c_str()) == false) {
        	bool result = removeFile(item);
        	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] fileitem [%s] result = %d\n",__FILE__,__FUNCTION__,__LINE__,item.c_str(),result);
        }
    }
    // Now delete folders
    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] DELETE FOLDERS\n",__FILE__,__FUNCTION__,__LINE__);

    for(int i = results.size() -1; i >= 0; --i) {
        string item = results[i];
        if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] item [%s] isdir(item.c_str()) = %d\n",__FILE__,__FUNCTION__,__LINE__,item.c_str(), isdir(item.c_str()));
        if(isdir(item.c_str()) == true) {

        	//printf("~~~~~ REMOVE FOLDER [%s] in [%s]\n",item.c_str(),path.c_str());

#ifdef WIN32
            //int result = _rmdir(item.c_str());
			int result = _wrmdir(utf8_decode(item).c_str());
#else
            int result = rmdir(item.c_str());
#endif
            if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] item [%s] result = %d\n",__FILE__,__FUNCTION__,__LINE__,item.c_str(),result);

            if(result != 0 && item != path) {
            	printf("CANNOT REMOVE FOLDER [%s] in [%s]\n",item.c_str(),path.c_str());
            	removeFolder(item);
            }
        }
    }

#ifdef WIN32
    //int result = _rmdir(path.c_str());
	int result = _wrmdir(utf8_decode(path).c_str());
#else
    int result = rmdir(path.c_str());
#endif

    if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] path [%s] result = %d\n",__FILE__,__FUNCTION__,__LINE__,path.c_str(),result);
}

bool StartsWith(const std::string &str, const std::string &key) {
  return str.find(key) == 0;
}

bool EndsWith(const string &str, const string& key)
{
    bool result = false;
    if (str.length() >= key.length()) {
    	result = (0 == str.compare(max((size_t)0,str.length() - key.length()), key.length(), key));
    }

    return result;
}

void endPathWithSlash(string &path,bool requireOSSlash) {
	if(EndsWith(path, "/") == false && EndsWith(path, "\\") == false) {
		string seperator = "/";
		if(requireOSSlash == true) {
#if defined(WIN32)
			seperator = "\\";
#endif
		}
		path += seperator;
	}
}

void trimPathWithStartingSlash(string &path) {
	if(StartsWith(path, "/") == true || StartsWith(path, "\\") == true) {
		path.erase(path.begin(),path.begin()+1);
		//printf("************* trimPathWithStartingSlash changed path [%s]\n",path.c_str());
	}
}

void updatePathClimbingParts(string &path) {
	// Update paths with ..
	string::size_type pos = path.find("..");
	if(pos != string::npos && pos != 0) {
		string orig = path;
		path.erase(pos,2);
		pos--;
		if(path[pos] == '/' || path[pos] == '\\') {
			path.erase(pos,1);
		}

		for(int x = pos; x >= 0; --x) {
			//printf("x [%d][%c] pos [%ld][%c] [%s]\n",x,path[x],(long int)pos,path[pos],path.substr(0,x+1).c_str());

			if((path[x] == '/' || path[x] == '\\') && x != pos) {
				path.erase(x,pos-x);
				break;
			}
		}
		pos = path.find("..");
		if(pos != string::npos && pos != 0) {
			updatePathClimbingParts(path);
		}

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("CHANGED relative path from [%s] to [%s]\n",orig.c_str(),path.c_str());
	}


/*
	string::size_type pos = path.rfind("..");
	if(pos != string::npos && pos != 0) {
		string orig = path;
		path.erase(pos,2);
		pos--;
		if(path[pos] == '/' || path[pos] == '\\') {
			path.erase(pos,1);
		}

		for(int x = pos; x >= 0; --x) {
			//printf("x [%d][%c] pos [%ld][%c] [%s]\n",x,path[x],(long int)pos,path[pos],path.substr(0,x+1).c_str());

			if((path[x] == '/' || path[x] == '\\') && x != pos) {
				path.erase(x,pos-x);
				break;
			}
		}

		printf("CHANGED relative path from [%s] to [%s]\n",orig.c_str(),path.c_str());
	}
*/
}

string getCRCCacheFilePath() {
	return crcCachePath;
}

void setCRCCacheFilePath(string path) {
	crcCachePath = path;
}

string getFormattedCRCCacheFileName(std::pair<string,string> cacheKeys) {
	string crcCacheFile = cacheKeys.first + cacheKeys.second;
	replaceAll(crcCacheFile, "/", "_");
	replaceAll(crcCacheFile, "\\", "_");
	replaceAll(crcCacheFile, "*", "_");
	replaceAll(crcCacheFile, ".", "_");
	return getCRCCacheFilePath() + crcCacheFile;
}
std::pair<string,string> getFolderTreeContentsCheckSumCacheKey(vector<string> paths, string pathSearchString, const string filterFileExt) {
	string cacheLookupId =  CacheManager::getFolderTreeContentsCheckSumRecursivelyCacheLookupKey1;

	string cacheKey = "";
	for(unsigned int idx = 0; idx < paths.size(); ++idx) {
		string path = paths[idx] + pathSearchString;
		cacheKey += path + "_" + filterFileExt + "_";
	}
	return make_pair(cacheLookupId,cacheKey);
}

pair<bool,time_t> hasCachedFileCRCValue(string crcCacheFile, int32 &value) {
	//bool result = false;
	pair<bool,time_t> result = make_pair(false,0);
	if(fileExists(crcCacheFile) == true) {
#ifdef WIN32
		FILE *fp = _wfopen(utf8_decode(crcCacheFile).c_str(), L"r");
#else
		FILE *fp = fopen(crcCacheFile.c_str(),"r");
#endif
		if(fp != NULL) {
			time_t refreshDate = 0;
			int32 crcValue = 0;
			time_t lastUpdateDate = 0;
			int res = fscanf(fp,"%ld,%d,%ld",&refreshDate,&crcValue,&lastUpdateDate);
			fclose(fp);

			result.second = lastUpdateDate;
			if(	refreshDate > 0 &&
				time(NULL) < refreshDate) {

				result.first = true;
				value = crcValue;
			}
			else {
				time_t now = time(NULL);
		        struct tm *loctime = localtime (&now);
		        char szBuf1[100]="";
		        strftime(szBuf1,100,"%Y-%m-%d %H:%M:%S",loctime);

		        loctime = localtime (&refreshDate);
		        char szBuf2[100]="";
		        strftime(szBuf2,100,"%Y-%m-%d %H:%M:%S",loctime);

				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"=-=-=-=- NEED TO CALCULATE CRC for Cache file [%s] now = %ld [%s], refreshDate = %ld [%s], crcValue = %d\n",crcCacheFile.c_str(),now, szBuf1, refreshDate, szBuf2, crcValue);
			}
		}
	}

	return result;
}

void writeCachedFileCRCValue(string crcCacheFile, int32 &crcValue) {
#ifdef WIN32
		FILE *fp = _wfopen(utf8_decode(crcCacheFile).c_str(), L"w");
#else
	FILE *fp = fopen(crcCacheFile.c_str(),"w");
#endif
	if(fp != NULL) {
		//RandomGen random;
		//int offset = random.randRange(5, 15);
	    srand((unsigned long)time(NULL) + (unsigned long)crcCacheFile.length());
	    int offset = rand() % 15;
	    if(offset == 0) {
	    	offset = 3;
	    }
	    time_t now = time(NULL);
		time_t refreshDate = now + (REFRESH_CRC_DAY_SECONDS * offset);

        struct tm *loctime = localtime (&refreshDate);
        char szBuf1[100]="";
        strftime(szBuf1,100,"%Y-%m-%d %H:%M:%S",loctime);

		fprintf(fp,"%ld,%d,%ld",refreshDate,crcValue,now);
		fclose(fp);

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"========== Writing CRC Cache offset [%d] refreshDate = %ld [%s], crcValue = %d, file [%s]\n",offset,refreshDate,szBuf1,crcValue,crcCacheFile.c_str());
	}
}

void clearFolderTreeContentsCheckSum(vector<string> paths, string pathSearchString, const string filterFileExt) {
	std::pair<string,string> cacheKeys = getFolderTreeContentsCheckSumCacheKey(paths, pathSearchString, filterFileExt);
	string cacheLookupId =  cacheKeys.first;
	std::map<string,int32> &crcTreeCache = CacheManager::getCachedItem< std::map<string,int32> >(cacheLookupId);

	string cacheKey = cacheKeys.second;
	if(crcTreeCache.find(cacheKey) != crcTreeCache.end()) {
		crcTreeCache.erase(cacheKey);
	}
	for(unsigned int idx = 0; idx < paths.size(); ++idx) {
		string path = paths[idx];
		clearFolderTreeContentsCheckSum(path, filterFileExt);
	}

	string crcCacheFile = getFormattedCRCCacheFileName(cacheKeys);
	if(fileExists(crcCacheFile) == true) {

		bool result = removeFile(crcCacheFile);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] fileitem [%s] result = %d\n",__FILE__,__FUNCTION__,__LINE__,crcCacheFile.c_str(),result);
	}
}

time_t getFolderTreeContentsCheckSumRecursivelyLastGenerated(vector<string> paths, string pathSearchString, const string filterFileExt) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"-------------- In [%s::%s Line: %d] Calculating CRC for [%s] -----------\n",__FILE__,__FUNCTION__,__LINE__,pathSearchString.c_str());

	std::pair<string,string> cacheKeys = getFolderTreeContentsCheckSumCacheKey(paths, pathSearchString, filterFileExt);
	string cacheLookupId =  cacheKeys.first;
	std::map<string,int32> &crcTreeCache = CacheManager::getCachedItem< std::map<string,int32> >(cacheLookupId);

	string cacheKey = cacheKeys.second;
	string crcCacheFile = getFormattedCRCCacheFileName(cacheKeys);
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"-------------- In [%s::%s Line: %d] looking for cached CRC file [%s] for [%s] -----------\n",__FILE__,__FUNCTION__,__LINE__,crcCacheFile.c_str(),pathSearchString.c_str());

	int32 crcValue = 0;
	pair<bool,time_t> crcResult = hasCachedFileCRCValue(crcCacheFile, crcValue);
	if(crcResult.first == true) {
        struct tm *loctime = localtime (&crcResult.second);
        char szBuf1[100]="";
        strftime(szBuf1,100,"%Y-%m-%d %H:%M:%S",loctime);

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] scanning folders found CACHED FILE for cacheKey [%s] last updated [%s]\n",__FILE__,__FUNCTION__,__LINE__,cacheKey.c_str(),szBuf1);
		//if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\n-------------- In [%s::%s Line: %d] scanning folders found CACHED FILE for cacheKey [%s] last updated [%s]\n",__FILE__,__FUNCTION__,__LINE__,cacheKey.c_str(),szBuf1);

		return crcResult.second;
	}
	else {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] scanning folders DID NOT FIND CACHED FILE checksum for cacheKey [%s]\n",__FILE__,__FUNCTION__,cacheKey.c_str());
		//if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\n-------------- In [%s::%s] scanning folders DID NOT FIND CACHED FILE checksum for cacheKey [%s]\n",__FILE__,__FUNCTION__,cacheKey.c_str());
	}

	return 0;
}

//finds all filenames like path and gets their checksum of all files combined
int32 getFolderTreeContentsCheckSumRecursively(vector<string> paths, string pathSearchString, const string filterFileExt, Checksum *recursiveChecksum, bool forceNoCache) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"-------------- In [%s::%s Line: %d] Calculating CRC for [%s] -----------\n",__FILE__,__FUNCTION__,__LINE__,pathSearchString.c_str());

	std::pair<string,string> cacheKeys = getFolderTreeContentsCheckSumCacheKey(paths, pathSearchString, filterFileExt);
	string cacheLookupId =  cacheKeys.first;
	std::map<string,int32> &crcTreeCache = CacheManager::getCachedItem< std::map<string,int32> >(cacheLookupId);

	string cacheKey = cacheKeys.second;
	if(forceNoCache == false && crcTreeCache.find(cacheKey) != crcTreeCache.end()) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] scanning folders found CACHED checksum = %d for cacheKey [%s]\n",__FILE__,__FUNCTION__,__LINE__,crcTreeCache[cacheKey],cacheKey.c_str());
		//if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\n-------------- In [%s::%s Line: %d] scanning folders found CACHED checksum = %d for cacheKey [%s]\n",__FILE__,__FUNCTION__,__LINE__,crcTreeCache[cacheKey],cacheKey.c_str());
		return crcTreeCache[cacheKey];
	}

	string crcCacheFile = getFormattedCRCCacheFileName(cacheKeys);
	//if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Looking for CRC Cache file [%s]\n",crcCacheFile.c_str());

	if(recursiveChecksum == NULL) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"-------------- In [%s::%s Line: %d] looking for cached CRC file [%s] for [%s] forceNoCache = %d -----------\n",__FILE__,__FUNCTION__,__LINE__,crcCacheFile.c_str(),pathSearchString.c_str(),forceNoCache);
	}

	int32 crcValue = 0;
	if(forceNoCache == false && hasCachedFileCRCValue(crcCacheFile, crcValue).first == true) {
		crcTreeCache[cacheKey] = crcValue;
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] scanning folders found CACHED FILE checksum = %d for cacheKey [%s] forceNoCache = %d\n",__FILE__,__FUNCTION__,__LINE__,crcTreeCache[cacheKey],cacheKey.c_str(),forceNoCache);
		if(recursiveChecksum == NULL) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"-------------- In [%s::%s Line: %d] scanning folders found CACHED FILE checksum = %d for cacheKey [%s] forceNoCache = %d\n",__FILE__,__FUNCTION__,__LINE__,crcTreeCache[cacheKey],cacheKey.c_str(),forceNoCache);
		}

		return crcTreeCache[cacheKey];
	}
	else {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] scanning folders DID NOT FIND CACHED FILE checksum for cacheKey [%s] forceNoCache = %d\n",__FILE__,__FUNCTION__,cacheKey.c_str(),forceNoCache);
		if(recursiveChecksum == NULL) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"-------------- In [%s::%s] scanning folders DID NOT FIND CACHED FILE checksum for cacheKey [%s] forceNoCache = %d\n",__FILE__,__FUNCTION__,cacheKey.c_str(),forceNoCache);
		}
	}

	Checksum checksum = (recursiveChecksum == NULL ? Checksum() : *recursiveChecksum);
	for(unsigned int idx = 0; idx < paths.size(); ++idx) {
		string path = paths[idx] + pathSearchString;

		getFolderTreeContentsCheckSumRecursively(path, filterFileExt, &checksum, forceNoCache);
	}

	if(recursiveChecksum != NULL) {
		*recursiveChecksum = checksum;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Final CRC file count: %d\n",__FILE__,__FUNCTION__,__LINE__,checksum.getFileCount());

	int32 result = checksum.getFinalFileListSum();
	//if(forceNoCache == false) {
	crcTreeCache[cacheKey] = result;
	writeCachedFileCRCValue(crcCacheFile, crcTreeCache[cacheKey]);
	//}
	return result;
}

std::pair<string,string> getFolderTreeContentsCheckSumCacheKey(const string &path, const string filterFileExt) {
	string cacheLookupId =  CacheManager::getFolderTreeContentsCheckSumRecursivelyCacheLookupKey2;

	string cacheKey = path + "_" + filterFileExt;
	return make_pair(cacheLookupId,cacheKey);
}

void clearFolderTreeContentsCheckSum(const string &path, const string filterFileExt) {
	std::pair<string,string> cacheKeys = getFolderTreeContentsCheckSumCacheKey(path,filterFileExt);
	string cacheLookupId =  cacheKeys.first;
	std::map<string,int32> &crcTreeCache = CacheManager::getCachedItem< std::map<string,int32> >(cacheLookupId);

	string cacheKey = cacheKeys.second;
	if(crcTreeCache.find(cacheKey) != crcTreeCache.end()) {
		crcTreeCache.erase(cacheKey);
	}
	string crcCacheFile = getFormattedCRCCacheFileName(cacheKeys);
	if(fileExists(crcCacheFile) == true) {
		bool result = removeFile(crcCacheFile.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] fileitem [%s] result = %d\n",__FILE__,__FUNCTION__,__LINE__,crcCacheFile.c_str(),result);
	}
}

//finds all filenames like path and gets their checksum of all files combined
int32 getFolderTreeContentsCheckSumRecursively(const string &path, const string &filterFileExt, Checksum *recursiveChecksum, bool forceNoCache) {
	std::pair<string,string> cacheKeys = getFolderTreeContentsCheckSumCacheKey(path, filterFileExt);

	string cacheLookupId =  cacheKeys.first;
	std::map<string,int32> &crcTreeCache = CacheManager::getCachedItem< std::map<string,int32> >(cacheLookupId);

	string cacheKey = cacheKeys.second;
	if(forceNoCache == false && crcTreeCache.find(cacheKey) != crcTreeCache.end()) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] scanning [%s] found CACHED checksum = %d for cacheKey [%s]\n",__FILE__,__FUNCTION__,__LINE__,path.c_str(),crcTreeCache[cacheKey],cacheKey.c_str());
		//if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\n-------------- In [%s::%s Line: %d] scanning [%s] found CACHED checksum = %d for cacheKey [%s]\n",__FILE__,__FUNCTION__,__LINE__,path.c_str(),crcTreeCache[cacheKey],cacheKey.c_str());
		return crcTreeCache[cacheKey];
	}

	string crcCacheFile = getFormattedCRCCacheFileName(cacheKeys);
	//if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Looking for CRC Cache file [%s]\n",crcCacheFile.c_str());

	if(recursiveChecksum == NULL) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"-------------- In [%s::%s Line: %d] looking for cached CRC file [%s] for [%s] forceNoCache = %d -----------\n",__FILE__,__FUNCTION__,__LINE__,crcCacheFile.c_str(),path.c_str(),forceNoCache);
	}

	int32 crcValue = 0;
	if(forceNoCache == false && hasCachedFileCRCValue(crcCacheFile, crcValue).first == true) {
		crcTreeCache[cacheKey] = crcValue;
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] scanning folders found CACHED FILE checksum = %d for cacheKey [%s] forceNoCache = %d\n",__FILE__,__FUNCTION__,__LINE__,crcTreeCache[cacheKey],cacheKey.c_str(),forceNoCache);
		if(recursiveChecksum == NULL) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"-------------- In [%s::%s Line: %d] scanning folders found CACHED FILE checksum = %d for cacheKey [%s] forceNoCache = %d\n",__FILE__,__FUNCTION__,__LINE__,crcTreeCache[cacheKey],cacheKey.c_str(),forceNoCache);
		}

		return crcTreeCache[cacheKey];
	}
	else {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] scanning folders DID NOT FIND CACHED FILE checksum for cacheKey [%s] forceNoCache = %d\n",__FILE__,__FUNCTION__,cacheKey.c_str(),forceNoCache);
		if(recursiveChecksum == NULL) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"-------------- In [%s::%s] scanning folders DID NOT FIND CACHED FILE checksum for cacheKey [%s] forceNoCache = %d\n",__FILE__,__FUNCTION__,cacheKey.c_str(),forceNoCache);
		}
	}

	bool topLevelCaller = (recursiveChecksum == NULL);
    Checksum checksum = (recursiveChecksum == NULL ? Checksum() : *recursiveChecksum);

	std::string mypath = path;
	// Stupid win32 is searching for all files without extension when *. is
	// specified as wildcard
	//
	if(mypath.size() >= 2 && mypath.compare(max((size_t)0,mypath.size() - 2), 2, "*.") == 0) {
		mypath = mypath.substr(0, max((size_t)0,mypath.size() - 2));
		mypath += "*";
	}

	glob_t globbuf;

	int res = glob(mypath.c_str(), 0, 0, &globbuf);
#if !defined(__APPLE__) && !defined(__FreeBSD__)
	if(res < 0) {
		std::stringstream msg;
		msg << "#2 Couldn't scan directory '" << mypath << "': " << strerror(errno);
		throw runtime_error(msg.str());
	}
#endif

	int fileLoopCount = 0;
	int fileMatchCount = 0;
	for(int i = 0; i < globbuf.gl_pathc; ++i) {
		const char* p = globbuf.gl_pathv[i];
		//printf("Line: %d p [%s]\n",__LINE__,p);

		if(isdir(p) == false) {
            bool addFile = true;
            if(EndsWith(p, ".") == true || EndsWith(p, "..") == true || EndsWith(p, ".svn") == true) {
            	addFile = false;
            }
            else if(filterFileExt != "") {
                addFile = EndsWith(p, filterFileExt);
            }

            if(addFile) {
                checksum.addFile(p);
                fileMatchCount++;
            }
		}
		fileLoopCount++;
	}

	globfree(&globbuf);

    // Look recursively for sub-folders
#if defined(__APPLE__) || defined(__FreeBSD__)
	res = glob(mypath.c_str(), 0, 0, &globbuf);
#else
	res = glob(mypath.c_str(), GLOB_ONLYDIR, 0, &globbuf);
#endif

#if !defined(__APPLE__) && !defined(__FreeBSD__)
	if(res < 0) {
		std::stringstream msg;
		msg << "#3 Couldn't scan directory '" << mypath << "': " << strerror(errno);
		throw runtime_error(msg.str());
	}
#endif

	for(int i = 0; i < globbuf.gl_pathc; ++i) {
#if defined(__APPLE__) || defined(__FreeBSD__)
		struct stat statStruct;
		// only process if dir..
		int actStat = lstat( globbuf.gl_pathv[i], &statStruct);
		if( S_ISDIR(statStruct.st_mode) == 0)
			continue;
#endif
		const char *p = globbuf.gl_pathv[i];
		//printf("Line: %d p [%s]\n",__LINE__,p);

    	string currentPath = p;
    	endPathWithSlash(currentPath);

        getFolderTreeContentsCheckSumRecursively(currentPath + "*", filterFileExt, &checksum, forceNoCache);
	}

	globfree(&globbuf);

	if(recursiveChecksum != NULL) {
		*recursiveChecksum = checksum;
	}

	if(topLevelCaller == true) {
		//printf("In [%s::%s Line: %d] Final CRC file count for [%s]: %d\n",__FILE__,__FUNCTION__,__LINE__,path.c_str(),checksum.getFileCount());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] Final CRC file count for [%s]: %d\n",__FILE__,__FUNCTION__,__LINE__,path.c_str(),checksum.getFileCount());

		int32 result = checksum.getFinalFileListSum();
		//if(forceNoCache == false) {
		crcTreeCache[cacheKey] = result;
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] scanning [%s] ending checksum = %d for cacheKey [%s] fileMatchCount = %d, fileLoopCount = %d\n",__FILE__,__FUNCTION__,path.c_str(),crcTreeCache[cacheKey],cacheKey.c_str(),fileMatchCount,fileLoopCount);
		writeCachedFileCRCValue(crcCacheFile, crcTreeCache[cacheKey]);
		//}

		return result;
	}
	else {
		return 0;
	}
}


std::pair<string,string> getFolderTreeContentsCheckSumListCacheKey(vector<string> paths, string pathSearchString, const string filterFileExt) {
	string cacheLookupId =  CacheManager::getFolderTreeContentsCheckSumListRecursivelyCacheLookupKey1;

	string cacheKey = "";
	for(unsigned int idx = 0; idx < paths.size(); ++idx) {
		string path = paths[idx] + pathSearchString;
		cacheKey += path + "_" + filterFileExt + "_";
	}
	return make_pair(cacheLookupId,cacheKey);
}

void clearFolderTreeContentsCheckSumList(vector<string> paths, string pathSearchString, const string filterFileExt) {
	std::pair<string,string> cacheKeys = getFolderTreeContentsCheckSumListCacheKey(paths, pathSearchString, filterFileExt);
	string cacheLookupId =  cacheKeys.first;
	std::map<string,vector<std::pair<string,int32> > > &crcTreeCache = CacheManager::getCachedItem< std::map<string,vector<std::pair<string,int32> > > >(cacheLookupId);

	string cacheKey = cacheKeys.second;
	if(crcTreeCache.find(cacheKey) != crcTreeCache.end()) {
		crcTreeCache.erase(cacheKey);
	}
	for(unsigned int idx = 0; idx < paths.size(); ++idx) {
		string path = paths[idx];
		clearFolderTreeContentsCheckSumList(path, filterFileExt);
	}
	string crcCacheFile = getFormattedCRCCacheFileName(cacheKeys);
	if(fileExists(crcCacheFile) == true) {
		bool result = removeFile(crcCacheFile.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] fileitem [%s] result = %d\n",__FILE__,__FUNCTION__,__LINE__,crcCacheFile.c_str(),result);
	}
}

vector<std::pair<string,int32> > getFolderTreeContentsCheckSumListRecursively(vector<string> paths, string pathSearchString, string filterFileExt, vector<std::pair<string,int32> > *recursiveMap) {
	std::pair<string,string> cacheKeys = getFolderTreeContentsCheckSumListCacheKey(paths, pathSearchString, filterFileExt);
	string cacheLookupId =  cacheKeys.first;
	std::map<string,vector<std::pair<string,int32> > > &crcTreeCache = CacheManager::getCachedItem< std::map<string,vector<std::pair<string,int32> > > >(cacheLookupId);

	string cacheKey = cacheKeys.second;
	if(crcTreeCache.find(cacheKey) != crcTreeCache.end()) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] scanning folders found CACHED result for cacheKey [%s]\n",__FILE__,__FUNCTION__,__LINE__,cacheKey.c_str());
		return crcTreeCache[cacheKey];
	}
	else {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] scanning folders, NO CACHE found result for cacheKey [%s]\n",__FILE__,__FUNCTION__,__LINE__,cacheKey.c_str());
	}

	bool topLevelCaller = (recursiveMap == NULL);

	vector<std::pair<string,int32> > checksumFiles = (recursiveMap == NULL ? vector<std::pair<string,int32> >() : *recursiveMap);
	for(unsigned int idx = 0; idx < paths.size(); ++idx) {
		string path = paths[idx] + pathSearchString;
		checksumFiles = getFolderTreeContentsCheckSumListRecursively(path, filterFileExt, &checksumFiles);
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] checksumFiles.size() = %d\n",__FILE__,__FUNCTION__,__LINE__,checksumFiles.size());

	if(topLevelCaller == true) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] EXITING TOP LEVEL RECURSION, checksumFiles.size() = %d\n",__FILE__,__FUNCTION__,__LINE__,checksumFiles.size());
	}

	crcTreeCache[cacheKey] = checksumFiles;
	return crcTreeCache[cacheKey];
}

//finds all filenames like path and gets the checksum of each file
vector<string> getFolderTreeContentsListRecursively(const string &path, const string &filterFileExt, bool includeFolders, vector<string> *recursiveMap) {
	bool topLevelCaller = (recursiveMap == NULL);
    vector<string> resultFiles = (recursiveMap == NULL ? vector<string>() : *recursiveMap);

	std::string mypath = path;
	/** Stupid win32 is searching for all files without extension when *. is
	 * specified as wildcard
	 */
	if(mypath.size() >= 2 && mypath.compare(max((size_t)0,mypath.size() - 2), 2, "*.") == 0) {
		mypath = mypath.substr(0, max((size_t)0,mypath.size() - 2));
		mypath += "*";
	}

	glob_t globbuf;

	int globFlags = 0;
	if(EndsWith(mypath,"{,.}*") == true) {
#ifndef WIN32
		globFlags = GLOB_BRACE;
#else
		// Windows glob source cannot handle GLOB_BRACE
		// but that should be ok for win32 platform
		replaceAll(mypath,"{,.}*","*");
#endif
	}

	int res = glob(mypath.c_str(), globFlags, 0, &globbuf);
#if !defined(__APPLE__) && !defined(__FreeBSD__)
	if(res < 0) {
		std::stringstream msg;
		msg << "#4 Couldn't scan directory '" << mypath << "': " << strerror(errno);
		throw runtime_error(msg.str());
	}
#endif
	for(int i = 0; i < globbuf.gl_pathc; ++i) {
		const char* p = globbuf.gl_pathv[i];

		bool skipItem = (EndsWith(p, ".") == true || EndsWith(p, "..") == true);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"~~~~~~~~~~~ Glob file [%s] skipItem = %d\n",p,skipItem);

		if(skipItem == false) {
			if(isdir(p) == false) {
				bool addFile = true;
				if(filterFileExt != "") {
					addFile = EndsWith(p, filterFileExt);
				}

				if(addFile) {
					resultFiles.push_back(p);
				}
			}
			else if(includeFolders == true) {
				resultFiles.push_back(p);
			}
		}
	}

	globfree(&globbuf);

    // Look recursively for sub-folders
#if defined(__APPLE__) || defined(__FreeBSD__)
	res = glob(mypath.c_str(), 0, 0, &globbuf);
#else //APPLE doesn't have the GLOB_ONLYDIR definition..
	globFlags |= GLOB_ONLYDIR;
	res = glob(mypath.c_str(), globFlags, 0, &globbuf);
#endif

#if !defined(__APPLE__) && !defined(__FreeBSD__)
	if(res < 0) {
		std::stringstream msg;
		msg << "#5 Couldn't scan directory '" << mypath << "': " << strerror(errno);
		throw runtime_error(msg.str());
	}
#endif

	for(int i = 0; i < globbuf.gl_pathc; ++i) {
#if defined(__APPLE__) || defined(__FreeBSD__)
		struct stat statStruct;
		// only get if dir..
		int actStat = lstat( globbuf.gl_pathv[ i], &statStruct);
		if( S_ISDIR(statStruct.st_mode) == 0)
			continue;
#endif
		const char* p = globbuf.gl_pathv[i];

		bool skipItem = (EndsWith(p, ".") == true || EndsWith(p, "..") == true);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"~~~~~~~~~~~ Glob folder [%s] skipItem = %d\n",p,skipItem);

		if(skipItem == false) {
			if(includeFolders == true) {
				resultFiles.push_back(p);
			}

			string currentPath = p;
			endPathWithSlash(currentPath);

			if(EndsWith(mypath,"{,.}*") == true) {
				currentPath += "{,.}*";
			}
			else {
				currentPath += "*";
			}
			resultFiles = getFolderTreeContentsListRecursively(currentPath, filterFileExt, includeFolders,&resultFiles);
		}
	}

	globfree(&globbuf);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] scanning [%s]\n",__FILE__,__FUNCTION__,path.c_str());

	if(topLevelCaller == true) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] EXITING TOP LEVEL RECURSION\n",__FILE__,__FUNCTION__,__LINE__);
	}

    return resultFiles;
}

std::pair<string,string> getFolderTreeContentsCheckSumListCacheKey(const string &path, const string filterFileExt) {
	string cacheLookupId =  CacheManager::getFolderTreeContentsCheckSumListRecursivelyCacheLookupKey2;

	string cacheKey = path + "_" + filterFileExt;
	return make_pair(cacheLookupId,cacheKey);
}

void clearFolderTreeContentsCheckSumList(const string &path, const string filterFileExt) {
	std::pair<string,string> cacheKeys = getFolderTreeContentsCheckSumListCacheKey(path,filterFileExt);
	string cacheLookupId =  cacheKeys.first;
	std::map<string,vector<std::pair<string,int32> > > &crcTreeCache = CacheManager::getCachedItem< std::map<string,vector<std::pair<string,int32> > > >(cacheLookupId);

	string cacheKey = cacheKeys.second;
	if(crcTreeCache.find(cacheKey) != crcTreeCache.end()) {
		crcTreeCache.erase(cacheKey);
	}
	string crcCacheFile = getFormattedCRCCacheFileName(cacheKeys);
	if(fileExists(crcCacheFile) == true) {
		bool result = removeFile(crcCacheFile.c_str());
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] fileitem [%s] result = %d\n",__FILE__,__FUNCTION__,__LINE__,crcCacheFile.c_str(),result);
	}
}

//finds all filenames like path and gets the checksum of each file
vector<std::pair<string,int32> > getFolderTreeContentsCheckSumListRecursively(const string &path, const string &filterFileExt, vector<std::pair<string,int32> > *recursiveMap) {
	std::pair<string,string> cacheKeys = getFolderTreeContentsCheckSumListCacheKey(path, filterFileExt);
	string cacheLookupId =  cacheKeys.first;
	std::map<string,vector<std::pair<string,int32> > > &crcTreeCache = CacheManager::getCachedItem< std::map<string,vector<std::pair<string,int32> > > >(cacheLookupId);

	string cacheKey = cacheKeys.second;
	if(crcTreeCache.find(cacheKey) != crcTreeCache.end()) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] scanning [%s] FOUND CACHED result for cacheKey [%s]\n",__FILE__,__FUNCTION__,path.c_str(),cacheKey.c_str());
		return crcTreeCache[cacheKey];
	}

	bool topLevelCaller = (recursiveMap == NULL);
    vector<std::pair<string,int32> > checksumFiles = (recursiveMap == NULL ? vector<std::pair<string,int32> >() : *recursiveMap);

	std::string mypath = path;
	/** Stupid win32 is searching for all files without extension when *. is
	 * specified as wildcard
	 */
	if(mypath.size() >= 2 && mypath.compare(max((size_t)0,mypath.size() - 2), 2, "*.") == 0) {
		mypath = mypath.substr(0, max((size_t)0,mypath.size() - 2));
		mypath += "*";
	}

	glob_t globbuf;

	int res = glob(mypath.c_str(), 0, 0, &globbuf);

#if !defined(__APPLE__) && !defined(__FreeBSD__)
	if(res < 0) {
		std::stringstream msg;
		msg << "#6 Couldn't scan directory '" << mypath << "': " << strerror(errno);
		throw runtime_error(msg.str());
	}
#endif

	for(int i = 0; i < globbuf.gl_pathc; ++i) {
		const char* p = globbuf.gl_pathv[i];

		if(isdir(p) == false) {
            bool addFile = true;
            if(EndsWith(p, ".") == true || EndsWith(p, "..") == true || EndsWith(p, ".svn") == true) {
            	addFile = false;
            }
            else if(filterFileExt != "") {
                addFile = EndsWith(p, filterFileExt);
            }

            if(addFile) {
                Checksum checksum;
                checksum.addFile(p);

                checksumFiles.push_back(std::pair<string,int32>(p,checksum.getSum()));
            }
		}
	}

	globfree(&globbuf);

    // Look recursively for sub-folders
#if defined(__APPLE__) || defined(__FreeBSD__)
	res = glob(mypath.c_str(), 0, 0, &globbuf);
#else //APPLE doesn't have the GLOB_ONLYDIR definition..
	res = glob(mypath.c_str(), GLOB_ONLYDIR, 0, &globbuf);
#endif

#if !defined(__APPLE__) && !defined(__FreeBSD__)
	if(res < 0) {
		std::stringstream msg;
		msg << "#7 Couldn't scan directory '" << mypath << "': " << strerror(errno);
		throw runtime_error(msg.str());
	}
#endif

	for(int i = 0; i < globbuf.gl_pathc; ++i) {
#if defined(__APPLE__) || defined(__FreeBSD__)
		struct stat statStruct;
		// only get if dir..
		int actStat = lstat( globbuf.gl_pathv[ i], &statStruct);
		if( S_ISDIR(statStruct.st_mode) == 0)
			continue;
#endif
		const char *p = globbuf.gl_pathv[i];

    	string currentPath = p;
    	endPathWithSlash(currentPath);

        checksumFiles = getFolderTreeContentsCheckSumListRecursively(currentPath + "*", filterFileExt, &checksumFiles);
	}

	globfree(&globbuf);

	crcTreeCache[cacheKey] = checksumFiles;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] scanning [%s] cacheKey [%s] checksumFiles.size() = %d\n",__FILE__,__FUNCTION__,path.c_str(),cacheKey.c_str(),checksumFiles.size());

	if(topLevelCaller == true) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] EXITING TOP LEVEL RECURSION, checksumFiles.size() = %d\n",__FILE__,__FUNCTION__,__LINE__,checksumFiles.size());
	}

    return crcTreeCache[cacheKey];
}

string extractFileFromDirectoryPath(string filename) {
	size_t lastDirectory     = filename.find_last_of("/\\");
	if (lastDirectory == string::npos) {
		return filename;
	}

	return filename.erase( 0, lastDirectory + 1);
}

string extractDirectoryPathFromFile(string filename) {
	size_t lastDirectory     = filename.find_last_of("/\\");
	string path = "";
	if (lastDirectory != string::npos) {
		path = filename.substr( 0, lastDirectory + 1);
	}

	return path;
}

string extractLastDirectoryFromPath(string Path) {
	string result = Path;
	size_t lastDirectory     = Path.find_last_of("/\\");
	if (lastDirectory == string::npos) {
		result = Path;
	}
	else {
		if(Path.length() > lastDirectory + 1) {
			result = Path.erase( 0, lastDirectory + 1);
		}
		else {
			for(int i = lastDirectory-1; i >= 0; --i) {
				if(Path[i] == '/' || Path[i] == '\\' && i > 0) {
					result = Path.erase( 0, i);
					break;
				}
			}
		}
	}
	return result;
}

string extractExtension(const string& filepath) {
	size_t lastPoint 		= filepath.find_last_of('.');
	size_t lastDirectory    = filepath.find_last_of("/\\");

	if (lastPoint == string::npos || (lastDirectory != string::npos && lastDirectory > lastPoint)) {
		return "";
	}
	return filepath.substr(lastPoint+1);
}

void createDirectoryPaths(string Path) {
 char DirName[256]="";
 const char *path = Path.c_str();
 char *dirName = DirName;
 while(*path) {
   //if (('\\' == *path) || ('/' == *path))
   if ('/' == *path) {
	   if(isdir(DirName) == false) {
#ifdef WIN32
		   int result = _wmkdir(utf8_decode(DirName).c_str());
		   //int result = _mkdir(DirName);
#elif defined(__GNUC__)
		   int result = mkdir(DirName, S_IRWXU | S_IRWXO | S_IRWXG);
#else
		   #error "Your compiler needs to support mkdir!"
#endif
        if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] DirName [%s] result = %d, errno = %d\n",__FILE__,__FUNCTION__,__LINE__,DirName,result,errno);
	   }
   }
   *dirName++ = *path++;
   *dirName = '\0';
 }
#ifdef WIN32
 //int result = _mkdir(DirName);
 int result = _wmkdir(utf8_decode(DirName).c_str());
#elif defined(__GNUC__)
 int result = mkdir(DirName, S_IRWXU | S_IRWXO | S_IRWXG);
#else
	#error "Your compiler needs to support mkdir!"
#endif
 if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] DirName [%s] result = %d, errno = %d\n",__FILE__,__FUNCTION__,__LINE__,DirName,result,errno);
}

void getFullscreenVideoInfo(int &colorBits,int &screenWidth,int &screenHeight,bool isFullscreen) {
    // Get the current video hardware information
    //const SDL_VideoInfo* vidInfo = SDL_GetVideoInfo();
    //colorBits      = vidInfo->vfmt->BitsPerPixel;
    //screenWidth    = vidInfo->current_w;
    //screenHeight   = vidInfo->current_h;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    /* Get available fullscreen/hardware modes */
    int flags = SDL_RESIZABLE;
    #if defined(WIN32) || defined(__APPLE__)
    flags = 0;
    #endif
    if(isFullscreen) flags = SDL_FULLSCREEN;
    SDL_Rect**modes = SDL_ListModes(NULL, SDL_OPENGL|flags);

    /* Check if there are any modes available */
    if (modes == (SDL_Rect**)0) {
    	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] no hardware modes available.\n",__FILE__,__FUNCTION__,__LINE__);

       const SDL_VideoInfo* vidInfo = SDL_GetVideoInfo();
       colorBits      = vidInfo->vfmt->BitsPerPixel;
       screenWidth    = vidInfo->current_w;
       screenHeight   = vidInfo->current_h;

       if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] using current resolution: %d x %d.\n",__FILE__,__FUNCTION__,__LINE__,screenWidth,screenHeight);
   }
   /* Check if our resolution is restricted */
   else if (modes == (SDL_Rect**)-1) {
	   if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] all resolutions available.\n",__FILE__,__FUNCTION__,__LINE__);

       const SDL_VideoInfo* vidInfo = SDL_GetVideoInfo();
       colorBits      = vidInfo->vfmt->BitsPerPixel;
       screenWidth    = vidInfo->current_w;
       screenHeight   = vidInfo->current_h;

       if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] using current resolution: %d x %d.\n",__FILE__,__FUNCTION__,__LINE__,screenWidth,screenHeight);
   }
   else{
       /* Print valid modes */
	   if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] available Modes are:\n",__FILE__,__FUNCTION__,__LINE__);

       int bestW = -1;
       int bestH = -1;
       for(int i=0; modes[i]; ++i) {
    	   if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%d x %d\n",modes[i]->w, modes[i]->h);

           if(bestW < modes[i]->w) {
               bestW = modes[i]->w;
               bestH = modes[i]->h;
           }
       }

       if(bestW > screenWidth) {
           screenWidth = bestW;
           screenHeight = bestH;
       }

       if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] using current resolution: %d x %d.\n",__FILE__,__FUNCTION__,__LINE__,screenWidth,screenHeight);
  }
}


void getFullscreenVideoModes(vector<ModeInfo> *modeinfos, bool isFullscreen) {
    // Get the current video hardware information
    //const SDL_VideoInfo* vidInfo = SDL_GetVideoInfo();
    //colorBits      = vidInfo->vfmt->BitsPerPixel;
    //screenWidth    = vidInfo->current_w;
    //screenHeight   = vidInfo->current_h;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    SDL_PixelFormat format;
  	//SDL_Rect **modes;
  	int loops(0);
	int bpp(0);
	std::map<std::string,bool> uniqueResList;

	do
	{
			//format.BitsPerPixel seems to get zeroed out on my windows box
			switch(loops)
			{
				case 0://32 bpp
					format.BitsPerPixel = 32;
					bpp = 32;
					break;
				case 1://24 bpp
					format.BitsPerPixel = 24;
					bpp = 24;
					break;
				case 2://16 bpp
					format.BitsPerPixel = 16;
					bpp = 16;
					break;
				case 3://8 bpp
					format.BitsPerPixel = 8;
					bpp = 8;
					break;
			}

			/* Get available fullscreen/hardware modes */
			//SDL_Rect**modes = SDL_ListModes(NULL, SDL_OPENGL|SDL_RESIZABLE);

		    int flags = SDL_RESIZABLE;
		    #if defined(WIN32) || defined(__APPLE__)
		    flags = 0;
		    #endif
		    if(isFullscreen) flags = SDL_FULLSCREEN;
			SDL_Rect**modes = SDL_ListModes(&format, SDL_OPENGL|flags);

			/* Check if there are any modes available */
			if (modes == (SDL_Rect**)0) {
				//printf("NO resolutions are usable for format = %d\n",format.BitsPerPixel);

				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] no hardware modes available.\n",__FILE__,__FUNCTION__,__LINE__);

			   const SDL_VideoInfo* vidInfo = SDL_GetVideoInfo();
  		       string lookupKey = intToStr(vidInfo->current_w) + "_" + intToStr(vidInfo->current_h) + "_" + intToStr(vidInfo->vfmt->BitsPerPixel);
		       if(uniqueResList.find(lookupKey) == uniqueResList.end()) {
		    	   uniqueResList[lookupKey] = true;
		    	   modeinfos->push_back(ModeInfo(vidInfo->current_w,vidInfo->current_h,vidInfo->vfmt->BitsPerPixel));
		       }
		       if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] adding only current resolution: %d x %d - %d.\n",__FILE__,__FUNCTION__,__LINE__,vidInfo->current_w,vidInfo->current_h,vidInfo->vfmt->BitsPerPixel);
		   }
		   /* Check if our resolution is restricted */
		   else if (modes == (SDL_Rect**)-1) {
			   //printf("ALL resolutions are usable for format = %d\n",format.BitsPerPixel);
			   if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] all resolutions available.\n",__FILE__,__FUNCTION__,__LINE__);

			   const SDL_VideoInfo* vidInfo = SDL_GetVideoInfo();
  		       string lookupKey = intToStr(vidInfo->current_w) + "_" + intToStr(vidInfo->current_h) + "_" + intToStr(vidInfo->vfmt->BitsPerPixel);
		       if(uniqueResList.find(lookupKey) == uniqueResList.end()) {
		    	   uniqueResList[lookupKey] = true;
		    	   modeinfos->push_back(ModeInfo(vidInfo->current_w,vidInfo->current_h,vidInfo->vfmt->BitsPerPixel));
		       }
		       if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] adding only current resolution: %d x %d - %d.\n",__FILE__,__FUNCTION__,__LINE__,vidInfo->current_w,vidInfo->current_h,vidInfo->vfmt->BitsPerPixel);

		       // Lets add these resolutions since sdl said all are supported
		       /*
		       240x160	 4:3		 GameboyAdvanced
		       256x192	 4:3		 NintendoDS
		       320x240	 4:3		 GP2x
		       480x272	 ~16:9		 PSP
		       480x320	 3:2		 iPhone
		       640x480	 4:3		 numerous PDA use this resolution
		       800x480	 5:3		 ASUS Eee PC, Nokia N800
		       1024x600	 ~16:9		 ASUS Eee PC 1000
		       1024x768	 4:3		 common LCD format
		       1200x900	 4:3		 OLPC
		       1280x1024	 5:4		 common LCD format
		       1440x900	 16:10		 common LCD format
		       1680x1050	 16:10		 common LCD format
		       1600x1200	 4:3		 common LCD format
		       1366x768	 ~16:9		 common resolution of HD-TVs, even so its not actually an official HD-TV resolution
		       1368x768	 ~16:9		 common resolution of HD-TVs, even so its not actually an official HD-TV resolution
		       1920x1200	 16:10		 common LCD format
		       2560x1600	 16:10		 30" LCD
		       1280x720	 16:9		 HD-TV (720p)
		       1920x1080	 16:9		 HD-TV (1080p)
		       2560x1440	 16:9		 Apple iMac
		       2560x1600	 16:10		 Largest Available Consumer Monitor
		       */

		       vector<pair<int,int> > allResoltuions;
		       if(SDL_VideoModeOK(640, 480, bpp, SDL_OPENGL|flags) == bpp) {
		    	   allResoltuions.push_back(make_pair(640, 480));
		       }
		       if(SDL_VideoModeOK(800, 480, bpp, SDL_OPENGL|flags) == bpp) {
		    	   allResoltuions.push_back(make_pair(800, 480));
		       }
		       if(SDL_VideoModeOK(800, 600, bpp, SDL_OPENGL|flags) == bpp) {
		    	   allResoltuions.push_back(make_pair(800, 600));
		       }
		       if(SDL_VideoModeOK(1024, 600, bpp, SDL_OPENGL|flags) == bpp) {
		    	   allResoltuions.push_back(make_pair(1024, 600));
		       }
		       if(SDL_VideoModeOK(1024, 768, bpp, SDL_OPENGL|flags) == bpp) {
		    	   allResoltuions.push_back(make_pair(1024, 768));
		       }
		       if(SDL_VideoModeOK(1280, 720, bpp, SDL_OPENGL|flags) == bpp) {
		    	   allResoltuions.push_back(make_pair(1280, 720));
		       }
		       if(SDL_VideoModeOK(1200, 900, bpp, SDL_OPENGL|flags) == bpp) {
		    	   allResoltuions.push_back(make_pair(1200, 900));
		       }
		       if(SDL_VideoModeOK(1280, 1024, bpp, SDL_OPENGL|flags) == bpp) {
		    	   allResoltuions.push_back(make_pair(1280, 1024));
		       }
		       if(SDL_VideoModeOK(1440, 900, bpp, SDL_OPENGL|flags) == bpp) {
		    	   allResoltuions.push_back(make_pair(1440, 900));
		       }
		       if(SDL_VideoModeOK(1680, 1050, bpp, SDL_OPENGL|flags) == bpp) {
		    	   allResoltuions.push_back(make_pair(1680, 1050));
		       }
		       if(SDL_VideoModeOK(1600, 1200, bpp, SDL_OPENGL|flags) == bpp) {
		    	   allResoltuions.push_back(make_pair(1600, 1200));
		       }
		       if(SDL_VideoModeOK(1366, 768, bpp, SDL_OPENGL|flags) == bpp) {
		    	   allResoltuions.push_back(make_pair(1366, 768));
		       }
		       if(SDL_VideoModeOK(1920, 1080, bpp, SDL_OPENGL|flags) == bpp) {
		    	   allResoltuions.push_back(make_pair(1920, 1080));
		       }
		       if(SDL_VideoModeOK(1920, 1200, bpp, SDL_OPENGL|flags) == bpp) {
		    	   allResoltuions.push_back(make_pair(1920, 1200));
		       }
		       if(SDL_VideoModeOK(2560, 1600, bpp, SDL_OPENGL|flags) == bpp) {
		    	   allResoltuions.push_back(make_pair(2560, 1600));
		       }
		       if(SDL_VideoModeOK(2560, 1440, bpp, SDL_OPENGL|flags) == bpp) {
		    	   allResoltuions.push_back(make_pair(2560, 1440));
		       }

			   for(unsigned int i=0; i < allResoltuions.size(); ++i) {
				   if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%d x %d\n",allResoltuions[i].first, allResoltuions[i].second,bpp);
				    string lookupKey = intToStr(allResoltuions[i].first) + "_" + intToStr(allResoltuions[i].second) + "_" + intToStr(bpp);
				    if(uniqueResList.find(lookupKey) == uniqueResList.end()) {
				    	uniqueResList[lookupKey] = true;
				    	modeinfos->push_back(ModeInfo(allResoltuions[i].first, allResoltuions[i].second,bpp));
				    	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] adding resolution: %d x %d - %d.\n",__FILE__,__FUNCTION__,__LINE__,allResoltuions[i].first, allResoltuions[i].second,bpp);
				    }
			   }
		   }
		   else {
			   //printf("SOME resolutions are usable for format = %d\n",format.BitsPerPixel);

			   /* Print valid modes */
			   if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] available Modes are:\n",__FILE__,__FUNCTION__,__LINE__);

			   for(int i=0; modes[i]; ++i) {
				   if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%d x %d\n",modes[i]->w, modes[i]->h,bpp);
				    string lookupKey = intToStr(modes[i]->w) + "_" + intToStr(modes[i]->h) + "_" + intToStr(bpp);
				    if(uniqueResList.find(lookupKey) == uniqueResList.end()) {
				    	uniqueResList[lookupKey] = true;
				    	modeinfos->push_back(ModeInfo(modes[i]->w,modes[i]->h,bpp));
				    	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] adding resolution: %d x %d - %d.\n",__FILE__,__FUNCTION__,__LINE__,modes[i]->w,modes[i]->h,bpp);
				    }
				    // fake the missing 16 bit resolutions
				    string lookupKey16 = intToStr(modes[i]->w) + "_" + intToStr(modes[i]->h) + "_" + intToStr(16);
				    if(uniqueResList.find(lookupKey16) == uniqueResList.end()) {
				    	uniqueResList[lookupKey16] = true;
				    	modeinfos->push_back(ModeInfo(modes[i]->w,modes[i]->h,16));
				    	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] adding resolution: %d x %d - %d.\n",__FILE__,__FUNCTION__,__LINE__,modes[i]->w,modes[i]->h,16);
				    }
			   }
		  }
	} while(++loops != 4);

	std::sort(modeinfos->begin(),modeinfos->end());
}



bool changeVideoMode(int resW, int resH, int colorBits, int ) {
	Private::shouldBeFullscreen = true;
	return true;
}

void restoreVideoMode(bool exitingApp) {
    //SDL_Quit();
}

int getScreenW() {
	return SDL_GetVideoSurface()->w;
}

int getScreenH() {
	return SDL_GetVideoSurface()->h;
}

void sleep(int millis) {
	SDL_Delay(millis);
}

bool isCursorShowing() {
	int state = SDL_ShowCursor(SDL_QUERY);
	return (state == SDL_ENABLE);
}

void showCursor(bool b) {
	if(isCursorShowing() == b) {
		return;
	}

	SDL_ShowCursor(b == true ? SDL_ENABLE : SDL_DISABLE);
}

//bool isKeyDown(SDLKey key) {
//	const Uint8* keystate = SDL_GetKeyState(0);
//
//	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = %d\n",__FILE__,__FUNCTION__,__LINE__,key);
//
//	if(key >= 0) {
//		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] keystate[key] = %d\n",__FILE__,__FUNCTION__,__LINE__,keystate[key]);
//
//		return (keystate[key] != 0);
//	}
//	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] returning false\n",__FILE__,__FUNCTION__,__LINE__);
//	return false;
//
//}

bool isKeyDown(int virtualKey) {
	char key = static_cast<char> (virtualKey);
	const Uint8* keystate = SDL_GetKeyState(0);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] key = %d\n",__FILE__,__FUNCTION__,__LINE__,key);

	// kinda hack and wrong...
	if(key >= 0) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] keystate[key] = %d\n",__FILE__,__FUNCTION__,__LINE__,keystate[key]);

		return (keystate[key] != 0);
	}
	switch(key) {
		case vkAdd:
			return (keystate[SDLK_PLUS] != 0 || keystate[SDLK_KP_PLUS] != 0);
		case vkSubtract:
			return (keystate[SDLK_MINUS] != 0 || keystate[SDLK_KP_MINUS] != 0);
		case vkAlt:
			return (keystate[SDLK_LALT] != 0 || keystate[SDLK_RALT] != 0);
		case vkControl:
			return (keystate[SDLK_LCTRL] != 0 || keystate[SDLK_RCTRL] != 0);
		case vkShift:
			return (keystate[SDLK_LSHIFT] != 0 || keystate[SDLK_RSHIFT] != 0);
		case vkEscape:
			return (keystate[SDLK_ESCAPE] != 0);
		case vkUp:
			return (keystate[SDLK_UP] != 0);
		case vkLeft:
			return (keystate[SDLK_LEFT] != 0);
		case vkRight:
			return (keystate[SDLK_RIGHT] != 0);
		case vkDown:
			return (keystate[SDLK_DOWN] != 0);
		case vkReturn:
			return (keystate[SDLK_RETURN] != 0 || keystate[SDLK_KP_ENTER] != 0);
		case vkBack:
			return  (keystate[SDLK_BACKSPACE] != 0);
		case vkDelete:
			return (keystate[SDLK_DELETE] != 0);
		case vkPrint:
			return (keystate[SDLK_PRINT] != 0);
		case vkPause:
			return (keystate[SDLK_PAUSE] != 0);
		default:
			std::cerr << "isKeyDown called with unknown key.\n";
			break;
	}
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] returning false\n",__FILE__,__FUNCTION__,__LINE__);
	return false;
}

string replaceAll(string& context, const string& from, const string& to) {
    size_t lookHere = 0;
    size_t foundHere = 0;
    if((foundHere = context.find(from, lookHere)) != string::npos) {
    	//if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Replacing context [%s] from [%s] to [%s]\n",context.c_str(),from.c_str(),to.c_str());

		while((foundHere = context.find(from, lookHere)) != string::npos) {
			  context.replace(foundHere, from.size(), to);
			  lookHere = foundHere + to.size();
		}

		//if(SystemFlags::VERBOSE_MODE_ENABLED) printf("New context [%s]\n",context.c_str());
    }
    return context;
}

string getFullFileArchiveExtractCommand(string fileArchiveExtractCommand,
		string fileArchiveExtractCommandParameters, string outputpath, string archivename) {
	string parsedOutputpath = outputpath;
	string parsedArchivename = archivename;

// This is required for execution on win32
#if defined(WIN32)
	replaceAll(parsedOutputpath, "\\\\", "\\");
	replaceAll(parsedOutputpath, "/", "\\");
	replaceAll(parsedArchivename, "\\\\", "\\");
	replaceAll(parsedArchivename, "/", "\\");
#endif

	string result = fileArchiveExtractCommand;
	result += " ";
	string args = replaceAll(fileArchiveExtractCommandParameters, "{outputpath}", parsedOutputpath);
	args = replaceAll(args, "{archivename}", parsedArchivename);
	result += args;

	return result;
}

bool executeShellCommand(string cmd, int expectedResult) {
	bool result = false;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"About to run [%s]", cmd.c_str());
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("About to run [%s]", cmd.c_str());

#ifdef WIN32
	FILE *file = _wpopen(utf8_decode(cmd).c_str(),L"r");
#else
	FILE *file = popen(cmd.c_str(),"r");
#endif

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"file = [%p]", file);
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("file = [%p]", file);

	if(file != NULL) {
		char szBuf[4096]="";
		while(feof(file) == false) {
			if(fgets( szBuf, 4095, file) != NULL) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"%s",szBuf);
				if(SystemFlags::VERBOSE_MODE_ENABLED) printf("%s",szBuf);
			}
		}
#ifdef WIN32
		int cmdRet = _pclose(file);
#else
		int cmdRet = pclose(file);
#endif

		/* Close pipe and print return value. */
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"Process returned %d\n",  cmdRet);
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Process returned %d\n",  cmdRet);
		result = (expectedResult == IGNORE_CMD_RESULT_VALUE || expectedResult == cmdRet);
	}
	return result;
}

bool removeFile(string file) {
#ifdef WIN32
	int result = _unlink(file.c_str());
#else
    int result = unlink(file.c_str());
#endif

    return (result == 0);
}

bool renameFile(string oldFile, string newFile) {
	int result = rename(oldFile.c_str(),newFile.c_str());
    return (result == 0);
}

off_t getFileSize(string filename) {
#ifdef WIN32
  #if defined(__MINGW32__)
  struct _stat stbuf;
  #else
  struct _stat64i32 stbuf;
  #endif
  if(_wstat(utf8_decode(filename).c_str(), &stbuf) != -1) {
#else
  struct stat stbuf;
  if(stat(filename.c_str(), &stbuf) != -1) {
#endif
	  return stbuf.st_size;
  }
  return 0;
}

string executable_path(string exeName, bool includeExeNameInPath) {
	string value = "";
#ifdef _WIN32
	char path[MAX_PATH]="";
	if( GetModuleFileNameA(NULL,path,MAX_PATH) == 0 ) {
		if(includeExeNameInPath == true) {
			value = exeName;
		}
		else {
			value = extractDirectoryPathFromFile(exeName);
		}
	}
	else {
		if(includeExeNameInPath == true) {
			value = path;
		}
		else {
			value = extractDirectoryPathFromFile(path);
		}
	}
#elif __APPLE__
	char path[MAXPATHLEN+1]="";
	uint32_t path_len = MAXPATHLEN;
	if ( _NSGetExecutablePath(path, &path_len) ) {
		if(includeExeNameInPath == true) {
			value = exeName;
		}
		else {
			value = extractDirectoryPathFromFile(exeName);
		}
	}
	else {
		if(includeExeNameInPath == true) {
			value = path;
		}
		else {
			value = extractDirectoryPathFromFile(path);
		}
	}
#else
	char exe_link_path[200]="";
	int length = readlink("/proc/self/exe", exe_link_path, sizeof(exe_link_path));
	if(length < 0 || length >= 200 ) {
		char *argv0_path = realpath(exeName.c_str(),NULL);
		if(argv0_path != NULL) {
			if(includeExeNameInPath == true) {
				value = argv0_path;
			}
			else {
				value = extractDirectoryPathFromFile(argv0_path);
			}
			free(argv0_path);
			argv0_path = NULL;
		}
		else {
			const char *shell_path = getenv("_");
			if(shell_path != NULL) {
				if(includeExeNameInPath == true) {
					value = shell_path;
				}
				else {
					value = extractDirectoryPathFromFile(shell_path);
				}
			}
			else {
				if(includeExeNameInPath == true) {
					value = exeName;
				}
				else {
					value = extractDirectoryPathFromFile(exeName);
				}
			}
		}
	}
	else {
		exe_link_path[length] = '\0';
		if(includeExeNameInPath == true) {
			value = exe_link_path;
		}
		else {
			value = extractDirectoryPathFromFile(exe_link_path);
		}
	}
#endif
    return value;
}

bool searchAndReplaceTextInFile(string fileName, string findText, string replaceText, bool simulateOnly) {
	bool replacedText = false;
	const int MAX_LEN_SINGLE_LINE = 4096;
	char buffer[MAX_LEN_SINGLE_LINE+2];
	char *buff_ptr, *find_ptr;
	FILE *fp1, *fp2;
	size_t find_len = findText.length();

	string tempfileName = fileName + "_tmp";
#ifdef WIN32
	fp1 = _wfopen(utf8_decode(fileName).c_str(), L"r");
	fp2 = _wfopen(utf8_decode(tempfileName).c_str(), L"w");
#else
	fp1 = fopen(fileName.c_str(),"r");
	fp2 = fopen(tempfileName.c_str(),"w");
#endif

	if(fp1 == NULL) {
		throw runtime_error("cannot open input file [" + fileName + "]");
	}
	if(fp2 == NULL) {
		throw runtime_error("cannot open output file [" + tempfileName + "]");
	}

	while(fgets(buffer,MAX_LEN_SINGLE_LINE + 2,fp1)) {
		buff_ptr = buffer;
		if(findText != "") {
			while ((find_ptr = strstr(buff_ptr,findText.c_str()))) {
				//printf("Replacing text [%s] with [%s] in file [%s]\n",findText.c_str(),replaceText.c_str(),fileName.c_str());

				while(buff_ptr < find_ptr) {
					fputc((int)*buff_ptr++,fp2);
				}
				fputs(replaceText.c_str(),fp2);

				buff_ptr += find_len;
				replacedText = true;
			}
		}
		fputs(buff_ptr,fp2);
	}

	fclose(fp2);
	fclose(fp1);

	if(replacedText == true && simulateOnly == false) {
		removeFile(fileName);
		renameFile(tempfileName,fileName);
	}
	else {
		removeFile(tempfileName);
	}
	//removeFile(tempfileName);
	return replacedText;
}

void copyFileTo(string fromFileName, string toFileName) {
	//Open an input and output stream in binary mode
#if defined(WIN32) && !defined(__MINGW32__)
	FILE *fp1 = _wfopen(utf8_decode(fromFileName).c_str(), L"rb");
	ifstream in(fp1);
	FILE *fp2 = _wfopen(utf8_decode(toFileName).c_str(), L"wb");
	ofstream out(fp2);
#else
	ifstream in(fromFileName.c_str(),ios::binary);
	ofstream out(toFileName.c_str(),ios::binary);
#endif

	if(in.is_open() && out.is_open()) {
		while(in.eof() == false) {
			out.put(in.get());
		}
	}
	else if(in.is_open() == false) {
		throw runtime_error("cannot open input file [" + fromFileName + "]");
	}
	else if(out.is_open() == false) {
		throw runtime_error("cannot open input file [" + toFileName + "]");
	}

	//Close both files
	in.close();
	out.close();

#if defined(WIN32) && !defined(__MINGW32__)
	if(fp1) {
		fclose(fp1);
	}
	if(fp2) {
		fclose(fp2);
	}
#endif
}

bool valid_utf8_file(const char* file_name) {
#if defined(WIN32) && !defined(__MINGW32__)
	wstring wstr = utf8_decode(file_name);
	FILE *fp = _wfopen(wstr.c_str(), L"r");
	ifstream ifs(fp);
#else
    ifstream ifs(file_name);
#endif

    if (!ifs) {
        return false; // even better, throw here
    }
    istreambuf_iterator<char> it(ifs.rdbuf());
    istreambuf_iterator<char> eos;

    bool result = utf8::is_valid(it, eos);

	ifs.close();
#if defined(WIN32) && !defined(__MINGW32__)
	if(fp) {
		fclose(fp);
	}
#endif

	return result;
}

// =====================================
//         ModeInfo
// =====================================

ModeInfo::ModeInfo(int w, int h, int d) {
	width=w;
	height=h;
	depth=d;
}

string ModeInfo::getString() const{
	return intToStr(width)+"x"+intToStr(height)+"-"+intToStr(depth);
}

void ValueCheckerVault::addItemToVault(const void *ptr,int value) {
#ifndef _DISABLE_MEMORY_VAULT_CHECKS

	Checksum checksum;
	vaultList[ptr] = checksum.addInt(value);

#endif

//	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] add vault key [%p] value [%s] [%d]\n",__FILE__,__FUNCTION__,__LINE__,ptr,intToStr(checksum.getSum()).c_str(),value);
}

void ValueCheckerVault::checkItemInVault(const void *ptr,int value) const {
#ifndef _DISABLE_MEMORY_VAULT_CHECKS

	map<const void *,int32>::const_iterator iterFind = vaultList.find(ptr);
	if(iterFind == vaultList.end()) {
//		if(SystemFlags::VERBOSE_MODE_ENABLED) {
//			printf("In [%s::%s Line: %d] check vault key [%p] value [%d]\n",__FILE__,__FUNCTION__,__LINE__,ptr,value);
//			for(map<const void *,string>::const_iterator iterFind = vaultList.begin();
//					iterFind != vaultList.end(); iterFind++) {
//				printf("In [%s::%s Line: %d] LIST-- check vault key [%p] value [%s]\n",__FILE__,__FUNCTION__,__LINE__,iterFind->first,iterFind->second.c_str());
//			}
//		}
		throw std::runtime_error("memory value has been unexpectedly modified (not found)!");
	}
	Checksum checksum;
	if(iterFind->second != checksum.addInt(value)) {
//		if(SystemFlags::VERBOSE_MODE_ENABLED) {
//			printf("In [%s::%s Line: %d] check vault key [%p] value [%s] [%d]\n",__FILE__,__FUNCTION__,__LINE__,ptr,intToStr(checksum.getSum()).c_str(),value);
//			for(map<const void *,string>::const_iterator iterFind = vaultList.begin();
//					iterFind != vaultList.end(); iterFind++) {
//				printf("In [%s::%s Line: %d] LIST-- check vault key [%p] value [%s]\n",__FILE__,__FUNCTION__,__LINE__,iterFind->first,iterFind->second.c_str());
//			}
//		}
		throw std::runtime_error("memory value has been unexpectedly modified (changed)!");
	}

#endif

}


}}//end namespace
