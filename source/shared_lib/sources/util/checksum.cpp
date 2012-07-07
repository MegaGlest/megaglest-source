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
std::map<string,uint32> Checksum::fileListCache;

int crc_table[256] =
{
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
	0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
	0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
	0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
	0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
	0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
	0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
	0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
	0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
	0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
	0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
	0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
	0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
	0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
	0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
	0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
	0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
	0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
	0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
	0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

Checksum::Checksum() {
	sum= 0;
	r= 55665;
	c1= 52845;
	c2= 22719;
}

uint32 Checksum::addByte(const char value) {
//	int32 cipher= (value ^ (r >> 8));
//
//	r= (cipher + r) * c1 + c2;
//	sum += cipher;
//
//	return cipher;

	const unsigned char *rVal = reinterpret_cast<const unsigned char *>(&value);
	sum = ~sum;
	sum = (sum >> 8) ^ crc_table[*rVal ^ (sum & 0xff)];
	sum = ~sum;

	return sum;
}

uint32 Checksum::addBytes(const void *_data, size_t _size) {
	const unsigned char *rVal = reinterpret_cast<const unsigned char *>(_data);
	sum = ~sum;
	while (_size--) {
		sum = (sum >> 8) ^ crc_table[*rVal++ ^ (sum & 0xff)];
	}
	sum = ~sum;

	return sum;
}


void Checksum::addSum(uint32 value) {
	sum += value;
}

uint32 Checksum::addInt(const int32 &value) {
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
	FILE *fp = _wfopen(wstr.c_str(), L"rb");
	ifstream ifs(fp);
#else
    ifstream ifs(path.c_str());
#endif

    if (ifs) {
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

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] buf.size() = %d, path [%s], isXMLFile = %d\n",__FILE__,__FUNCTION__,__LINE__,buf.size(), path.c_str(),isXMLFile);

		if(isXMLFile == true) {
			for(std::size_t i = 0; i < buf.size(); ++i) {
				// Ignore Spaces in XML files as they are
				// ONLY for formatting
				//if(isXMLFile == true) {
					if(inCommentTag == true) {
						if(buf[i] == '>' && i >= 3 && buf[i-1] == '-' && buf[i-2] == '-') {
							inCommentTag = false;
							//printf("TURNING OFF comment TAG, i = %d [%c]",i,buf[i]);
							if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] i = %d\n",__FILE__,__FUNCTION__,__LINE__,i);
						}
						else {
							//printf("SKIPPING XML comment character, i = %d [%c]",i,buf[i]);
							if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] i = %d\n",__FILE__,__FUNCTION__,__LINE__,i);
						}
						continue;
					}
					//else if(buf[i] == '-' && i >= 4 && buf[i-1] == '-' && buf[i-2] == '!' && buf[i-3] == '<') {
					else if(buf[i] == '<' && i+4 < bufSize && buf[i+1] == '!' && buf[i+2] == '-' && buf[i+3] == '-') {
						inCommentTag = true;
						//printf("TURNING ON comment TAG, i = %d [%c]",i,buf[i]);
						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] i = %d\n",__FILE__,__FUNCTION__,__LINE__,i);
						continue;
					}
					else if(buf[i] == ' ' || buf[i] == '\t' || buf[i] == '\n' || buf[i] == '\r') {
						//printf("SKIPPING special character, i = %d [%c]",i,buf[i]);
						if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] i = %d\n",__FILE__,__FUNCTION__,__LINE__,i);
						continue;
					}
				//}
				uint32 cipher = addByte(buf[i]);

				if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] %d / %d, cipher = %u\n",__FILE__,__FUNCTION__,__LINE__,i,buf.size(), cipher);
			}
		}
		else {
			uint32 cipher = addBytes(&buf[0],buf.size());
			if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] %d, cipher = %u\n",__FILE__,__FUNCTION__,__LINE__,buf.size(), cipher);
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

uint32 Checksum::getSum() {
	//printf("Getting checksum for files [%d]\n",fileList.size());
	if(fileList.size() > 0) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] fileList.size() = %d\n",__FILE__,__FUNCTION__,__LINE__,fileList.size());

		Checksum newResult;

		{

		for(std::map<string,uint32>::iterator iterMap = fileList.begin();
			iterMap != fileList.end(); ++iterMap) {

			MutexSafeWrapper safeMutexSocketDestructorFlag(&Checksum::fileListCacheSynchAccessor,string(__FILE__) + "_" + intToStr(__LINE__));
			if(Checksum::fileListCache.find(iterMap->first) == Checksum::fileListCache.end()) {
				Checksum fileResult;
				bool fileAddedOk = fileResult.addFileToSum(iterMap->first);
				Checksum::fileListCache[iterMap->first] = fileResult.getSum();
				//printf("fileAddedOk = %d for file [%s] CRC [%d]\n",fileAddedOk,iterMap->first.c_str(),Checksum::fileListCache[iterMap->first]);
			}
			else {
				//printf("Getting checksum from CACHE for file [%s] CRC [%d]\n",iterMap->first.c_str(),Checksum::fileListCache[iterMap->first]);
			}
			newResult.addSum(Checksum::fileListCache[iterMap->first]);
		}
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] fileList.size() = %d\n",__FILE__,__FUNCTION__,__LINE__,fileList.size());

		return newResult.getSum();
	}
	return sum;
}

uint32 Checksum::getFinalFileListSum() {
	sum = 0;
	return getSum();
}

uint32 Checksum::getFileCount() {
	return (uint32)fileList.size();
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
