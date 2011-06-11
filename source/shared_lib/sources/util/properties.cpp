// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2007 Martio Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "properties.h"

#include <fstream>
#include <stdexcept>
#include <cstring>

#include "conversion.h"
#include "util.h"
#include "platform_common.h"
#include "platform_util.h"

#ifdef WIN32
#include <shlwapi.h>
#include <shlobj.h>
#endif

#include "utf8.h"
#include "font.h"

//#include <locale>
//#include <iostream>
//#include <string>
//#include <sstream>
#include "string_utils.h"

#include "leak_dumper.h"

using namespace std;
using namespace Shared::PlatformCommon;
using namespace Shared::Platform;
using namespace Shared::Graphics;

namespace Shared{ namespace Util{

string Properties::applicationPath = "";

// =====================================================
//	class Properties
// =====================================================

//wstring widen( const string& str )
//{
//      wostringstream wstm ;
//      wstm.imbue(std::locale("en_US.UTF-8"));
//      const ctype<wchar_t>& ctfacet =
//      use_facet< ctype<wchar_t> >( wstm.getloc() ) ;
//      for( size_t i=0 ; i<str.size() ; ++i )
//      wstm << ctfacet.widen( str[i] ) ;
//      return wstm.str() ;
//}

// Convert a narrow string to a wide string//
//std::wstring widen(const std::string& str) {
//	// Make space for wide string
//	wchar_t* buffer = new wchar_t[str.size() + 1];
//	// convert ASCII to UNICODE
//	mbstowcs( buffer, str.c_str(), str.size() );
//	// NULL terminate it
//	buffer[str.size()] = 0;
//	// Clean memory and return it
//	std::wstring wstr = buffer;
//	delete [] buffer;
//	return wstr;
//}
// Widen an individual character

//wstring fromUtf8(const char* str, size_t length) {
//	wchar_t result[4097]= L"";
//	int len = 0;
//	for(int i = 0 ; i < length; i++)
//	{
//		if (((byte)str[i]) < 0x80)
//		{
//			result[len++] = ((byte)str[i]);
//			continue;
//		}
//		if (((byte)str[i]) >= 0xC0)
//		{
//			wchar_t c = ((byte)str[i++]) - 0xC0;
//			while(((byte)str[i]) >= 0x80)
//				c = (c << 6) | (((byte)str[i++]) - 0x80);
//			--i;
//			result[len++] = c;
//			continue;
//		}
//	}
//	result[len] = 0;
//	return result;
//}

//string conv_utf8_iso8859_7(string s) {
//    int len = s.size();
//    string out = "";
//    string curr_char = "";
//    for(int i=0; i < len; i++) {
//        curr_char = curr_char + s[i];
//        if( ( (s[i]) & (128+64) ) == 128) {
//            //character end found
//            if ( curr_char.size() == 2) {
//                // 2-byte character check for it is greek one and convert
//                if      ((curr_char[0])==205) out = out + (char)(curr_char[1]+16);
//                else if ((curr_char[0])==206) out = out + (char)(curr_char[1]+48);
//                else if ((curr_char[0])==207) out = out + (char)(curr_char[1]+112);
//                else ; // non greek 2-byte character, discard character
//            } else ;// n-byte character, n>2, discard character
//            curr_char = "";
//        }
//        else if ((s[i]) < 128) {
//            // character is one byte (ascii)
//            out = out + curr_char;
//            curr_char = "";
//        }
//    }
//    return out;
//}

// Map from the most-significant 6 bits of the first byte to the total number of bytes in a
// UTF-8 character.
//static char UTF8_2_ISO_8859_1_len[] =
//{
//  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* erroneous */
//  2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 5, 6
//};
//
//static char UTF8_2_ISO_8859_1_mask[] = {0x3F, 0x7F, 0x1F, 0x0F, 0x07,
//0x03, 0x01};


/*----------------------------------------------------------------------
-------
   Convert a UTF-8 string to a ISO-8859-1 MultiByte string.
   No more than 'count' bytes will be written to the output buffer.
   Return the size of the converted string in bytes, excl null
terminator.
*/
//int ldap_x_utf8s_to_iso_8859_1s( char *mbstr, const char *utf8str, size_t count )
//{
//  int res = 0;
//
//  while (*utf8str != '\0')
//  {
//    int           len = UTF8_2_ISO_8859_1_len[(*utf8str >> 2) & 0x3F];
//    unsigned long u   = *utf8str & UTF8_2_ISO_8859_1_mask[len];
//
//    // erroneous
//    if (len == 0)
//      len = 5;
//
//    for (++utf8str; --len > 0 && (*utf8str != '\0'); ++utf8str)
//    {
//      // be sure this is not an unexpected start of a new character
//      if ((*utf8str & 0xC0) != 0x80)
//        break;
//
//      u = (u << 6) | (*utf8str & 0x3F);
//    }
//
//    if (mbstr != 0 && count != 0)
//    {
//      // be sure there is enough space left in the destination buffer
//      if (res >= count)
//        return res;
//
//      // add the mapped character to the destination string or '?'(0x1A, SUB) if character
//      // can't be represented in ISO-8859-1
//      *mbstr++ = (u <= 0xFF ? (char)u : '?');
//    }
//    ++res;
//  }
//
//  // add the terminating null character
//  if (mbstr != 0 && count != 0)
//  {
//    // be sure there is enough space left in the destination buffer
//    if (res >= count)
//      return res;
//    *mbstr = 0;
//  }
//
//  return res;
//} // ldap_x_utf8s_to_iso_8859_1s
//
//
///*----------------------------------------------------------------------
//-------
//   Convert a ISO-8859-1 MultiByte string to a UTF-8 string.
//   No more than 'count' bytes will be written to the output buffer.
//   Return the size of the converted string in bytes, excl null
//terminator.
//*/
//int ldap_x_iso_8859_1s_to_utf8s(char *utf8str, const char *mbstr, size_t count)
//{
//  int res = 0;
//
//  // loop until we reach the end of the mb string
//  for (; *mbstr != '\0'; ++mbstr)
//  {
//    // the character needs no mapping if the highest bit is not set
//    if ((*mbstr & 0x80) == 0)
//    {
//      if (utf8str != 0 && count != 0)
//      {
//        // be sure there is enough space left in the destination buffer
//        if (res >= count)
//          return res;
//
//        *utf8str++ = *mbstr;
//      }
//      ++res;
//    }
//
//    // otherwise mapping is necessary
//    else
//    {
//      if (utf8str != 0 && count != 0)
//      {
//        // be sure there is enough space left in the destination buffer
//        if (res+1 >= count)
//          return res;
//
//        *utf8str++ = (0xC0 | (0x03 & (*mbstr >> 6)));
//        *utf8str++ = (0x80 | (0x3F & *mbstr));
//      }
//      res += 2;
//    }
//  }
//
//  // add the terminating null character
//  if (utf8str != 0 && count != 0)
//  {
//    // be sure there is enough space left in the destination buffer
//    if (res >= count)
//      return res;
//    *utf8str = 0;
//  }
//
//  return res;
//} // ldap_x_iso_8859_1s_to_utf8s

void Properties::load(const string &path, bool clearCurrentProperties) {

	//wchar_t lineBuffer[maxLine]=L"";
	char lineBuffer[maxLine]="";
	string line, key, value;
	size_t pos=0;
	this->path= path;

	//std::locale::global(std::locale(""));
	bool is_utf8_language = valid_utf8_file(path.c_str());

#if defined(WIN32) && !defined(__MINGW32__)
	wstring wstr = utf8_decode(path);
	FILE *fp = _wfopen(wstr.c_str(), L"r");
	//wifstream fileStream(fp);
	ifstream fileStream(fp);
#else
	//wifstream fileStream;
	ifstream fileStream;
	fileStream.open(path.c_str(), ios_base::in);
#endif
	
	if(fileStream.is_open() == false){
		if(SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled) SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] path = [%s]\n",__FILE__,__FUNCTION__,__LINE__,path.c_str());
		throw runtime_error("File NOT FOUND, can't open file: [" + path + "]");
	}

	if(clearCurrentProperties == true) {
		propertyMap.clear();
	}

	while(fileStream.eof() == false) {
		lineBuffer[0]='\0';
		fileStream.getline(lineBuffer, maxLine);
		lineBuffer[maxLine-1]='\0';

		//printf("\n[%ls]\n",lineBuffer);
		//printf("\n[%s]\n",&lineBuffer[0]);

		if(lineBuffer[0] != '\0') {
			// If the file is NOT in UTF-8 format convert each line
			if(is_utf8_language == false && Font::forceLegacyFonts == false) {
				char *utfStr = String::ConvertToUTF8(&lineBuffer[0]);

				//printf("\nBefore [%s] After [%s]\n",&lineBuffer[0],utfStr);

				memset(&lineBuffer[0],0,maxLine);
				memcpy(&lineBuffer[0],&utfStr[0],strlen(utfStr));

				delete [] utfStr;
			}
			else if(is_utf8_language == true && Font::forceLegacyFonts == true) {
				char *asciiStr = String::ConvertFromUTF8(&lineBuffer[0]);

				//printf("\nBefore [%s] After [%s]\n",&lineBuffer[0],utfStr);

				memset(&lineBuffer[0],0,maxLine);
				memcpy(&lineBuffer[0],&asciiStr[0],strlen(asciiStr));

				delete [] asciiStr;
			}
		}

//		bool isRLM = utf8::starts_with_rlm(&lineBuffer[0], &lineBuffer[0] + strlen(lineBuffer));
//		if(isRLM) {
//			printf("\n\nORIGINAL TEXT [%s] isRLM = %d\n\n",&lineBuffer[0],isRLM);
//		}

		//if(is_utf8_language == true && Font::forceLegacyFonts == true) {
			//string line = lineBuffer;
			//wstring wstr = fromUtf8(line.c_str(), line.size());

			//vector <unsigned short> utf16result;
			//utf8::utf8to16(line.begin(), line.end(), back_inserter(utf16result));
			//vector <int> utf16result;
			//utf8::utf8to32(line.begin(), line.end(), back_inserter(utf16result));

			//printf("\nConverted UTF-8 from [%s] to [%s]\n",line.c_str(),utf16result[0]);

			//char newBuf[4097]="";
			//int newSize = ldap_x_utf8s_to_iso_8859_1s( &newBuf[0], line.c_str(), 4096 );

			//std::wstring wstr = widen(newBuf);
			//String st(wstr.c_str());
			//String st(line.c_str());

			//printf("\nConverted UTF-8 from [%s] to [%ls]\n",line.c_str(),wstr.c_str());

 		    //const wchar_t *wBuf = &szPath[0];
			//setlocale(LC_ALL, "en_CA.ISO-8559-15");
			//std::locale::global(std::locale("en_CA.ISO-8559-15"));
		    //size_t size = 4096;
		    //char pMBBuffer[4096 + 1]="";
		    //wcstombs(&pMBBuffer[0], &lineBuffer[0], (size_t)size);// Convert to char* from TCHAR[]
	  	    //string newStr="";
	  	    //newStr.assign(&pMBBuffer[0]); // Now assign the char* to the string, and there you have it!!! :)
	  	    //printf("\nConverted UTF-8 from [%ls] to [%s]\n",&lineBuffer[0],newStr.c_str());
	  	    //std::locale::global(std::locale(""));

			//char newBuf[4097]="";
			//int newSize = ldap_x_utf8s_to_iso_8859_1s( &newBuf[0], &pMBBuffer[0], 4096 );

			//String st(&lineBuffer[0]);
			//printf("\nConverted UTF-8 from [%ls] to [%s]\n",&lineBuffer[0],&newBuf[0]);

			//char newBuf[4097]="";
			//int newSize = ldap_x_utf8s_to_iso_8859_1s( &newBuf[0], line.c_str(), 4096 );

			//string newStr = conv_utf8_iso8859_7(line);
			//printf("\nConverted UTF-8 from [%s] to [%s]\n",line.c_str(),newBuf);
//			for(int i = 0; i < line.size(); ++i) {
//				printf("to [%c][%d]\n",line[i],line[i]);
//			}
			//for(int i = 0; i < newStr.size(); ++i) {
			//	printf("to [%c][%d]\n",newStr[i],newStr[i]);
			//}

//			for(int i = 0; i < utf16result.size(); ++i) {
//				printf("to [%c]\n",utf16result[i]);
//			}
//
			//memset(&lineBuffer[0],0,maxLine);
			//memcpy(&lineBuffer[0],&newBuf[0],newSize);
		//}
		//else {
			//string line = lineBuffer;
			//printf("\nNON UTF-8 from [%s]\n",line.c_str());
			//for(int i = 0; i < line.size(); ++i) {
			//	printf("to [%c][%d]\n",line[i],line[i]);
			//}
		//}

		//process line if it it not a comment
		if(lineBuffer[0] != ';') {
			//wstring wstr = lineBuffer;
			//line.assign(wstr.begin(),wstr.end());

			// gracefully handle win32 \r\n line endings
			size_t len= strlen(lineBuffer);
   			if(len > 0 && lineBuffer[len-1] == '\r') {
   				lineBuffer[len-1]= 0;
			}

			line= lineBuffer;
			pos= line.find('=');

			if(pos != string::npos){
				key= line.substr(0, pos);
				value= line.substr(pos+1);

				if(applyTagsToValue(value) == true) {
					if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Property key [%s] now has value [%s]\n",key.c_str(),value.c_str());
				}

				bool replaceExisting = false;
				if(propertyMap.find(key) != propertyMap.end()) {
					replaceExisting = true;
				}
				propertyMap[key] = value;

				if(replaceExisting == false) {
					propertyVector.push_back(PropertyPair(key, value));
				}
				else {
					for(unsigned int i = 0; i < propertyVector.size(); ++i) {
						PropertyPair &currentPair = propertyVector[i];
						if(currentPair.first == key) {
							currentPair.second = value;
							break;
						}
					}
				}
			}
		}
	}

	fileStream.close();
#if defined(WIN32) && !defined(__MINGW32__)
	if(fp) {
		fclose(fp);
	}
#endif
}

std::map<string,string> Properties::getTagReplacementValues(std::map<string,string> *mapExtraTagReplacementValues) {
	std::map<string,string> mapTagReplacementValues;

	//
	// #1
	// First add the standard tags
	//
	char *homeDir = NULL;
#ifdef WIN32
	homeDir = getenv("USERPROFILE");
#else
	homeDir = getenv("HOME");
#endif

	mapTagReplacementValues["~/"] = (homeDir != NULL ? homeDir : "");
	mapTagReplacementValues["$HOME"] = (homeDir != NULL ? homeDir : "");
	mapTagReplacementValues["%%HOME%%"] = (homeDir != NULL ? homeDir : "");
	mapTagReplacementValues["%%USERPROFILE%%"] = (homeDir != NULL ? homeDir : "");
	mapTagReplacementValues["%%HOMEPATH%%"] = (homeDir != NULL ? homeDir : "");

	// For win32 we allow use of the appdata variable since that is the recommended
	// place for application data in windows platform
#ifdef WIN32
	TCHAR szPath[MAX_PATH];
   // Get path for each computer, non-user specific and non-roaming data.
   if ( SUCCEEDED( SHGetFolderPath( NULL, CSIDL_APPDATA,
                                    NULL, 0, szPath))) {

	   //const wchar_t *wBuf = &szPath[0];
	   //size_t size = MAX_PATH + 1;
	   //char pMBBuffer[MAX_PATH + 1]="";
	   //wcstombs_s(&size, &pMBBuffer[0], (size_t)size, wBuf, (size_t)size);// Convert to char* from TCHAR[]
  	   //string appPath="";
	   //appPath.assign(&pMBBuffer[0]); // Now assign the char* to the string, and there you have it!!! :) 
	   std::string appPath = utf8_encode(szPath);

	   //string appPath = szPath;
	   mapTagReplacementValues["$APPDATA"] = appPath;
	   mapTagReplacementValues["%%APPDATA%%"] = appPath;
   }
#endif

	char *username = NULL;
	username = getenv("USERNAME");

	mapTagReplacementValues["$USERNAME"] = (username != NULL ? username : "");
	mapTagReplacementValues["%%USERNAME%%"] = (username != NULL ? username : "");

	mapTagReplacementValues["$APPLICATIONPATH"] = Properties::applicationPath;
	mapTagReplacementValues["%%APPLICATIONPATH%%"] = Properties::applicationPath;

#if defined(CUSTOM_DATA_INSTALL_PATH)
	mapTagReplacementValues["$APPLICATIONDATAPATH"] = CUSTOM_DATA_INSTALL_PATH;
	mapTagReplacementValues["%%APPLICATIONDATAPATH%%"] = CUSTOM_DATA_INSTALL_PATH;

	//mapTagReplacementValues["$COMMONDATAPATH", 	string(CUSTOM_DATA_INSTALL_PATH) + "/commondata/");
	//mapTagReplacementValues["%%COMMONDATAPATH%%",	string(CUSTOM_DATA_INSTALL_PATH) + "/commondata/");

#else
	mapTagReplacementValues["$APPLICATIONDATAPATH"] = Properties::applicationPath;
	mapTagReplacementValues["%%APPLICATIONDATAPATH%%"] = Properties::applicationPath;

	//mapTagReplacementValues["$COMMONDATAPATH", 	Properties::applicationPath + "/commondata/");
	//mapTagReplacementValues["%%COMMONDATAPATH%%",	Properties::applicationPath + "/commondata/");
#endif

	//
	// #2
	// Next add the extra tags if passed in
	//
	if(mapExtraTagReplacementValues != NULL) {
		for(std::map<string,string>::iterator iterMap = mapExtraTagReplacementValues->begin();
				iterMap != mapExtraTagReplacementValues->end(); ++iterMap) {
			mapTagReplacementValues[iterMap->first] = iterMap->second;
		}
	}

	return mapTagReplacementValues;
}

bool Properties::applyTagsToValue(string &value, std::map<string,string> *mapTagReplacementValues) {
	string originalValue = value;

	if(mapTagReplacementValues != NULL) {
		for(std::map<string,string>::iterator iterMap = mapTagReplacementValues->begin();
				iterMap != mapTagReplacementValues->end(); ++iterMap) {
			replaceAll(value, iterMap->first, iterMap->second);
		}
	}
	else {
	char *homeDir = NULL;
#ifdef WIN32
	homeDir = getenv("USERPROFILE");
#else
	homeDir = getenv("HOME");
#endif

	replaceAll(value, "~/", 			(homeDir != NULL ? homeDir : ""));
	replaceAll(value, "$HOME", 			(homeDir != NULL ? homeDir : ""));
	replaceAll(value, "%%HOME%%", 		(homeDir != NULL ? homeDir : ""));
	replaceAll(value, "%%USERPROFILE%%",(homeDir != NULL ? homeDir : ""));
	replaceAll(value, "%%HOMEPATH%%",	(homeDir != NULL ? homeDir : ""));

	// For win32 we allow use of the appdata variable since that is the recommended
	// place for application data in windows platform
#ifdef WIN32
	TCHAR szPath[MAX_PATH];
   // Get path for each computer, non-user specific and non-roaming data.
   if ( SUCCEEDED( SHGetFolderPath( NULL, CSIDL_APPDATA, 
                                    NULL, 0, szPath))) {
	   //const wchar_t *wBuf = &szPath[0];
	   //size_t size = MAX_PATH + 1;
	   //char pMBBuffer[MAX_PATH + 1]="";
	   //wcstombs_s(&size, &pMBBuffer[0], (size_t)size, wBuf, (size_t)size);// Convert to char* from TCHAR[]
  	   //string appPath="";
	   //appPath.assign(&pMBBuffer[0]); // Now assign the char* to the string, and there you have it!!! :) 
	   std::string appPath = utf8_encode(szPath);

		//string appPath = szPath;
       replaceAll(value, "$APPDATA", appPath);
	   replaceAll(value, "%%APPDATA%%", appPath);
   }
#endif

	char *username = NULL;
	username = getenv("USERNAME");
	replaceAll(value, "$USERNAME", 		(username != NULL ? username : ""));
	replaceAll(value, "%%USERNAME%%", 	(username != NULL ? username : ""));

	replaceAll(value, "$APPLICATIONPATH", 		Properties::applicationPath);
	replaceAll(value, "%%APPLICATIONPATH%%",	Properties::applicationPath);

#if defined(CUSTOM_DATA_INSTALL_PATH)
	replaceAll(value, "$APPLICATIONDATAPATH", 		CUSTOM_DATA_INSTALL_PATH);
	replaceAll(value, "%%APPLICATIONDATAPATH%%",	CUSTOM_DATA_INSTALL_PATH);

	//replaceAll(value, "$COMMONDATAPATH", 	string(CUSTOM_DATA_INSTALL_PATH) + "/commondata/");
	//replaceAll(value, "%%COMMONDATAPATH%%",	string(CUSTOM_DATA_INSTALL_PATH) + "/commondata/");

#else
	replaceAll(value, "$APPLICATIONDATAPATH", 		Properties::applicationPath);
	replaceAll(value, "%%APPLICATIONDATAPATH%%",	Properties::applicationPath);

	//replaceAll(value, "$COMMONDATAPATH", 	Properties::applicationPath + "/commondata/");
	//replaceAll(value, "%%COMMONDATAPATH%%",	Properties::applicationPath + "/commondata/");
#endif

	}

	//printf("\nBEFORE SUBSTITUTE [%s] AFTER [%s]\n",originalValue.c_str(),value.c_str());
	return (originalValue != value);
}

void Properties::save(const string &path){
#if defined(WIN32) && !defined(__MINGW32__)
	FILE *fp = _wfopen(utf8_decode(path).c_str(), L"w");
	ofstream fileStream(fp);
#else
	ofstream fileStream;
	fileStream.open(path.c_str(), ios_base::out | ios_base::trunc);
#endif
	fileStream << "; === propertyMap File === \n";
	fileStream << '\n';

	for(PropertyMap::iterator pi= propertyMap.begin(); pi!=propertyMap.end(); ++pi){
		fileStream << pi->first << '=' << pi->second << '\n';
	}

	fileStream.close();
#if defined(WIN32) && !defined(__MINGW32__)
	fclose(fp);
#endif
}

void Properties::clear(){
	propertyMap.clear();
	propertyVector.clear();
}

bool Properties::getBool(const string &key, const char *defaultValueIfNotFound) const{
	try{
		return strToBool(getString(key,defaultValueIfNotFound));
	}
	catch(exception &e){
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		throw runtime_error("Error accessing value: " + key + " in: " + path+"\n[" + e.what() + "]");
	}
}

int Properties::getInt(const string &key,const char *defaultValueIfNotFound) const{
	try{
		return strToInt(getString(key,defaultValueIfNotFound));
	}
	catch(exception &e){
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		throw runtime_error("Error accessing value: " + key + " in: " + path + "\n[" + e.what() + "]");
	}
}

int Properties::getInt(const string &key, int min, int max,const char *defaultValueIfNotFound) const{
	int i= getInt(key,defaultValueIfNotFound);
	if(i<min || i>max){
		throw runtime_error("Value out of range: " + key + ", min: " + intToStr(min) + ", max: " + intToStr(max));
	}
	return i;
}

float Properties::getFloat(const string &key, const char *defaultValueIfNotFound) const{
	try{
		return strToFloat(getString(key,defaultValueIfNotFound));
	}
	catch(exception &e){
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		throw runtime_error("Error accessing value: " + key + " in: " + path + "\n[" + e.what() + "]");
	}
}

float Properties::getFloat(const string &key, float min, float max, const char *defaultValueIfNotFound) const{
	float f= getFloat(key,defaultValueIfNotFound);
	if(f<min || f>max){
		throw runtime_error("Value out of range: " + key + ", min: " + floatToStr(min) + ", max: " + floatToStr(max));
	}
	return f;
}

const string Properties::getString(const string &key, const char *defaultValueIfNotFound) const{
	PropertyMap::const_iterator it;
	it= propertyMap.find(key);
	if(it==propertyMap.end()){
	    if(defaultValueIfNotFound != NULL) {
	        //printf("In [%s::%s - %d]defaultValueIfNotFound = [%s]\n",__FILE__,__FUNCTION__,__LINE__,defaultValueIfNotFound);
	        return string(defaultValueIfNotFound);
	    }
	    else {
            throw runtime_error("Value not found in propertyMap: " + key + ", loaded from: " + path);
	    }
	}
	else{
		return (it->second != "" ? it->second : (defaultValueIfNotFound != NULL ? defaultValueIfNotFound : it->second));
	}
}

void Properties::setInt(const string &key, int value){
	setString(key, intToStr(value));
}

void Properties::setBool(const string &key, bool value){
	setString(key, boolToStr(value));
}

void Properties::setFloat(const string &key, float value){
	setString(key, floatToStr(value));
}

void Properties::setString(const string &key, const string &value){
	propertyMap.erase(key);
	propertyMap.insert(PropertyPair(key, value));
}

string Properties::toString(){
	string rStr;

	for(PropertyMap::iterator pi= propertyMap.begin(); pi!=propertyMap.end(); pi++)
		rStr+= pi->first + "=" + pi->second + "\n";

	return rStr;
}




bool Properties::getBool(const char *key, const char *defaultValueIfNotFound) const{
	try{
		return strToBool(getString(key,defaultValueIfNotFound));
	}
	catch(exception &e){
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		throw runtime_error("Error accessing value: " + string(key) + " in: " + path+"\n[" + e.what() + "]");
	}
}

int Properties::getInt(const char *key,const char *defaultValueIfNotFound) const{
	try{
		return strToInt(getString(key,defaultValueIfNotFound));
	}
	catch(exception &e){
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		throw runtime_error("Error accessing value: " + string(key) + " in: " + path + "\n[" + e.what() + "]");
	}
}

float Properties::getFloat(const char *key, const char *defaultValueIfNotFound) const{
	try{
		return strToFloat(getString(key,defaultValueIfNotFound));
	}
	catch(exception &e){
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		throw runtime_error("Error accessing value: " + string(key) + " in: " + path + "\n[" + e.what() + "]");
	}
}

const string Properties::getString(const char *key, const char *defaultValueIfNotFound) const{
	PropertyMap::const_iterator it;
	it= propertyMap.find(key);
	if(it==propertyMap.end()){
	    if(defaultValueIfNotFound != NULL) {
	        //printf("In [%s::%s - %d]defaultValueIfNotFound = [%s]\n",__FILE__,__FUNCTION__,__LINE__,defaultValueIfNotFound);
	        return string(defaultValueIfNotFound);
	    }
	    else {
            throw runtime_error("Value not found in propertyMap: " + string(key) + ", loaded from: " + path);
	    }
	}
	else{
		return (it->second != "" ? it->second : (defaultValueIfNotFound != NULL ? defaultValueIfNotFound : it->second));
	}
}


}}//end namepsace
