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
  const char *filepath;
  FILE *stream;
  FTPClientThread *ftpServer;
};

static size_t my_fwrite(void *buffer, size_t size, size_t nmemb, void *stream) {
    struct FtpFile *out=(struct FtpFile *)stream;

    string fullFilePath = "";
    if(out != NULL && out->filepath != NULL) {
        fullFilePath = out->filepath;
    }
    if(out != NULL && out->filename != NULL) {
        fullFilePath += out->filename;
    }

    // Abort file xfer and delete partial file
    if(out && out->ftpServer && out->ftpServer->getQuitStatus() == true) {
        if(out->stream) {
            fclose(out->stream);
            out->stream = NULL;
        }

        if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> FTP Client thread CANCELLED, deleting file for writing [%s]\n",fullFilePath.c_str());
        unlink(fullFilePath.c_str());
        return -1;
    }

    if(out && out->stream == NULL) {
        if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> FTP Client thread opening file for writing [%s]\n",fullFilePath.c_str());

        /* open file for writing */
        out->stream = fopen(fullFilePath.c_str(), "wb");
        if(out->stream == NULL) {
          if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> FTP Client thread FAILED to open file for writing [%s]\n",fullFilePath.c_str());
          return -1; /* failure, can't open file to write */
        }
    }
    return fwrite(buffer, size, nmemb, out->stream);
}

static long file_is_comming(struct curl_fileinfo *finfo,void *data,int remains) {
    struct FtpFile *out=(struct FtpFile *)data;

    string rootFilePath = "";
    string fullFilePath = "";
    if(out != NULL && out->filepath != NULL) {
        rootFilePath = out->filepath;
    }
    if(out != NULL && out->filename != NULL) {
        fullFilePath = rootFilePath + finfo->filename;
    }

    if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\n===> FTP Client thread file_is_comming: remains: [%3d] filename: [%s] size: [%10luB] ", remains, finfo->filename,(unsigned long)finfo->size);

    switch(finfo->filetype) {
        case CURLFILETYPE_DIRECTORY:
            printf("DIR (creating [%s%s])\n",rootFilePath.c_str(),finfo->filename);
            rootFilePath += finfo->filename;
            createDirectoryPaths(rootFilePath.c_str());
            break;
        case CURLFILETYPE_FILE:
            printf("FILE ");
            break;
        default:
            printf("OTHER\n");
            break;
    }

    if(finfo->filetype == CURLFILETYPE_FILE) {
        // do not transfer files >= 50B
        //if(finfo->size > 50) {
        //  printf("SKIPPED\n");
        //  return CURL_CHUNK_BGN_FUNC_SKIP;
        //}

        if(SystemFlags::VERBOSE_MODE_ENABLED) printf(" opening file [%s] ", fullFilePath.c_str());

        out->stream = fopen(fullFilePath.c_str(), "wb");
        if(out->stream == NULL) {
            return CURL_CHUNK_BGN_FUNC_FAIL;
        }
    }

  return CURL_CHUNK_BGN_FUNC_OK;
}

static long file_is_downloaded(void *data) {
    struct FtpFile *out=(struct FtpFile *)data;
    if(out->stream) {
        printf("DOWNLOAD COMPLETE!\n");

        fclose(out->stream);
        out->stream = NULL;
    }
    return CURL_CHUNK_END_FUNC_OK;
}

FTPClientThread::FTPClientThread(int portNumber, string serverUrl, std::pair<string,string> mapsPath, std::pair<string,string> tilesetsPath, FTPClientCallbackInterface *pCBObject) : BaseThread() {
    this->portNumber    = portNumber;
    this->serverUrl     = serverUrl;
    this->mapsPath      = mapsPath;
    this->tilesetsPath  = tilesetsPath;
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
        if(SystemFlags::VERBOSE_MODE_ENABLED) curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        CURLcode res = curl_easy_perform(curl);
        if(CURLE_OK != res) {
          // we failed
          printf("curl FAILED with: %d\n", res);
        }
        else {
            result = ftp_crt_SUCCESS;
        }

        curl_easy_cleanup(curl);
    }

    if(ftpfile.stream) {
        fclose(ftpfile.stream);
        ftpfile.stream = NULL;

        if(result != ftp_crt_SUCCESS) {
            unlink(destFile.c_str());
        }
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
       this->pCBObject->FTPClient_CallbackEvent(mapFileName,ftp_cct_Map,result);
    }
}

void FTPClientThread::addMapToRequests(string mapFilename) {
    MutexSafeWrapper safeMutex(&mutexMapFileList);
    if(std::find(mapFileList.begin(),mapFileList.end(),mapFilename) == mapFileList.end()) {
        mapFileList.push_back(mapFilename);
    }
}

void FTPClientThread::addTilesetToRequests(string tileSetName) {
    MutexSafeWrapper safeMutex(&mutexTilesetList);
    if(std::find(tilesetList.begin(),tilesetList.end(),tileSetName) == tilesetList.end()) {
        tilesetList.push_back(tileSetName);
    }
}

void FTPClientThread::getTilesetFromServer(string tileSetName) {
    FTP_Client_ResultType result = getTilesetFromServer(tileSetName, "", "tilsets_custom", "mg_ftp_server");
    if(result != ftp_crt_SUCCESS && this->getQuitStatus() == false) {
        result = getTilesetFromServer(tileSetName, "tilesets", "", "mg_ftp_server");
    }

    if(this->pCBObject != NULL) {
       this->pCBObject->FTPClient_CallbackEvent(tileSetName,ftp_cct_Tileset,result);
    }
}

FTP_Client_ResultType FTPClientThread::getTilesetFromServer(string tileSetName, string tileSetNameSubfolder, string ftpUser, string ftpUserPassword) {
    FTP_Client_ResultType result = ftp_crt_FAIL;

    string destFile = this->tilesetsPath.second;

    // Root folder for the tileset
    string destRootFolder = "";
    if(tileSetNameSubfolder == "") {
        destRootFolder = this->tilesetsPath.second;
        if(EndsWith(destRootFolder,"/") == false && EndsWith(destRootFolder,"\\") == false) {
             destRootFolder += "/";
        }
        destRootFolder += tileSetName;
        if(EndsWith(destRootFolder,"/") == false && EndsWith(destRootFolder,"\\") == false) {
             destRootFolder += "/";
        }

        createDirectoryPaths(destRootFolder);
    }

    if(EndsWith(destFile,"/") == false && EndsWith(destFile,"\\") == false) {
        destFile += "/";
    }
    destFile += tileSetName;
    if(EndsWith(destFile,"/") == false && EndsWith(destFile,"\\") == false) {
        destFile += "/";
    }

    if(tileSetNameSubfolder != "") {
        destFile += tileSetNameSubfolder;

        if(EndsWith(destFile,"/") == false && EndsWith(destFile,"\\") == false) {
            destFile += "/";
        }
    }

    if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> FTP Client thread about to try to RETR into [%s]\n",destFile.c_str());

    struct FtpFile ftpfile = {
        destFile.c_str(), // name to store the file as if succesful
        destFile.c_str(),
        NULL,
        this
    };

    //curl_global_init(CURL_GLOBAL_DEFAULT);

    CURL *curl = curl_easy_init();
    if(curl) {
        ftpfile.stream = NULL;

        char szBuf[1024]="";
        if(tileSetNameSubfolder == "") {
            sprintf(szBuf,"ftp://%s:%s@%s:%d/%s/*",ftpUser.c_str(),ftpUserPassword.c_str(),serverUrl.c_str(),portNumber,tileSetName.c_str());
        }
        else {
            sprintf(szBuf,"ftp://%s:%s@%s:%d/%s/%s/*",ftpUser.c_str(),ftpUserPassword.c_str(),serverUrl.c_str(),portNumber,tileSetName.c_str(),tileSetNameSubfolder.c_str());
        }

        curl_easy_setopt(curl, CURLOPT_URL,szBuf);

        // turn on wildcard matching
        curl_easy_setopt(curl, CURLOPT_WILDCARDMATCH, 1L);

        // callback is called before download of concrete file started
        curl_easy_setopt(curl, CURLOPT_CHUNK_BGN_FUNCTION, file_is_comming);

        // callback is called after data from the file have been transferred
        curl_easy_setopt(curl, CURLOPT_CHUNK_END_FUNCTION, file_is_downloaded);

        // put transfer data into callbacks
        // helper data
        //struct callback_data data = { 0 };
        curl_easy_setopt(curl, CURLOPT_CHUNK_DATA, &ftpfile);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ftpfile);

        // Define our callback to get called when there's data to be written
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_fwrite);
        // Set a pointer to our struct to pass to the callback
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ftpfile);

        // Switch on full protocol/debug output
        if(SystemFlags::VERBOSE_MODE_ENABLED) curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        CURLcode res = curl_easy_perform(curl);

        if(CURLE_OK != res) {
          // we failed
          printf("curl FAILED with: %d\n", res);

          if(destRootFolder != "") {
              unlink(destRootFolder.c_str());
          }
        }
        else {
            result = ftp_crt_SUCCESS;

            bool requireMoreFolders = false;
            if(tileSetNameSubfolder == "") {
                tileSetNameSubfolder = "models";
                requireMoreFolders = true;
            }
            else if(tileSetNameSubfolder == "models") {
                tileSetNameSubfolder = "sounds";
                requireMoreFolders = true;
            }
            else if(tileSetNameSubfolder == "sounds") {
                tileSetNameSubfolder = "textures";
                requireMoreFolders = true;
            }
            else if(tileSetNameSubfolder == "textures") {
                tileSetNameSubfolder = "";
                requireMoreFolders = false;
            }

            if(requireMoreFolders == true) {
                FTP_Client_ResultType result2 = getTilesetFromServer(tileSetName, tileSetNameSubfolder, ftpUser, ftpUserPassword);
            }
        }

        curl_easy_cleanup(curl);
    }

    if(ftpfile.stream) {
        fclose(ftpfile.stream);
        ftpfile.stream = NULL;
    }
    return result;
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

                if(this->getQuitStatus() == true) {
                    break;
                }

                MutexSafeWrapper safeMutex2(&mutexTilesetList);
                if(tilesetList.size() > 0) {
                    string tileset = tilesetList[0];
                    tilesetList.erase(tilesetList.begin() + 0);
                    safeMutex2.ReleaseLock();

                    getTilesetFromServer(tileset);
                }
                else {
                    safeMutex2.ReleaseLock();
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
