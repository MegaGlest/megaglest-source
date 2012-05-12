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

#include "checksum.h"

#include <cassert>
#include <stdexcept>
#include <fcntl.h> // for open()

#ifdef WIN32
  #include <io.h> // for open()
#endif

#include <sys/stat.h> // for open()

#include "util.h"
#include "platform_common.h"
#include "conversion.h"
#include "platform_util.h"
#include "leak_dumper.h"

using namespace std;
using namespace Shared::PlatformCommon;
using namespace Shared::Util;

namespace Shared{ namespace Util{

// =====================================================
//	class Checksum
// =====================================================

Mutex Checksum::fileListCacheSynchAccessor;
std::map<string,int32> Checksum::fileListCache;

Checksum::Checksum() {
	sum= 0;
	r= 55665;
	c1= 52845;
	c2= 22719;
}

void Checksum::addByte(int8 value) {
	int32 cipher= (value ^ (r >> 8));

	r= (cipher + r) * c1 + c2;
	sum += cipher;
}

void Checksum::addSum(int32 value) {
	sum += value;
}

int32 Checksum::addInt(const int32 &value) {
	int8 byte 	= (value >>  0) & 0xFF;
	addByte(byte);
	byte  		= (value >>  8) & 0xFF;
	addByte(byte);
	byte 		= (value >> 16) & 0xFF;
	addByte(byte);
	byte 		= (value >> 24) & 0xFF;
	addByte(byte);

	return sum;
}

void Checksum::addString(const string &value) {
	for(unsigned int i = 0; i < value.size(); ++i) {
		addByte(value[i]);
	}
}

void Checksum::addFile(const string &path) {
	if(path != "") {
		fileList[path] = 0;
	}
}

bool Checksum::addFileToSum(const string &path) {

// OLD SLOW FILE I/O
/*
	FILE* file= fopen(path.c_str(), "rb");
	if(file!=NULL){

		addString(lastFile(path));

		while(!feof(file)){
			int8 byte= 0;

			size_t readBytes = fread(&byte, 1, 1, file);
			addByte(byte);
		}
	}
	else
	{
		throw megaglest_runtime_error("Can not open file: " + path);
	}
	fclose(file);
*/



/*
   const double MAX_CRC_FILESIZE = 100000000;
   int fd=0;
   size_t bytes_read, bytes_expected = MAX_CRC_FILESIZE * sizeof(int8);
   int8 *data=0;
   const char *infile = path.c_str();

   if ((fd = open(infile,O_RDONLY)) < 0)
	   throw megaglest_runtime_error("Can not open file: " + path);

   if ((data = (int8 *)malloc(bytes_expected)) == NULL)
	   throw megaglest_runtime_error("malloc failed, Can not open file: " + path);

   bytes_read = read(fd, data, bytes_expected);

   //if (bytes_read != bytes_expected)
   //   throw megaglest_runtime_error("read failed, Can not open file: " + path);

   for(int i = 0; i < bytes_read; i++) {
		addByte(data[i]);
   }
   free(data);
*/

    bool fileExists = false;

/*
#ifdef WIN32
	FILE* file= _wfopen(utf8_decode(path).c_str(), L"rb");
#else
	FILE* file= fopen(path.c_str(), "rb");
#endif
	if(file != NULL) {
        fileExists = true;
		addString(lastFile(path));

		bool isXMLFile = (EndsWith(path, ".xml") == true);
		bool inCommentTag=false;
		char buf[4096]="";  // Should be large enough.
		int bufSize = sizeof buf;
		while(!feof(file)) {
			//int8 byte= 0;

			//size_t readBytes = fread(&byte, 1, 1, file);
			memset(buf,0,bufSize);
			if(fgets(buf, bufSize, file) != NULL) {
				//addByte(byte);
			    for(int i = 0; i < bufSize && buf[i] != 0; i++) {
			    	// Ignore Spaces in XML files as they are
			    	// ONLY for formatting
			    	if(isXMLFile == true) {
			    		if(inCommentTag == true) {
			    			if(buf[i] == '>' && i >= 3 && buf[i-1] == '-' && buf[i-2] == '-') {
			    				inCommentTag = false;
			    				//printf("TURNING OFF comment TAG, i = %d [%c]",i,buf[i]);
			    			}
			    			else {
			    				//printf("SKIPPING XML comment character, i = %d [%c]",i,buf[i]);
			    			}
			    			continue;
			    		}
			    		//else if(buf[i] == '-' && i >= 4 && buf[i-1] == '-' && buf[i-2] == '!' && buf[i-3] == '<') {
			    		else if(buf[i] == '<' && i+4 < bufSize && buf[i+1] == '!' && buf[i+2] == '-' && buf[i+3] == '-') {
		    				inCommentTag = true;
		    				//printf("TURNING ON comment TAG, i = %d [%c]",i,buf[i]);
		    				continue;
			    		}
			    		else if(buf[i] == ' ' || buf[i] == '\t' || buf[i] == '\n' || buf[i] == '\r') {
			    			//printf("SKIPPING special character, i = %d [%c]",i,buf[i]);
			    			continue;
			    	    }
			    	}
			 		addByte(buf[i]);
			    }
			}
		}
	}
	else {
		throw megaglest_runtime_error("Can not open file: " + path);
	}
	fclose(file);
*/

#if defined(WIN32) && !defined(__MINGW32__)
	wstring wstr = utf8_decode(path);
	FILE *fp = _wfopen(wstr.c_str(), L"r");
	ifstream ifs(fp);
#else
    ifstream ifs(path.c_str());
#endif

    if (!ifs) {
        fileExists = true;
		addString(lastFile(path));

		bool isXMLFile = (EndsWith(path, ".xml") == true);
		bool inCommentTag=false;

		// Determine the file length
		ifs.seekg(0, ios::end);
		std::size_t size=ifs.tellg();
		ifs.seekg(0, ios::beg);

		int bufSize = size / sizeof(char);
		// Create a vector to store the data
		std::vector<char> buf(bufSize);
		// Load the data
		ifs.read((char*)&buf[0], buf.size());

	    for(unsigned int i = 0; i < buf.size(); ++i) {
	    	// Ignore Spaces in XML files as they are
	    	// ONLY for formatting
	    	if(isXMLFile == true) {
	    		if(inCommentTag == true) {
	    			if(buf[i] == '>' && i >= 3 && buf[i-1] == '-' && buf[i-2] == '-') {
	    				inCommentTag = false;
	    				//printf("TURNING OFF comment TAG, i = %d [%c]",i,buf[i]);
	    			}
	    			else {
	    				//printf("SKIPPING XML comment character, i = %d [%c]",i,buf[i]);
	    			}
	    			continue;
	    		}
	    		//else if(buf[i] == '-' && i >= 4 && buf[i-1] == '-' && buf[i-2] == '!' && buf[i-3] == '<') {
	    		else if(buf[i] == '<' && i+4 < bufSize && buf[i+1] == '!' && buf[i+2] == '-' && buf[i+3] == '-') {
    				inCommentTag = true;
    				//printf("TURNING ON comment TAG, i = %d [%c]",i,buf[i]);
    				continue;
	    		}
	    		else if(buf[i] == ' ' || buf[i] == '\t' || buf[i] == '\n' || buf[i] == '\r') {
	    			//printf("SKIPPING special character, i = %d [%c]",i,buf[i]);
	    			continue;
	    	    }
	    	}
	 		addByte(buf[i]);
	    }

		// Close the file
		ifs.close();
    }
#if defined(WIN32) && !defined(__MINGW32__)
	if(fp) {
		fclose(fp);
	}
#endif

    return fileExists;
}

int32 Checksum::getSum() {
	if(fileList.size() > 0) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] fileList.size() = %d\n",__FILE__,__FUNCTION__,__LINE__,fileList.size());

		Checksum newResult;
		for(std::map<string,int32>::iterator iterMap = fileList.begin();
			iterMap != fileList.end(); ++iterMap) {

			MutexSafeWrapper safeMutexSocketDestructorFlag(&Checksum::fileListCacheSynchAccessor,string(__FILE__) + "_" + intToStr(__LINE__));
			if(Checksum::fileListCache.find(iterMap->first) == Checksum::fileListCache.end()) {
				Checksum fileResult;
				fileResult.addFileToSum(iterMap->first);
				Checksum::fileListCache[iterMap->first] = fileResult.getSum();
			}
			newResult.addSum(Checksum::fileListCache[iterMap->first]);
		}
		return newResult.getSum();
	}
	return sum;
}

int32 Checksum::getFinalFileListSum() {
	sum = 0;
	return getSum();
}

int32 Checksum::getFileCount() {
	return (int32)fileList.size();
}

void Checksum::removeFileFromCache(const string file) {
	MutexSafeWrapper safeMutexSocketDestructorFlag(&Checksum::fileListCacheSynchAccessor,string(__FILE__) + "_" + intToStr(__LINE__));
    if(Checksum::fileListCache.find(file) != Checksum::fileListCache.end()) {
        Checksum::fileListCache.erase(file);
    }
}

void Checksum::clearFileCache() {
	MutexSafeWrapper safeMutexSocketDestructorFlag(&Checksum::fileListCacheSynchAccessor,string(__FILE__) + "_" + intToStr(__LINE__));
    Checksum::fileListCache.clear();
}

}}//end namespace
