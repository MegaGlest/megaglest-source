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
#include "platform_common.h"
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
    ftp_crt_ABORTED 	= 3,
    ftp_crt_HOST_NOT_ACCEPTING = 4
};

enum FTP_Client_CallbackType {
    ftp_cct_Map                 = 0,
    ftp_cct_Tileset             = 1,
    ftp_cct_Techtree            = 2,
    ftp_cct_Scenario           	= 3,
    ftp_cct_File           		= 4,
    ftp_cct_DownloadProgress    = 5,
    ftp_cct_ExtractProgress     = 6
};

class FTPClientCallbackInterface {
public:
	virtual ~FTPClientCallbackInterface() {}
    struct FtpProgressStats {
      double download_total;
      double download_now;
      double upload_total;
      double upload_now;
      string currentFilename;
      FTP_Client_CallbackType downloadType;
    };

    virtual void FTPClient_CallbackEvent(string itemName,
    										 FTP_Client_CallbackType type,
    										 pair<FTP_Client_ResultType,string> result,
    										 void *userdata) = 0;
};

class FTPClientThread : public BaseThread, public ShellCommandOutputCallbackInterface
{
protected:
    int portNumber;
    string serverUrl;
    FTPClientCallbackInterface *pCBObject;
    std::pair<string,string> mapsPath;
    std::pair<string,string> tilesetsPath;
    std::pair<string,string> techtreesPath;
    std::pair<string,string> scenariosPath;

    Mutex mutexMapFileList;
    vector<pair<string,string> > mapFileList;

    Mutex mutexTilesetList;
    vector<pair<string,string> > tilesetList;

    Mutex mutexTechtreeList;
    vector<pair<string,string> > techtreeList;

    Mutex mutexScenarioList;
    vector<pair<string,string> > scenarioList;

    Mutex mutexFileList;
    vector<pair<string,string> > fileList;

    void getMapFromServer(pair<string,string> mapFilename);
    pair<FTP_Client_ResultType,string> getMapFromServer(pair<string,string> mapFileName, string ftpUser, string ftpUserPassword);

    void getTilesetFromServer(pair<string,string> tileSetName);
    pair<FTP_Client_ResultType,string> getTilesetFromServer(pair<string,string> tileSetName, string tileSetNameSubfolder, string ftpUser, string ftpUserPassword, bool findArchive);

    void getTechtreeFromServer(pair<string,string> techtreeName);
    pair<FTP_Client_ResultType,string> getTechtreeFromServer(pair<string,string> techtreeName, string ftpUser, string ftpUserPassword);

    void getScenarioFromServer(pair<string,string> fileName);
    pair<FTP_Client_ResultType,string> getScenarioInternalFromServer(pair<string,string> fileName);

    void getFileFromServer(pair<string,string> fileName);
    pair<FTP_Client_ResultType,string> getFileInternalFromServer(pair<string,string> fileName);

    Mutex mutexProgressMutex;

    string fileArchiveExtension;
    string fileArchiveExtractCommand;
    string fileArchiveExtractCommandParameters;
    int fileArchiveExtractCommandSuccessResult;

    pair<FTP_Client_ResultType,string> getFileFromServer(FTP_Client_CallbackType downloadType,
    		pair<string,string> fileNameTitle,
    		string remotePath, string destFileSaveAs, string ftpUser,
    		string ftpUserPassword, vector <string> *wantDirListOnly=NULL);

    string shellCommandCallbackUserData;
    virtual void * getShellCommandOutput_UserData(string cmd);
    virtual void ShellCommandOutput_CallbackEvent(string cmd,char *output,void *userdata);

public:

    FTPClientThread(int portNumber,string serverUrl,
    				std::pair<string,string> mapsPath,
    				std::pair<string,string> tilesetsPath,
    				std::pair<string,string> techtreesPath,
    				std::pair<string,string> scenariosPath,
    				FTPClientCallbackInterface *pCBObject,
    				string fileArchiveExtension,
    				string fileArchiveExtractCommand,
    				string fileArchiveExtractCommandParameters,
    				int fileArchiveExtractCommandSuccessResult);
    virtual void execute();
    virtual void signalQuit();
    virtual bool shutdownAndWait();

    void addMapToRequests(string mapFilename,string URL="");
    void addTilesetToRequests(string tileSetName,string URL="");
    void addTechtreeToRequests(string techtreeName,string URL="");
    void addScenarioToRequests(string fileName,string URL="");
    void addFileToRequests(string fileName,string URL="");

    FTPClientCallbackInterface * getCallBackObject();
    void setCallBackObject(FTPClientCallbackInterface *value);

    Mutex * getProgressMutex() { return &mutexProgressMutex; }
};

}}//end namespace

#endif
