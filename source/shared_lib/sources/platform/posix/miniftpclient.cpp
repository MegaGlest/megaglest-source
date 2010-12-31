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

#include "miniftpclient.h"
#include "util.h"
#include "platform_common.h"

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>
#include <algorithm>

using namespace Shared::Util;
using namespace Shared::PlatformCommon;

namespace Shared { namespace PlatformCommon {

/*
 * This is an example showing how to get a single file from an FTP server.
 * It delays the actual destination file creation until the first write
 * callback so that it won't create an empty file in case the remote file
 * doesn't exist or something else fails.
 */

struct FtpFile {
  const char *filename;
  FILE *stream;
  FTPClientThread *ftpServer;
};

static size_t my_fwrite(void *buffer, size_t size, size_t nmemb, void *stream) {
  struct FtpFile *out=(struct FtpFile *)stream;

  // Abort file xfer and delete partial file
  if(out && out->ftpServer && out->ftpServer->getQuitStatus() == true) {
    if(out->stream) {
        fclose(out->stream);
        out->stream = NULL;
    }

    unlink(out->filename);

    return -1;
  }

  if(out && out->stream == NULL) {
    if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> FTP Client thread opening file for writing [%s]\n",out->filename);

    /* open file for writing */
    out->stream=fopen(out->filename, "wb");
    if(out->stream == NULL) {
      if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> FTP Client thread FAILED to open file for writing [%s]\n",out->filename);

      return -1; /* failure, can't open file to write */
    }
  }
  return fwrite(buffer, size, nmemb, out->stream);
}

FTPClientThread::FTPClientThread(int portNumber, string serverUrl, std::pair<string,string> mapsPath, FTPClientCallbackInterface *pCBObject) : BaseThread() {
    this->portNumber    = portNumber;
    this->serverUrl     = serverUrl;
    this->mapsPath      = mapsPath;
    this->pCBObject     = pCBObject;
}

void FTPClientThread::signalQuit() {
    if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> FTP Client: signalQuit\n");
    BaseThread::signalQuit();
}

bool FTPClientThread::shutdownAndWait() {
    if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> FTP Client: shutdownAndWait\n");

    signalQuit();
    return BaseThread::shutdownAndWait();
}


FTP_Client_ResultType FTPClientThread::getMapFromServer(string mapFileName, string ftpUser, string ftpUserPassword) {
    CURLcode res;

    FTP_Client_ResultType result = ftp_crt_FAIL;

    string destFileExt = "";
    string destFile = this->mapsPath.second;

    if(EndsWith(destFile,"/") == false && EndsWith(destFile,"\\") == false) {
        destFile += "/";
    }
    destFile += mapFileName;

    if(EndsWith(destFile,".mgm") == false && EndsWith(destFile,".gbm") == false) {
        destFileExt = ".mgm";
        destFile += destFileExt;
    }

    if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> FTP Client thread about to try to RETR into [%s]\n",destFile.c_str());

    struct FtpFile ftpfile = {
        destFile.c_str(), /* name to store the file as if succesful */
        NULL,
        this
    };

    //curl_global_init(CURL_GLOBAL_DEFAULT);

    CURL *curl = curl_easy_init();
    if(curl) {
        ftpfile.stream = NULL;

        char szBuf[1024]="";
        sprintf(szBuf,"ftp://%s:%s@%s:%d/%s%s",ftpUser.c_str(),ftpUserPassword.c_str(),serverUrl.c_str(),portNumber,mapFileName.c_str(),destFileExt.c_str());

        curl_easy_setopt(curl, CURLOPT_URL,szBuf);
        /* Define our callback to get called when there's data to be written */
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_fwrite);
        /* Set a pointer to our struct to pass to the callback */
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ftpfile);

        /* Switch on full protocol/debug output */
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        res = curl_easy_perform(curl);

        if(CURLE_OK != res) {
          // we failed
          fprintf(stderr, "curl told us %d\n", res);
        }
        else {
            result = ftp_crt_SUCCESS;
        }

        curl_easy_cleanup(curl);
    }

    if(ftpfile.stream) {
        fclose(ftpfile.stream);
        ftpfile.stream = NULL;
    }
    return result;
}


void FTPClientThread::getMapFromServer(string mapFileName) {
    FTP_Client_ResultType result = getMapFromServer(mapFileName + ".mgm", "maps_custom", "mg_ftp_server");
    if(result != ftp_crt_SUCCESS && this->getQuitStatus() == false) {
        result = getMapFromServer(mapFileName + ".gbm", "maps_custom", "mg_ftp_server");
        if(result != ftp_crt_SUCCESS && this->getQuitStatus() == false) {
            result = getMapFromServer(mapFileName + ".mgm", "maps", "mg_ftp_server");
            if(result != ftp_crt_SUCCESS && this->getQuitStatus() == false) {
                result = getMapFromServer(mapFileName + ".gbm", "maps", "mg_ftp_server");
            }
        }
    }

    if(this->pCBObject != NULL) {
       this->pCBObject->FTPClient_CallbackEvent(mapFileName,result);
    }
}

void FTPClientThread::addMapToRequests(string mapFilename) {
    MutexSafeWrapper safeMutex(&mutexMapFileList);
    if(std::find(mapFileList.begin(),mapFileList.end(),mapFilename) == mapFileList.end()) {
        mapFileList.push_back(mapFilename);
    }
}


void FTPClientThread::execute() {
    {
        RunningStatusSafeWrapper runningStatus(this);
        SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

        if(getQuitStatus() == true) {
            return;
        }

        if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> FTP Client thread is running\n");
        SystemFlags::OutputDebug(SystemFlags::debugNetwork,"FTP Client thread is running\n");

        try	{
            while(this->getQuitStatus() == false) {
                MutexSafeWrapper safeMutex(&mutexMapFileList);
                if(mapFileList.size() > 0) {
                    string mapFilename = mapFileList[0];
                    mapFileList.erase(mapFileList.begin() + 0);
                    safeMutex.ReleaseLock();

                    getMapFromServer(mapFilename);
                }
                else {
                    safeMutex.ReleaseLock();
                }

                sleep(25);
            }

            if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> FTP Client exiting!\n");
        }
        catch(const exception &ex) {
            SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
            SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
        }
        catch(...) {
            SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] UNKNOWN Error\n",__FILE__,__FUNCTION__,__LINE__);
            SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] unknown error\n",__FILE__,__FUNCTION__,__LINE__);
        }

        SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] FTP Client thread is exiting\n",__FILE__,__FUNCTION__,__LINE__);
    }

    // Delete ourself when the thread is done (no other actions can happen after this
    // such as the mutex which modifies the running status of this method
	//delete this;
}

}}//end namespace
