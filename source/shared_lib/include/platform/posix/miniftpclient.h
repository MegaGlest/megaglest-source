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
    ftp_crt_SUCCESS = 0,
    ftp_crt_FAIL = 1
};

class FTPClientCallbackInterface {
public:
    virtual void FTPClient_CallbackEvent(string mapFilename, FTP_Client_ResultType result) = 0;
};

class FTPClientThread : public BaseThread
{
protected:
    int portNumber;
    string serverUrl;
    FTPClientCallbackInterface *pCBObject;
    std::pair<string,string> mapsPath;

    Mutex mutexMapFileList;
    vector<string> mapFileList;
    void getMapFromServer(string mapFilename);
    FTP_Client_ResultType getMapFromServer(string mapFileName, string ftpUser, string ftpUserPassword);

public:

    FTPClientThread(int portNumber,string serverUrl, std::pair<string,string> mapsPath, FTPClientCallbackInterface *pCBObject);
    virtual void execute();
    virtual void signalQuit();
    virtual bool shutdownAndWait();

    void addMapToRequests(string mapFilename);
};

}}//end namespace

#endif
