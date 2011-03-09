// ==============================================================
//	This file is part of MegaGlest Shared Library (www.glest.org)
//
//	Copyright (C) 2009-2010 Titus Tscharntke (info@titusgames.de) and
//                          Mark Vejvoda (mark_vejvoda@hotmail.com)
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================
#ifndef _SHARED_PLATFORMCOMMON_MINIFTPCLIENTTHREAD_H_
#define _SHARED_PLATFORMCOMMON_MINIFTPCLIENTTHREAD_H_

#include "base_thread.h"
#include <vector>
#include <string>

#include "leak_dumper.h"

using namespace std;

namespace Shared { namespace PlatformCommon {

// =====================================================
//	class FTPClientThread
// =====================================================

enum FTP_Client_ResultType {
    ftp_crt_SUCCESS 	= 0,
    ftp_crt_PARTIALFAIL = 1,
    ftp_crt_FAIL    	= 2,
    ftp_crt_ABORTED 	= 3
};

enum FTP_Client_CallbackType {
    ftp_cct_Map                 = 0,
    ftp_cct_Tileset             = 1,
    ftp_cct_Techtree            = 2,
    ftp_cct_DownloadProgress    = 3
};

class FTPClientCallbackInterface {
public:

    struct FtpProgressStats {
      double download_total;
      double download_now;
      double upload_total;
      double upload_now;
      string currentFilename;
    };

    virtual void FTPClient_CallbackEvent(string itemName, FTP_Client_CallbackType type, FTP_Client_ResultType result, void *userdata) = 0;
};

class FTPClientThread : public BaseThread
{
protected:
    int portNumber;
    string serverUrl;
    FTPClientCallbackInterface *pCBObject;
    std::pair<string,string> mapsPath;
    std::pair<string,string> tilesetsPath;
    std::pair<string,string> techtreesPath;

    Mutex mutexMapFileList;
    vector<string> mapFileList;

    Mutex mutexTilesetList;
    vector<string> tilesetList;

    Mutex mutexTechtreeList;
    vector<string> techtreeList;

    void getMapFromServer(string mapFilename);
    FTP_Client_ResultType getMapFromServer(string mapFileName, string ftpUser, string ftpUserPassword);

    void getTilesetFromServer(string tileSetName);
    FTP_Client_ResultType getTilesetFromServer(string tileSetName, string tileSetNameSubfolder, string ftpUser, string ftpUserPassword, bool findArchive);

    void getTechtreeFromServer(string techtreeName);
    FTP_Client_ResultType getTechtreeFromServer(string techtreeName, string ftpUser, string ftpUserPassword);

    Mutex mutexProgressMutex;

    string fileArchiveExtension;
    string fileArchiveExtractCommand;
    string fileArchiveExtractCommandParameters;

public:

    FTPClientThread(int portNumber,string serverUrl,
    				std::pair<string,string> mapsPath,
    				std::pair<string,string> tilesetsPath,
    				std::pair<string,string> techtreesPath,
    				FTPClientCallbackInterface *pCBObject,
    				string fileArchiveExtension,
    				string fileArchiveExtractCommand,
    				string fileArchiveExtractCommandParameters);
    virtual void execute();
    virtual void signalQuit();
    virtual bool shutdownAndWait();

    void addMapToRequests(string mapFilename);
    void addTilesetToRequests(string tileSetName);
    void addTechtreeToRequests(string techtreeName);

    FTPClientCallbackInterface * getCallBackObject();
    void setCallBackObject(FTPClientCallbackInterface *value);

    Mutex * getProgressMutex() { return &mutexProgressMutex; }
};

}}//end namespace

#endif
