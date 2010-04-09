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

#include "platform_util.h"

#include <io.h>
#include <DbgHelp.h>

#include <cassert>

#include "util.h"
#include "conversion.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>
#include <algorithm>
#include <map>

#include <SDL.h>
#include "sdl_private.h"
#include "window.h"
#include "noimpl.h"

#include "leak_dumper.h"

#define S_ISDIR(mode) ((mode) & _S_IFDIR)

using namespace Shared::Util;
using namespace std;

namespace Shared{ namespace Platform{

namespace Private{

bool shouldBeFullscreen = false;
int ScreenWidth;
int ScreenHeight;

}

// =====================================================
//	class PerformanceTimer
// =====================================================

void PerformanceTimer::init(int fps, int maxTimes){
	int64 freq;

	if(QueryPerformanceFrequency((LARGE_INTEGER*) &freq)==0){
		throw runtime_error("Performance counters not supported");
	}

	times= 0;
	this->maxTimes= maxTimes;

	QueryPerformanceCounter((LARGE_INTEGER*) &lastTicks);

	updateTicks= freq/fps;
}

bool PerformanceTimer::isTime(){
	QueryPerformanceCounter((LARGE_INTEGER*) &thisTicks);

	if((thisTicks-lastTicks)>=updateTicks && times<maxTimes){
		lastTicks+= updateTicks;
		times++;
		return true;
	}
	times= 0;
	return false;
}

void PerformanceTimer::reset(){
	QueryPerformanceCounter((LARGE_INTEGER*) &thisTicks);
	lastTicks= thisTicks;
}

// =====================================================
//	class Chrono
// =====================================================

int64 Chrono::freq;

Chrono::Chrono(){
	if(!QueryPerformanceFrequency((LARGE_INTEGER*) &freq)){
		throw runtime_error("Performance counters not supported");
	}
	stopped= true;
	accumCount= 0;
}

void Chrono::start(){
	stopped= false;
	QueryPerformanceCounter((LARGE_INTEGER*) &startCount);
}

void Chrono::stop(){
	int64 endCount;
	QueryPerformanceCounter((LARGE_INTEGER*) &endCount);
	accumCount+= endCount-startCount;
	stopped= true;
}

int64 Chrono::getMicros() const{
	return queryCounter(1000000);
}

int64 Chrono::getMillis() const{
	return queryCounter(1000);
}

int64 Chrono::getSeconds() const{
	return queryCounter(1);
}

int64 Chrono::queryCounter(int multiplier) const{
	if(stopped){
		return multiplier*accumCount/freq;
	}
	else{
		int64 endCount;
		QueryPerformanceCounter((LARGE_INTEGER*) &endCount);
		return multiplier*(accumCount+endCount-startCount)/freq;
	}
}

int64 Chrono::getCurMillis() {
    return getCurTicks() * 1000 / freq;
}
int64 Chrono::getCurTicks() {
		int64 now;
		QueryPerformanceCounter((LARGE_INTEGER*) &now);
		return now;
}

// =====================================================
//	class PlatformExceptionHandler
// =====================================================

PlatformExceptionHandler *PlatformExceptionHandler::thisPointer= NULL;

LONG WINAPI PlatformExceptionHandler::handler(LPEXCEPTION_POINTERS pointers){

	//printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	HANDLE hFile = CreateFile(
		thisPointer->dumpFileName.c_str(),
		GENERIC_WRITE,
		FILE_SHARE_WRITE,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		0);

	//printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	MINIDUMP_EXCEPTION_INFORMATION lExceptionInformation;

	lExceptionInformation.ThreadId= GetCurrentThreadId();
	lExceptionInformation.ExceptionPointers= pointers;
	lExceptionInformation.ClientPointers= false;

	MiniDumpWriteDump(
		GetCurrentProcess(),
		GetCurrentProcessId(),
		hFile,
		MiniDumpNormal,
		&lExceptionInformation,
		NULL,
		NULL );

	//printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	thisPointer->handle();

	//printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	return EXCEPTION_EXECUTE_HANDLER;
}

void PlatformExceptionHandler::install(string dumpFileName){
	thisPointer= this;
	this->dumpFileName= dumpFileName;
	SetUnhandledExceptionFilter(handler);
}

// =====================================================
//	class Misc
// =====================================================

void Tokenize(const string& str,vector<string>& tokens,const string& delimiters) {
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
}

void findDirs(const vector<string> &paths, vector<string> &results, bool errorOnNotFound) {
    results.clear();
    int pathCount = paths.size();
    for(int idx = 0; idx < pathCount; idx++) {
        string path = paths[idx] + "/*.";
        vector<string> current_results;
        findAll(path, current_results, false, errorOnNotFound);
        if(current_results.size() > 0) {
            for(unsigned int folder_index = 0; folder_index < current_results.size(); folder_index++) {
                const string current_folder = current_results[folder_index];
                const string current_folder_path = paths[idx] + "/" + current_folder;
                if(isdir(current_folder_path.c_str()) == true) {
                    if(std::find(results.begin(),results.end(),current_folder) == results.end()) {
                        results.push_back(current_folder);
                    }
                }
            }
        }
    }

    std::sort(results.begin(),results.end());
}

void findAll(const vector<string> &paths, const string &fileFilter, vector<string> &results, bool cutExtension, bool errorOnNotFound) {
    results.clear();
    int pathCount = paths.size();
    for(int idx = 0; idx < pathCount; idx++) {
        string path = paths[idx] + "/" + fileFilter;
        vector<string> current_results;
        findAll(path, current_results, cutExtension, errorOnNotFound);
        if(current_results.size() > 0) {
            for(unsigned int folder_index = 0; folder_index < current_results.size(); folder_index++) {
                string &current_file = current_results[folder_index];
                if(std::find(results.begin(),results.end(),current_file) == results.end()) {
                    results.push_back(current_file);
                }
            }
        }
    }

    std::sort(results.begin(),results.end());
}

//finds all filenames like path and stores them in resultys
void findAll(const string &path, vector<string> &results, bool cutExtension, bool errorOnNotFound){

	int i= 0;
	struct _finddata_t fi;
	intptr_t handle;
	char *cstr;

	results.clear();

	cstr= new char[path.length()+1];
	strcpy(cstr, path.c_str());

	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] path [%s]\n",__FILE__,__FUNCTION__,__LINE__,path.c_str());

	if((handle=_findfirst(cstr,&fi))!=-1){
		do{
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] fi.name [%s]\n",__FILE__,__FUNCTION__,__LINE__,fi.name);

			if((fi.attrib & _A_HIDDEN) == _A_HIDDEN) {
				SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] file IS HIDDEN fi.name [%s]\n",__FILE__,__FUNCTION__,__LINE__,fi.name);
			}
			else
			{
				if(!(strcmp(".", fi.name)==0 || strcmp("..", fi.name)==0 || strcmp(".svn", fi.name)==0)){
					i++;
					results.push_back(fi.name);
				}
				else {
					SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] SKIPPED SPECIAL FILENAME [%s]\n",__FILE__,__FUNCTION__,__LINE__,fi.name);
				}
			}
		}
		while(_findnext(handle, &fi)==0);
	}
	else if(errorOnNotFound == true){
		_findclose(handle);
		throw runtime_error("Error opening files: "+ path);
	}

	_findclose(handle);
	if(i==0 && errorOnNotFound == true){
		throw runtime_error("No files found: "+ path);
	}

	if(cutExtension){
		for (unsigned int i=0; i<results.size(); ++i){
			results.at(i)=cutLastExt(results.at(i));
		}
	}

	delete [] cstr;
}

bool isdir(const char *path)
{
  struct stat stats;
  bool ret = stat (path, &stats) == 0 && S_ISDIR(stats.st_mode);
  if(ret == false) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] NOT a path [%s]\n",__FILE__,__FUNCTION__,__LINE__,path);

  return ret;
}

bool EndsWith(const string &str, const string& key)
{
    bool result = false;
    if (str.length() > key.length()) {
    	result = (0 == str.compare (str.length() - key.length(), key.length(), key));
    }

    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] result [%d] str = [%s] key = [%s]\n",__FILE__,__FUNCTION__,result,str.c_str(),key.c_str());
    return result;
}

//finds all filenames like path and gets their checksum of all files combined
int32 getFolderTreeContentsCheckSumRecursively(vector<string> paths, string pathSearchString, const string filterFileExt, Checksum *recursiveChecksum) {
	Checksum checksum = (recursiveChecksum == NULL ? Checksum() : *recursiveChecksum);
	int count = paths.size();
	for(int idx = 0; idx < count; ++idx) {
		string path = paths[idx] + pathSearchString;
		getFolderTreeContentsCheckSumRecursively(path, filterFileExt, &checksum);
	}

	if(recursiveChecksum != NULL) {
		*recursiveChecksum = checksum;
	}

	return checksum.getSum();
}

//finds all filenames like path and gets their checksum of all files combined
int32 getFolderTreeContentsCheckSumRecursively(const string &path, const string &filterFileExt, Checksum *recursiveChecksum) {

    Checksum checksum = (recursiveChecksum == NULL ? Checksum() : *recursiveChecksum);

/* MV - PORT THIS to win32

    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] scanning [%s]\n",__FILE__,__FUNCTION__,path.c_str());

	std::string mypath = path;
	// Stupid win32 is searching for all files without extension when *. is specified as wildcard
	if(mypath.compare(mypath.size() - 2, 2, "*.") == 0) {
		mypath = mypath.substr(0, mypath.size() - 2);
		mypath += "*";
	}

	glob_t globbuf;

	int res = glob(mypath.c_str(), 0, 0, &globbuf);
	if(res < 0) {
		std::stringstream msg;
		msg << "Couldn't scan directory '" << mypath << "': " << strerror(errno);
		throw runtime_error(msg.str());
	}

	for(size_t i = 0; i < globbuf.gl_pathc; ++i) {
		const char* p = globbuf.gl_pathv[i];
		//
		//const char* begin = p;
		//for( ; *p != 0; ++p) {
		//	// strip the path component
		//	if(*p == '/')
		//		begin = p+1;
		//}


		if(isdir(p) == false)
		{
            bool addFile = true;
            if(filterFileExt != "")
            {
                addFile = EndsWith(p, filterFileExt);
            }

            if(addFile)
            {
                //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] adding file [%s]\n",__FILE__,__FUNCTION__,p);

                checksum.addFile(p);
            }
		}
	}

	globfree(&globbuf);

    // Look recursively for sub-folders
	res = glob(mypath.c_str(), GLOB_ONLYDIR, 0, &globbuf);
	if(res < 0) {
		std::stringstream msg;
		msg << "Couldn't scan directory '" << mypath << "': " << strerror(errno);
		throw runtime_error(msg.str());
	}

	for(size_t i = 0; i < globbuf.gl_pathc; ++i) {
		const char* p = globbuf.gl_pathv[i];
		//
		//const char* begin = p;
		//for( ; *p != 0; ++p) {
			// strip the path component
		//	if(*p == '/')
		//		begin = p+1;
		//}

        getFolderTreeContentsCheckSumRecursively(string(p) + "/*", filterFileExt, &checksum);
	}

	globfree(&globbuf);
*/

	if(recursiveChecksum != NULL) {
		*recursiveChecksum = checksum;
	}

    return checksum.getSum();
}

vector<std::pair<string,int32> > getFolderTreeContentsCheckSumListRecursively(vector<string> paths, string pathSearchString, string filterFileExt, vector<std::pair<string,int32> > *recursiveMap) {
	vector<std::pair<string,int32> > checksumFiles = (recursiveMap == NULL ? vector<std::pair<string,int32> >() : *recursiveMap);
	int count = paths.size();
	for(int idx = 0; idx < count; ++idx) {
		string path = paths[idx] + pathSearchString;
		getFolderTreeContentsCheckSumListRecursively(path, filterFileExt, &checksumFiles);
	}
	return checksumFiles;
}

//finds all filenames like path and gets the checksum of each file
vector<std::pair<string,int32> > getFolderTreeContentsCheckSumListRecursively(const string &path, const string &filterFileExt, vector<std::pair<string,int32> > *recursiveMap) {

    vector<std::pair<string,int32> > checksumFiles = (recursiveMap == NULL ? vector<std::pair<string,int32> >() : *recursiveMap);

/* MV - PORT THIS to win32

    //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] scanning [%s]\n",__FILE__,__FUNCTION__,path.c_str());

	std::string mypath = path;
	// Stupid win32 is searching for all files without extension when *. is specified as wildcard
	if(mypath.compare(mypath.size() - 2, 2, "*.") == 0) {
		mypath = mypath.substr(0, mypath.size() - 2);
		mypath += "*";
	}

	glob_t globbuf;

	int res = glob(mypath.c_str(), 0, 0, &globbuf);
	if(res < 0) {
		std::stringstream msg;
		msg << "Couldn't scan directory '" << mypath << "': " << strerror(errno);
		throw runtime_error(msg.str());
	}

	for(size_t i = 0; i < globbuf.gl_pathc; ++i) {
		const char* p = globbuf.gl_pathv[i];
		//
		//const char* begin = p;
		//for( ; *p != 0; ++p) {
			// strip the path component
		//	if(*p == '/')
		//		begin = p+1;
		//}


		if(isdir(p) == false)
		{
            bool addFile = true;
            if(filterFileExt != "")
            {
                addFile = EndsWith(p, filterFileExt);
            }

            if(addFile)
            {
                //SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s] adding file [%s]\n",__FILE__,__FUNCTION__,p);

                Checksum checksum;
                checksum.addFile(p);

                checksumFiles.push_back(std::pair<string,int32>(p,checksum.getSum()));
            }
		}
	}

	globfree(&globbuf);

    // Look recursively for sub-folders
	res = glob(mypath.c_str(), GLOB_ONLYDIR, 0, &globbuf);
	if(res < 0) {
		std::stringstream msg;
		msg << "Couldn't scan directory '" << mypath << "': " << strerror(errno);
		throw runtime_error(msg.str());
	}

	for(size_t i = 0; i < globbuf.gl_pathc; ++i) {
		const char* p = globbuf.gl_pathv[i];
		//
		//const char* begin = p;
		//for( ; *p != 0; ++p) {
			// strip the path component
		//	if(*p == '/')
		//		begin = p+1;
		//}

        checksumFiles = getFolderTreeContentsCheckSumListRecursively(string(p) + "/*", filterFileExt, &checksumFiles);
	}

	globfree(&globbuf);
*/
    return checksumFiles;
}

string extractDirectoryPathFromFile(string filename)
{
    return filename.substr( 0, filename.rfind("/")+1 );
}

string extractExtension(const string& filepath) {
	size_t lastPoint = filepath.find_last_of('.');
	size_t lastDirectory_Win = filepath.find_last_of('\\');
	size_t lastDirectory_Lin = filepath.find_last_of('/');
	size_t lastDirectory = (lastDirectory_Win<lastDirectory_Lin)?lastDirectory_Lin:lastDirectory_Win;
	if (lastPoint == string::npos || (lastDirectory != string::npos && lastDirectory > lastPoint)) {
		return "";
	}
	return filepath.substr(lastPoint+1);
} 

void createDirectoryPaths(string Path)
{
 char DirName[256]="";
 const char *path = Path.c_str();
 char *dirName = DirName;
 while(*path)
 {
   //if (('\\' == *path) || ('/' == *path))
   if ('/' == *path)
   {
     //if (':' != *(path-1))
     {
        _mkdir(DirName);
     }
   }
   *dirName++ = *path++;
   *dirName = '\0';
 }
 _mkdir(DirName);
}

//void getFullscreenVideoInfo(int &colorBits,int &screenWidth,int &screenHeight) {
//    // Get the current video hardware information
//    //const SDL_VideoInfo* vidInfo = SDL_GetVideoInfo();
//    //colorBits      = vidInfo->vfmt->BitsPerPixel;
//    //screenWidth    = vidInfo->current_w;
//
///*
//	//screenHeight   = vidInfo->current_h;
//    int cx = GetSystemMetrics(SM_CXVIRTUALSCREEN);
//    // height
//    int cy = GetSystemMetrics(SM_CYVIRTUALSCREEN);
//
//	printf("cx = %d, cy = %d\n",cx,cy);
//
//	if(cx > screenWidth) {
//		screenWidth  = cx;
//		screenHeight = cy;
//	}
//*/
//	int iMaxWidth	= -1;
//	int iMaxHeight	= -1;
//	int iMaxBits	= -1;
//
//	DEVMODE devMode;
//	for (int i=0; EnumDisplaySettings(NULL, i, &devMode) ;i++){
//
//		//printf("devMode.dmPelsWidth = %d, devMode.dmPelsHeight = %d, devMode.dmBitsPerPel = %d\n",devMode.dmPelsWidth,devMode.dmPelsHeight,devMode.dmBitsPerPel);
//
//		if(devMode.dmPelsWidth > iMaxWidth) {
//			iMaxWidth  = devMode.dmPelsWidth;
//			iMaxHeight = devMode.dmPelsHeight;
//			iMaxBits   = devMode.dmBitsPerPel;
//			//devMode.dmDisplayFrequency=refreshFrequency;
//		}
//	}
//	if(iMaxWidth > 0) {
//		colorBits      = iMaxBits;
//		screenWidth    = iMaxWidth;
//		screenHeight   = iMaxHeight;
//	}
//}
//
//bool changeVideoMode(int resW, int resH, int colorBits, int refreshFrequency){
//	DEVMODE devMode;
//
//	for (int i=0; EnumDisplaySettings(NULL, i, &devMode) ;i++){
//		if (devMode.dmPelsWidth== resW &&
//			devMode.dmPelsHeight== resH &&
//			devMode.dmBitsPerPel== colorBits){
//
//			devMode.dmDisplayFrequency=refreshFrequency;
//
//			LONG result= ChangeDisplaySettings(&devMode, 0);
//			if(result == DISP_CHANGE_SUCCESSFUL){
//				return true;
//			}
//			else{
//				return false;
//			}
//		}
//	}
//
//	return false;
//}
//
//void restoreVideoMode(bool exitingApp) {
//	int dispChangeErr= ChangeDisplaySettings(NULL, 0);
//	assert(dispChangeErr==DISP_CHANGE_SUCCESSFUL);
//}

void getFullscreenVideoInfo(int &colorBits,int &screenWidth,int &screenHeight) {
    // Get the current video hardware information
    //const SDL_VideoInfo* vidInfo = SDL_GetVideoInfo();
    //colorBits      = vidInfo->vfmt->BitsPerPixel;
    //screenWidth    = vidInfo->current_w;
    //screenHeight   = vidInfo->current_h;

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    /* Get available fullscreen/hardware modes */
    SDL_Rect**modes = SDL_ListModes(NULL, SDL_FULLSCREEN|SDL_HWSURFACE);

    /* Check if there are any modes available */
    if (modes == (SDL_Rect**)0) {
       SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] no hardware modes available.\n",__FILE__,__FUNCTION__,__LINE__);

       const SDL_VideoInfo* vidInfo = SDL_GetVideoInfo();
       colorBits      = vidInfo->vfmt->BitsPerPixel;
       screenWidth    = vidInfo->current_w;
       screenHeight   = vidInfo->current_h;

       SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] using current resolution: %d x %d.\n",__FILE__,__FUNCTION__,__LINE__,screenWidth,screenHeight);
   }
   /* Check if our resolution is restricted */
   else if (modes == (SDL_Rect**)-1) {
       SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] all resolutions available.\n",__FILE__,__FUNCTION__,__LINE__);

       const SDL_VideoInfo* vidInfo = SDL_GetVideoInfo();
       colorBits      = vidInfo->vfmt->BitsPerPixel;
       screenWidth    = vidInfo->current_w;
       screenHeight   = vidInfo->current_h;

       SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] using current resolution: %d x %d.\n",__FILE__,__FUNCTION__,__LINE__,screenWidth,screenHeight);
   }
   else{
       /* Print valid modes */
       SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] available Modes are:\n",__FILE__,__FUNCTION__,__LINE__);

       int bestW = -1;
       int bestH = -1;
       for(int i=0; modes[i]; ++i) {
           SystemFlags::OutputDebug(SystemFlags::debugSystem,"%d x %d\n",modes[i]->w, modes[i]->h);

           if(bestW < modes[i]->w) {
               bestW = modes[i]->w;
               bestH = modes[i]->h;
           }
       }

       if(bestW > screenWidth) {
           screenWidth = bestW;
           screenHeight = bestH;
       }

       SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] using current resolution: %d x %d.\n",__FILE__,__FUNCTION__,__LINE__,screenWidth,screenHeight);
  }
}

void getFullscreenVideoModes(list<ModeInfo> *modeinfos) {
    // Get the current video hardware information
    //const SDL_VideoInfo* vidInfo = SDL_GetVideoInfo();
    //colorBits      = vidInfo->vfmt->BitsPerPixel;
    //screenWidth    = vidInfo->current_w;
    //screenHeight   = vidInfo->current_h;

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    SDL_PixelFormat format;
  	SDL_Rect **modes;
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
			}

			/* Get available fullscreen/hardware modes */
			//SDL_Rect**modes = SDL_ListModes(NULL, SDL_OPENGL|SDL_RESIZABLE);
			SDL_Rect**modes = SDL_ListModes(&format, SDL_FULLSCREEN);

			/* Check if there are any modes available */
			if (modes == (SDL_Rect**)0) {
			   SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] no hardware modes available.\n",__FILE__,__FUNCTION__,__LINE__);

			   const SDL_VideoInfo* vidInfo = SDL_GetVideoInfo();
  		       string lookupKey = intToStr(vidInfo->current_w) + "_" + intToStr(vidInfo->current_h) + "_" + intToStr(vidInfo->vfmt->BitsPerPixel);
		       if(uniqueResList.find(lookupKey) == uniqueResList.end()) {
		    	   uniqueResList[lookupKey] = true;
		    	   modeinfos->push_back(ModeInfo(vidInfo->current_w,vidInfo->current_h,vidInfo->vfmt->BitsPerPixel));
		       }
			   SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] adding only current resolution: %d x %d.\n",__FILE__,__FUNCTION__,__LINE__,vidInfo->current_w,vidInfo->current_h);
		   }
		   /* Check if our resolution is restricted */
		   else if (modes == (SDL_Rect**)-1) {
			   SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] all resolutions available.\n",__FILE__,__FUNCTION__,__LINE__);

			   const SDL_VideoInfo* vidInfo = SDL_GetVideoInfo();
  		       string lookupKey = intToStr(vidInfo->current_w) + "_" + intToStr(vidInfo->current_h) + "_" + intToStr(vidInfo->vfmt->BitsPerPixel);
		       if(uniqueResList.find(lookupKey) == uniqueResList.end()) {
		    	   uniqueResList[lookupKey] = true;
		    	   modeinfos->push_back(ModeInfo(vidInfo->current_w,vidInfo->current_h,vidInfo->vfmt->BitsPerPixel));
		       }
			   SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] adding only current resolution: %d x %d.\n",__FILE__,__FUNCTION__,__LINE__,vidInfo->current_w,vidInfo->current_h);
		   }
		   else{
			   /* Print valid modes */
			   SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] available Modes are:\n",__FILE__,__FUNCTION__,__LINE__);

			   for(int i=0; modes[i]; ++i) {
				   SystemFlags::OutputDebug(SystemFlags::debugSystem,"%d x %d\n",modes[i]->w, modes[i]->h,bpp);
				    string lookupKey = intToStr(modes[i]->w) + "_" + intToStr(modes[i]->h) + "_" + intToStr(bpp);
				    if(uniqueResList.find(lookupKey) == uniqueResList.end()) {
				    	uniqueResList[lookupKey] = true;
				    	modeinfos->push_back(ModeInfo(modes[i]->w,modes[i]->h,bpp));
				    	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] adding resolution: %d x %d - %d.\n",__FILE__,__FUNCTION__,__LINE__,modes[i]->w,modes[i]->h,bpp);
				    }
				    // fake the missing 16 bit resolutions
				    string lookupKey16 = intToStr(modes[i]->w) + "_" + intToStr(modes[i]->h) + "_" + intToStr(16);
				    if(uniqueResList.find(lookupKey16) == uniqueResList.end()) {
				    	uniqueResList[lookupKey16] = true;
				    	modeinfos->push_back(ModeInfo(modes[i]->w,modes[i]->h,16));
				    	SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] adding resolution: %d x %d - %d.\n",__FILE__,__FUNCTION__,__LINE__,modes[i]->w,modes[i]->h,16);
				    }
			   }
		  }
	} while(++loops != 3);
}

bool changeVideoMode(int resW, int resH, int colorBits, int ) {
	Private::shouldBeFullscreen = true;
	return true;
}

void restoreVideoMode(bool exitingApp) {
    SDL_Quit();
}


void message(string message){
	MessageBox(NULL, message.c_str(), "Message", MB_OK);
}

bool ask(string message){
	return MessageBox(NULL, message.c_str(), "Confirmation", MB_YESNO)==IDYES;
}

void exceptionMessage(const exception &excp){
	string message, title;
	showCursor(true);

	message+= "ERROR(S):\n\n";
	message+= excp.what();

	title= "Error: Unhandled Exception";
	printf("Error detected with text: %s\n",message.c_str());

	MessageBox(NULL, message.c_str(), title.c_str(), MB_ICONSTOP | MB_OK | MB_TASKMODAL);
}

//int getScreenW(){
//	return GetSystemMetrics(SM_CXSCREEN);
//}

//int getScreenH(){
//	return GetSystemMetrics(SM_CYSCREEN);
//}

int getScreenW() {
	return SDL_GetVideoSurface()->w;
}

int getScreenH() {
	return SDL_GetVideoSurface()->h;
}


void sleep(int millis){
	Sleep(millis);
}

void showCursor(bool b){
	ShowCursor(b);
}

bool isKeyDown(int virtualKey){
	//return (GetKeyState(virtualKey) & 0x8000) != 0;

	char key = static_cast<char> (virtualKey);
	const Uint8* keystate = SDL_GetKeyState(0);

	// kinda hack and wrong...
	if(key >= 0) {
		return keystate[key];
	}
	switch(key) {
		case vkAdd:
			return keystate[SDLK_PLUS] | keystate[SDLK_KP_PLUS];
		case vkSubtract:
			return keystate[SDLK_MINUS] | keystate[SDLK_KP_MINUS];
		case vkAlt:
			return keystate[SDLK_LALT] | keystate[SDLK_RALT];
		case vkControl:
			return keystate[SDLK_LCTRL] | keystate[SDLK_RCTRL];
		case vkShift:
			return keystate[SDLK_LSHIFT] | keystate[SDLK_RSHIFT];
		case vkEscape:
			return keystate[SDLK_ESCAPE];
		case vkUp:
			return keystate[SDLK_UP];
		case vkLeft:
			return keystate[SDLK_LEFT];
		case vkRight:
			return keystate[SDLK_RIGHT];
		case vkDown:
			return keystate[SDLK_DOWN];
		case vkReturn:
			return keystate[SDLK_RETURN] | keystate[SDLK_KP_ENTER];
		case vkBack:
			return keystate[SDLK_BACKSPACE];
		default:
			SystemFlags::OutputDebug(SystemFlags::debugSystem,"isKeyDown called with unknown key.\n");
			break;
	}
	return false;
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

}}//end namespace
