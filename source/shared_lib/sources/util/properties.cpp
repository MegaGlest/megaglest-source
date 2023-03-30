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

#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "conversion.h"
#include "platform_common.h"
#include "platform_util.h"
#include "randomgen.h"
#include "util.h"

#ifdef WIN32

#include <shlobj.h>
#include <shlwapi.h>

#endif

#include "font.h"
#include "leak_dumper.h"
#include "string_utils.h"
#include "utf8.h"

using namespace std;
using namespace Shared::PlatformCommon;
using namespace Shared::Platform;
using namespace Shared::Graphics;

namespace Shared {
namespace Util {

#ifndef NO_APPIMAGE
string Properties::appDir = "";
#ifdef APPIMAGE_NODATA
string Properties::appimageDir = "";
#endif
#endif

string Properties::applicationPath = "";
string Properties::applicationDataPath = "";
string Properties::gameVersion = "";

string Properties::techtreePath = "";
string Properties::scenarioPath = "";
string Properties::tutorialPath = "";

// =====================================================
//	class Properties
// =====================================================

void Properties::load(const string &path, bool clearCurrentProperties) {

  char lineBuffer[maxLine] = "";
  this->path = path;

  bool is_utf8_language = valid_utf8_file(path.c_str());

#if defined(WIN32) && !defined(__MINGW32__)
  wstring wstr = utf8_decode(path);
  FILE *fp = _wfopen(wstr.c_str(), L"r");
  // wifstream fileStream(fp);
  ifstream fileStream(fp);
#else
  // wifstream fileStream;
  ifstream fileStream;
  fileStream.open(path.c_str(), ios_base::in);
#endif

  if (fileStream.is_open() == false) {
    if (SystemFlags::getSystemSettingType(SystemFlags::debugSystem).enabled)
      SystemFlags::OutputDebug(SystemFlags::debugSystem,
                               "In [%s::%s Line: %d] path = [%s]\n",
                               extractFileFromDirectoryPath(__FILE__).c_str(),
                               __FUNCTION__, __LINE__, path.c_str());
    throw megaglest_runtime_error("File NOT FOUND, can't open file: [" + path +
                                  "]");
  }

  if (clearCurrentProperties == true) {
    propertyMap.clear();
    propertyMapTmp.clear();
  }

  while (fileStream.eof() == false) {
    lineBuffer[0] = '\0';
    fileStream.getline(lineBuffer, maxLine);
    lineBuffer[maxLine - 1] = '\0';

    processTextLine(is_utf8_language, lineBuffer);
  }

  fileStream.close();
#if defined(WIN32) && !defined(__MINGW32__)
  if (fp) {
    fclose(fp);
  }
#endif
}

void Properties::loadFromText(const string &text) {
  bool is_utf8_language = false;

  char lineBuffer[maxLine] = "";

  std::istringstream textStream(text);
  while (textStream.eof() == false) {
    // lineBuffer[0]='\0';
    std::string lineText;
    getline(textStream, lineText);
    if (lineText.length() > 0) {
      memset(&lineBuffer[0], 0, maxLine);
      memcpy(&lineBuffer[0], &lineText[0], lineText.length());

      // fileStream.getline(lineBuffer, maxLine);
      // lineBuffer[maxLine-1]='\0';

      processTextLine(is_utf8_language, &lineBuffer[0]);
    }
  }
}

void Properties::processTextLine(bool is_utf8_language, char *lineBuffer) {
  // printf("\n[%ls]\n",lineBuffer);
  // printf("\n[%s]\n",&lineBuffer[0]);

  string line, key, value, original_value;
  size_t pos = 0;

  if (lineBuffer != NULL && lineBuffer[0] != '\0') {
    // If the file is NOT in UTF-8 format convert each line
    if (is_utf8_language == false && Font::forceLegacyFonts == false) {
      char *utfStr = ConvertToUTF8(&lineBuffer[0]);

      // printf("\nBefore [%s] After [%s]\n",&lineBuffer[0],utfStr);

      memset(&lineBuffer[0], 0, maxLine);
      memcpy(&lineBuffer[0], &utfStr[0], strlen(utfStr));

      delete[] utfStr;
    } else if (is_utf8_language == true && Font::forceLegacyFonts == true) {
      char *asciiStr = ConvertFromUTF8(&lineBuffer[0]);

      // printf("\nBefore [%s] After [%s]\n",&lineBuffer[0],utfStr);

      memset(&lineBuffer[0], 0, maxLine);
      memcpy(&lineBuffer[0], &asciiStr[0], strlen(asciiStr));

      delete[] asciiStr;
    }
  }

  // process line if it it not a comment
  if (lineBuffer != NULL && lineBuffer[0] != ';' && lineBuffer[0] != '#') {
    // gracefully handle win32 \r\n line endings
    size_t len = strlen(lineBuffer);
    if (len > 0 && lineBuffer[len - 1] == '\r') {
      lineBuffer[len - 1] = 0;
    }

    line = lineBuffer;
    pos = line.find('=');

    if (pos != string::npos) {
      key = line.substr(0, pos);
      value = line.substr(pos + 1);
      original_value = value;

      if (applyTagsToValue(value) == true) {
        if (SystemFlags::VERBOSE_MODE_ENABLED)
          printf("Property key [%s] now has value [%s] original_value [%s]\n",
                 key.c_str(), value.c_str(), original_value.c_str());
      }

      bool replaceExisting = false;
      if (propertyMap.find(key) != propertyMap.end()) {
        replaceExisting = true;
      }
      propertyMap[key] = original_value;
      propertyMapTmp[key] = value;

      if (replaceExisting == false) {
        propertyVector.push_back(PropertyPair(key, original_value));
        propertyVectorTmp.push_back(PropertyPair(key, value));
      } else {
        for (unsigned int i = 0; i < propertyVector.size(); ++i) {
          PropertyPair &currentPair = propertyVector[i];
          if (currentPair.first == key) {
            currentPair.second = original_value;

            propertyVectorTmp[i].second = value;
            break;
          }
        }
      }
    }
  }
}

#ifndef NO_APPIMAGE
void Properties::setAppDirPath() {
  char *appDirChar = getenv("APPDIR");
  appDir = appDirChar == NULL ? "" : appDirChar;
}

#ifdef APPIMAGE_NODATA
void Properties::setAppimageDirPath() {
  char *appimageDirChar = getenv("APPIMAGE");
  if (appimageDirChar != NULL) {
    appimageDir = appimageDirChar;
    appimageDir = appimageDir.substr(0, appimageDir.find_last_of("/") + 1);
  }
}
#endif

string Properties::appendAppDirPath(string value) {
  if (!Properties::appDir.empty() && value[0] == '/')
    return Properties::appDir + value;
  return value;
}

string Properties::appendAppImagePath(string value) {
#ifdef APPIMAGE_NODATA
  if (!Properties::appimageDir.empty())
    return Properties::appimageDir;
#endif
  return appendAppDirPath(value);
}
#endif

std::map<string, string> Properties::getTagReplacementValues(
    std::map<string, string> *mapExtraTagReplacementValues) {
  std::map<string, string> mapTagReplacementValues;

  //
  // #1
  // First add the standard tags
  //
#ifdef WIN32
  const char *homeDirX = getenv("USERPROFILE");
#else
  string home = getUserHome();
  const char *homeDirX = home.c_str();
#endif

  string homeDir = safeCharPtrCopy(homeDirX, 8096);

  mapTagReplacementValues["~/"] = homeDir;
  mapTagReplacementValues["$HOME"] = homeDir;
  mapTagReplacementValues["%%HOME%%"] = homeDir;
  mapTagReplacementValues["%%USERPROFILE%%"] = homeDir;
  mapTagReplacementValues["%%HOMEPATH%%"] = homeDir;
  mapTagReplacementValues["{HOMEPATH}"] = homeDir;

  // For win32 we allow use of the appdata variable since that is the
  // recommended place for application data in windows platform
#ifdef WIN32
  TCHAR szPath[MAX_PATH];
  // Get path for each computer, non-user specific and non-roaming data.
  if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, szPath))) {

    // const wchar_t *wBuf = &szPath[0];
    // size_t size = MAX_PATH + 1;
    // char pMBBuffer[MAX_PATH + 1]="";
    // wcstombs_s(&size, &pMBBuffer[0], (size_t)size, wBuf, (size_t)size);//
    // Convert to char* from TCHAR[] string appPath="";
    // appPath.assign(&pMBBuffer[0]); // Now assign the char* to the string, and
    // there you have it!!! :)
    std::string appPath = utf8_encode(szPath);

    // string appPath = szPath;
    mapTagReplacementValues["$APPDATA"] = appPath;
    mapTagReplacementValues["%%APPDATA%%"] = appPath;
    mapTagReplacementValues["{APPDATA}"] = appPath;
  }
#endif

  // char *username = NULL;
  char *username = getenv("USERNAME");

  mapTagReplacementValues["$USERNAME"] = (username != NULL ? username : "");
  mapTagReplacementValues["%%USERNAME%%"] = (username != NULL ? username : "");
  mapTagReplacementValues["{USERNAME}"] = (username != NULL ? username : "");

  mapTagReplacementValues["$APPLICATIONPATH"] = Properties::applicationPath;
  mapTagReplacementValues["%%APPLICATIONPATH%%"] = Properties::applicationPath;
  mapTagReplacementValues["{APPLICATIONPATH}"] = Properties::applicationPath;

#if defined(CUSTOM_DATA_INSTALL_PATH)
  mapTagReplacementValues["$APPLICATIONDATAPATH"] =
      formatPath(TOSTRING(CUSTOM_DATA_INSTALL_PATH));
  mapTagReplacementValues["%%APPLICATIONDATAPATH%%"] =
      formatPath(TOSTRING(CUSTOM_DATA_INSTALL_PATH));
  mapTagReplacementValues["{APPLICATIONDATAPATH}"] =
      formatPath(TOSTRING(CUSTOM_DATA_INSTALL_PATH));

#else
  mapTagReplacementValues["$APPLICATIONDATAPATH"] =
      Properties::applicationDataPath;
  mapTagReplacementValues["%%APPLICATIONDATAPATH%%"] =
      Properties::applicationDataPath;
  mapTagReplacementValues["{APPLICATIONDATAPATH}"] =
      Properties::applicationDataPath;

#endif

  mapTagReplacementValues["{TECHTREEPATH}"] = Properties::techtreePath;
  mapTagReplacementValues["{SCENARIOPATH}"] = Properties::scenarioPath;
  mapTagReplacementValues["{TUTORIALPATH}"] = Properties::tutorialPath;

  mapTagReplacementValues["$GAMEVERSION"] = Properties::gameVersion;

  //
  // #2
  // Next add the extra tags if passed in
  //
  if (mapExtraTagReplacementValues != NULL) {
    for (std::map<string, string>::iterator iterMap =
             mapExtraTagReplacementValues->begin();
         iterMap != mapExtraTagReplacementValues->end(); ++iterMap) {
      mapTagReplacementValues[iterMap->first] = iterMap->second;
    }
  }

  return mapTagReplacementValues;
}

bool Properties::isValuePathVariable(const string &value) {
  if (value.find("~/") != value.npos || value.find("$HOME") != value.npos ||
      value.find("%%HOME%%") != value.npos ||
      value.find("%%USERPROFILE%%") != value.npos ||
      value.find("%%HOMEPATH%%") != value.npos ||
      value.find("{HOMEPATH}") != value.npos ||
      value.find("$APPDATA") != value.npos ||
      value.find("%%APPDATA%%") != value.npos ||
      value.find("{APPDATA}") != value.npos ||
      value.find("$APPLICATIONPATH") != value.npos ||
      value.find("%%APPLICATIONPATH%%") != value.npos ||
      value.find("{APPLICATIONPATH}") != value.npos ||
      value.find("$APPLICATIONDATAPATH") != value.npos ||
      value.find("%%APPLICATIONDATAPATH%%") != value.npos ||
      value.find("{APPLICATIONDATAPATH}") != value.npos ||
      value.find("{TECHTREEPATH}") != value.npos ||
      value.find("{SCENARIOPATH}") != value.npos ||
      value.find("{TUTORIALPATH}") != value.npos) {

    return true;
  }
  return false;
}
void Properties::updateValuePathVariable(string &value,
                                         bool skipUpdatePathClimbingParts) {
  replaceAll(value, "//", "/");
  replaceAll(value, "\\\\", "\\");
  if (skipUpdatePathClimbingParts == false) {
    updatePathClimbingParts(value);
  }
}

bool Properties::applyTagsToValue(
    string &value, const std::map<string, string> *mapTagReplacementValues,
    bool skipUpdatePathClimbingParts) {
  string originalValue = value;
  bool valueRequiresPathUpdate = Properties::isValuePathVariable(value);
  // if(originalValue.find("$APPLICATIONDATAPATH") != string::npos) {
  //	printf("\nBEFORE SUBSTITUTE [%s] app [%s] mapTagReplacementValues
  //[%p]\n",originalValue.c_str(),Properties::applicationPath.c_str(),mapTagReplacementValues);
  // }

  if (mapTagReplacementValues != NULL) {
    for (std::map<string, string>::const_iterator iterMap =
             mapTagReplacementValues->begin();
         iterMap != mapTagReplacementValues->end(); ++iterMap) {

      //			if(value.find("{SCENARIOPATH}") != string::npos)
      //{ 				printf("\n\n** WILL REPLACE
      //[%s]\n",value.c_str()); replaceAll(value,
      //"{SCENARIOPATH}",		Properties::scenarioPath);
      //printf("** REPLACED [%s]\n\n",value.c_str());
      //			}
      //			else {
      replaceAll(value, iterMap->first, iterMap->second);
      //			}
    }
  } else {
#ifdef WIN32
    const char *homeDirX = getenv("USERPROFILE");
#else
    string home = getUserHome();
    const char *homeDirX = home.c_str();
#endif

    string homeDir = safeCharPtrCopy(homeDirX, 8096);

    replaceAll(value, "~/", homeDir);
    replaceAll(value, "$HOME", homeDir);
    replaceAll(value, "%%HOME%%", homeDir);
    replaceAll(value, "%%USERPROFILE%%", homeDir);
    replaceAll(value, "%%HOMEPATH%%", homeDir);
    replaceAll(value, "{HOMEPATH}", homeDir);

    // For win32 we allow use of the appdata variable since that is the
    // recommended place for application data in windows platform
#ifdef WIN32
    TCHAR szPath[MAX_PATH];
    // Get path for each computer, non-user specific and non-roaming data.
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, szPath))) {
      // const wchar_t *wBuf = &szPath[0];
      // size_t size = MAX_PATH + 1;
      // char pMBBuffer[MAX_PATH + 1]="";
      // wcstombs_s(&size, &pMBBuffer[0], (size_t)size, wBuf, (size_t)size);//
      // Convert to char* from TCHAR[] string appPath="";
      // appPath.assign(&pMBBuffer[0]); // Now assign the char* to the string,
      // and there you have it!!! :)
      std::string appPath = utf8_encode(szPath);

      // string appPath = szPath;
      replaceAll(value, "$APPDATA", appPath);
      replaceAll(value, "%%APPDATA%%", appPath);
      replaceAll(value, "{APPDATA}", appPath);
    }
#endif

    // char *username = NULL;
    char *username = getenv("USERNAME");
    replaceAll(value, "$USERNAME", (username != NULL ? username : ""));
    replaceAll(value, "%%USERNAME%%", (username != NULL ? username : ""));
    replaceAll(value, "{USERNAME}", (username != NULL ? username : ""));

    replaceAll(value, "$APPLICATIONPATH", Properties::applicationPath);
    replaceAll(value, "%%APPLICATIONPATH%%", Properties::applicationPath);
    replaceAll(value, "{APPLICATIONPATH}", Properties::applicationPath);

#if defined(CUSTOM_DATA_INSTALL_PATH)
#ifndef NO_APPIMAGE
    replaceAll(
        value, "$APPLICATIONDATAPATH",
        appendAppImagePath(formatPath(TOSTRING(CUSTOM_DATA_INSTALL_PATH))));
    replaceAll(
        value, "%%APPLICATIONDATAPATH%%",
        appendAppImagePath(formatPath(TOSTRING(CUSTOM_DATA_INSTALL_PATH))));
    replaceAll(
        value, "{APPLICATIONDATAPATH}",
        appendAppImagePath(formatPath(TOSTRING(CUSTOM_DATA_INSTALL_PATH))));
#else
    replaceAll(value, "$APPLICATIONDATAPATH",
               formatPath(TOSTRING(CUSTOM_DATA_INSTALL_PATH)));
    replaceAll(value, "%%APPLICATIONDATAPATH%%",
               formatPath(TOSTRING(CUSTOM_DATA_INSTALL_PATH)));
    replaceAll(value, "{APPLICATIONDATAPATH}",
               formatPath(TOSTRING(CUSTOM_DATA_INSTALL_PATH)));
#endif

#else
    replaceAll(value, "$APPLICATIONDATAPATH", Properties::applicationDataPath);
    replaceAll(value, "%%APPLICATIONDATAPATH%%",
               Properties::applicationDataPath);
    replaceAll(value, "{APPLICATIONDATAPATH}", Properties::applicationDataPath);

#endif

    replaceAll(value, "{TECHTREEPATH}", Properties::techtreePath);
    //	if(value.find("{SCENARIOPATH}") != string::npos) {
    //		printf("\n\n** WILL REPLACE [%s]\n",value.c_str());
    replaceAll(value, "{SCENARIOPATH}", Properties::scenarioPath);
    //		printf("** REPLACED [%s]\n\n",value.c_str());
    //	}
    replaceAll(value, "{TUTORIALPATH}", Properties::tutorialPath);

    replaceAll(value, "$GAMEVERSION", Properties::gameVersion);
  }

  // if(originalValue != value || originalValue.find("$APPLICATIONDATAPATH") !=
  // string::npos) { 	printf("\nBEFORE SUBSTITUTE [%s] AFTER
  //[%s]\n",originalValue.c_str(),value.c_str());
  // }

  if (valueRequiresPathUpdate == true) {
    Properties::updateValuePathVariable(value, skipUpdatePathClimbingParts);
  }
  return (originalValue != value);
}

void Properties::save(const string &path) {
#if defined(WIN32) && !defined(__MINGW32__)
  FILE *fp = _wfopen(utf8_decode(path).c_str(), L"w");
  ofstream fileStream(fp);
#else
  ofstream fileStream;
  fileStream.open(path.c_str(), ios_base::out | ios_base::trunc);
#endif
  fileStream << "; === propertyMap File === \n";
  fileStream << '\n';

  for (PropertyMap::iterator pi = propertyMap.begin(); pi != propertyMap.end();
       ++pi) {
    fileStream << pi->first << '=' << pi->second << '\n';
  }

  fileStream.close();
#if defined(WIN32) && !defined(__MINGW32__)
  if (fp)
    fclose(fp);
#endif
}

void Properties::clear() {
  propertyMap.clear();
  propertyMapTmp.clear();
  propertyVector.clear();
  propertyVectorTmp.clear();
}

int Properties::getPropertyCount() const {
  return (int)propertyVectorTmp.size();
}
string Properties::getKey(int i) const { return propertyVectorTmp[i].first; }
string Properties::getString(int i) const {
  return propertyVectorTmp[i].second;
}

bool Properties::getBool(const string &key,
                         const char *defaultValueIfNotFound) const {
  try {
    return strToBool(getString(key, defaultValueIfNotFound));
  } catch (exception &e) {
    SystemFlags::OutputDebug(SystemFlags::debugError,
                             "In [%s::%s Line: %d] Error [%s]\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__, e.what());
    // throw megaglest_runtime_error("Error accessing value: " + key + " in: " +
    // path+"\n[" + e.what() + "]");
    throw runtime_error("Error accessing value: " + key + " in: " + path +
                        "\n[" + e.what() + "]");
  }
  return false;
}

int Properties::getInt(const string &key,
                       const char *defaultValueIfNotFound) const {
  try {
    return strToInt(getString(key, defaultValueIfNotFound));
  } catch (exception &e) {
    SystemFlags::OutputDebug(SystemFlags::debugError,
                             "In [%s::%s Line: %d] Error [%s]\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__, e.what());
    // throw megaglest_runtime_error("Error accessing value: " + key + " in: " +
    // path + "\n[" + e.what() + "]");
    throw runtime_error("Error accessing value: " + key + " in: " + path +
                        "\n[" + e.what() + "]");
  }
  return 0;
}

int Properties::getInt(const string &key, int min, int max,
                       const char *defaultValueIfNotFound) const {
  int i = getInt(key, defaultValueIfNotFound);
  if (i < min || i > max) {
    throw megaglest_runtime_error("Value out of range: " + key + ", min: " +
                                  intToStr(min) + ", max: " + intToStr(max));
  }
  return i;
}

float Properties::getFloat(const string &key,
                           const char *defaultValueIfNotFound) const {
  float result = 0.0;
  try {
    result = strToFloat(getString(key, defaultValueIfNotFound));
  } catch (exception &e) {
    SystemFlags::OutputDebug(SystemFlags::debugError,
                             "In [%s::%s Line: %d] Error [%s]\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__, e.what());
    // throw megaglest_runtime_error("Error accessing value: " + key + " in: " +
    // path + "\n[" + e.what() + "]");
    throw runtime_error("Error accessing value: " + key + " in: " + path +
                        "\n[" + e.what() + "]");
  }
  return result;
}

float Properties::getFloat(const string &key, float min, float max,
                           const char *defaultValueIfNotFound) const {
  float f = getFloat(key, defaultValueIfNotFound);
  if (f < min || f > max) {
    throw megaglest_runtime_error("Value out of range: " + key +
                                  ", min: " + floatToStr(min, 16) +
                                  ", max: " + floatToStr(max, 16));
  }
  return f;
}

const string Properties::getString(const string &key,
                                   const char *defaultValueIfNotFound) const {
  PropertyMap::const_iterator it = propertyMapTmp.find(key);
  if (it == propertyMapTmp.end()) {
    if (defaultValueIfNotFound != NULL) {
      // printf("In [%s::%s - %d]defaultValueIfNotFound =
      // [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,defaultValueIfNotFound);
      return string(defaultValueIfNotFound);
    } else {
      // throw megaglest_runtime_error("Value not found in propertyMap: " + key
      // + ", loaded from: " + path);
      throw runtime_error("Value not found in propertyMap: " + key +
                          ", loaded from: " + path);
    }
  } else {
    return (it->second != ""
                ? it->second
                : (defaultValueIfNotFound != NULL ? defaultValueIfNotFound
                                                  : it->second));
  }
}

const string Properties::getRandomKey(const bool realrandom) const {
  PropertyMap::const_iterator it;
  int max = getPropertyCount();
  int randomIndex = -1;
  if (realrandom == true) {
    Chrono seed(true);
    srand((unsigned int)seed.getCurTicks());

    randomIndex = rand() % max;
  } else {
    RandomGen randgen;
    randomIndex = randgen.randRange(0, max);
  }
  string s = getKey(randomIndex);
  return (s != "" ? s : "nothing found");
}

bool Properties::hasString(const string &key) const {
  PropertyMap::const_iterator it = propertyMapTmp.find(key);
  if (it == propertyMapTmp.end()) {
    return false;
  }
  return true;
}

void Properties::setInt(const string &key, int value) {
  setString(key, intToStr(value));
}

void Properties::setBool(const string &key, bool value) {
  setString(key, boolToStr(value));
}

void Properties::setFloat(const string &key, float value) {
  setString(key, floatToStr(value, 6));
}

void Properties::setString(const string &key, const string &value) {
  propertyMap.erase(key);
  propertyMap.insert(PropertyPair(key, value));

  propertyMapTmp.erase(key);
  propertyMapTmp.insert(PropertyPair(key, value));
}

string Properties::toString() {
  string rStr;

  for (PropertyMap::iterator pi = propertyMapTmp.begin();
       pi != propertyMapTmp.end(); ++pi)
    rStr += pi->first + "=" + pi->second + "\n";

  return rStr;
}

bool Properties::getBool(const char *key,
                         const char *defaultValueIfNotFound) const {
  try {
    return strToBool(getString(key, defaultValueIfNotFound));
  } catch (exception &e) {
    SystemFlags::OutputDebug(SystemFlags::debugError,
                             "In [%s::%s Line: %d] Error [%s]\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__, e.what());
    // throw megaglest_runtime_error("Error accessing value: " + string(key) + "
    // in: " + path+"\n[" + e.what() + "]");
    throw runtime_error("Error accessing value: " + string(key) +
                        " in: " + path + "\n[" + e.what() + "]");
  }
  return false;
}

int Properties::getInt(const char *key,
                       const char *defaultValueIfNotFound) const {
  try {
    return strToInt(getString(key, defaultValueIfNotFound));
  } catch (exception &e) {
    SystemFlags::OutputDebug(SystemFlags::debugError,
                             "In [%s::%s Line: %d] Error [%s]\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__, e.what());
    // throw megaglest_runtime_error("Error accessing value: " + string(key) + "
    // in: " + path + "\n[" + e.what() + "]");
    throw runtime_error("Error accessing value: " + string(key) +
                        " in: " + path + "\n[" + e.what() + "]");
  }
  return 0;
}

float Properties::getFloat(const char *key,
                           const char *defaultValueIfNotFound) const {
  float result = 0.0;
  try {
    result = strToFloat(getString(key, defaultValueIfNotFound));
  } catch (exception &e) {
    SystemFlags::OutputDebug(SystemFlags::debugError,
                             "In [%s::%s Line: %d] Error [%s]\n",
                             extractFileFromDirectoryPath(__FILE__).c_str(),
                             __FUNCTION__, __LINE__, e.what());
    // throw megaglest_runtime_error("Error accessing value: " + string(key) + "
    // in: " + path + "\n[" + e.what() + "]");
    throw runtime_error("Error accessing value: " + string(key) +
                        " in: " + path + "\n[" + e.what() + "]");
  }
  return result;
}

const string Properties::getString(const char *key,
                                   const char *defaultValueIfNotFound) const {
  PropertyMap::const_iterator it = propertyMapTmp.find(key);
  if (it == propertyMapTmp.end()) {
    if (defaultValueIfNotFound != NULL) {
      // printf("In [%s::%s - %d]defaultValueIfNotFound =
      // [%s]\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,defaultValueIfNotFound);
      return string(defaultValueIfNotFound);
    } else {
      // throw megaglest_runtime_error("Value not found in propertyMap: " +
      // string(key) + ", loaded from: " + path);
      throw runtime_error("Value not found in propertyMap: " + string(key) +
                          ", loaded from: " + path);
    }
  } else {
    return (it->second != ""
                ? it->second
                : (defaultValueIfNotFound != NULL ? defaultValueIfNotFound
                                                  : it->second));
  }
}

} // namespace Util
} // namespace Shared
