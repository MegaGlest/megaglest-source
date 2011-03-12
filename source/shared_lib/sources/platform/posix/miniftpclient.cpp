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
#include "conversion.h"

using namespace Shared::Util;
using namespace Shared::PlatformCommon;

namespace Shared { namespace PlatformCommon {

const char *FTP_MAPS_CUSTOM_USERNAME        = "maps_custom";
const char *FTP_MAPS_USERNAME               = "maps";
const char *FTP_TILESETS_CUSTOM_USERNAME    = "tilesets_custom";
const char *FTP_TILESETS_USERNAME           = "tilesets";
const char *FTP_TECHTREES_CUSTOM_USERNAME   = "techtrees_custom";
const char *FTP_TECHTREES_USERNAME          = "techtrees";

const char *FTP_COMMON_PASSWORD             = "mg_ftp_server";

/*
 * This is an example showing how to get a single file from an FTP server.
 * It delays the actual destination file creation until the first write
 * callback so that it won't create an empty file in case the remote file
 * doesn't exist or something else fails.
 */

struct FtpFile {
  const char *itemName;
  const char *filename;
  const char *filepath;
  FILE *stream;
  FTPClientThread *ftpServer;
  string currentFilename;
  bool isValidXfer;
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
        SystemFlags::OutputDebug(SystemFlags::debugNetwork,"===> FTP Client thread CANCELLED, deleting file for writing [%s]\n",fullFilePath.c_str());


        removeFile(fullFilePath);
        return -1;
    }

    if(out && out->stream == NULL) {
        if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> FTP Client thread opening file for writing [%s]\n",fullFilePath.c_str());
        SystemFlags::OutputDebug(SystemFlags::debugNetwork,"===> FTP Client thread opening file for writing [%s]\n",fullFilePath.c_str());

        /* open file for writing */
        out->stream = fopen(fullFilePath.c_str(), "wb");
        if(out->stream == NULL) {
          if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> FTP Client thread FAILED to open file for writing [%s]\n",fullFilePath.c_str());
          SystemFlags::OutputDebug(SystemFlags::debugNetwork,"===> FTP Client thread FAILED to open file for writing [%s]\n",fullFilePath.c_str());
          return -1; /* failure, can't open file to write */
        }

        out->isValidXfer = true;
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
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"===> FTP Client thread file_is_comming: remains: [%3d] filename: [%s] size: [%10luB] ", remains, finfo->filename,(unsigned long)finfo->size);

    if(out != NULL) {
        //out->currentFilename = finfo->filename;
        out->currentFilename = fullFilePath;

        if(SystemFlags::VERBOSE_MODE_ENABLED) printf(" current filename: [%s] ", fullFilePath.c_str());
        SystemFlags::OutputDebug(SystemFlags::debugNetwork,"current filename: [%s] ", fullFilePath.c_str());
    }

    switch(finfo->filetype) {
        case CURLFILETYPE_DIRECTORY:
            if(SystemFlags::VERBOSE_MODE_ENABLED) printf(" DIR (creating [%s%s])\n",rootFilePath.c_str(),finfo->filename);
            SystemFlags::OutputDebug(SystemFlags::debugNetwork," DIR (creating [%s%s])\n",rootFilePath.c_str(),finfo->filename);

            rootFilePath += finfo->filename;
            createDirectoryPaths(rootFilePath.c_str());
            break;
        case CURLFILETYPE_FILE:
            if(SystemFlags::VERBOSE_MODE_ENABLED) printf(" FILE ");
            SystemFlags::OutputDebug(SystemFlags::debugNetwork," FILE ");
            break;
        default:
            if(SystemFlags::VERBOSE_MODE_ENABLED) printf(" OTHER\n");
            SystemFlags::OutputDebug(SystemFlags::debugNetwork," OTHER\n");
            break;
    }

    if(finfo->filetype == CURLFILETYPE_FILE) {
        // do not transfer files >= 50B
        //if(finfo->size > 50) {
        //  printf("SKIPPED\n");
        //  return CURL_CHUNK_BGN_FUNC_SKIP;
        //}

        if(SystemFlags::VERBOSE_MODE_ENABLED) printf(" opening file [%s] ", fullFilePath.c_str());
        SystemFlags::OutputDebug(SystemFlags::debugNetwork," opening file [%s] ", fullFilePath.c_str());

        out->stream = fopen(fullFilePath.c_str(), "wb");
        if(out->stream == NULL) {
            if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> FTP Client thread FAILED to open file for writing [%s]\n",fullFilePath.c_str());
            SystemFlags::OutputDebug(SystemFlags::debugNetwork,"===> FTP Client thread FAILED to open file for writing [%s]\n",fullFilePath.c_str());

            return CURL_CHUNK_BGN_FUNC_FAIL;
        }
    }

    out->isValidXfer = true;
    return CURL_CHUNK_BGN_FUNC_OK;
}

static long file_is_downloaded(void *data) {
    struct FtpFile *out=(struct FtpFile *)data;
    if(out->stream) {
        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("DOWNLOAD COMPLETE!\n");
        SystemFlags::OutputDebug(SystemFlags::debugNetwork,"DOWNLOAD COMPLETE!\n");

        fclose(out->stream);
        out->stream = NULL;
    }
    return CURL_CHUNK_END_FUNC_OK;
}

int file_progress(struct FtpFile *out,double download_total, double download_now, double upload_total,double upload_now) {
  //if(SystemFlags::VERBOSE_MODE_ENABLED) printf(" download progress [%f][%f][%f][%f] ",download_total,download_now,upload_total,upload_now);
  SystemFlags::OutputDebug(SystemFlags::debugNetwork," download progress [%f][%f][%f][%f] ",download_total,download_now,upload_total,upload_now);

  if(out != NULL &&
     out->ftpServer != NULL &&
     out->ftpServer->getCallBackObject() != NULL) {
         if(out->ftpServer->getQuitStatus() == true) {
             if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> FTP Client thread CANCELLED\n");
             SystemFlags::OutputDebug(SystemFlags::debugNetwork,"===> FTP Client thread CANCELLED\n");

             return -1;
         }
         FTPClientCallbackInterface::FtpProgressStats stats;
         stats.download_total   = download_total;
         stats.download_now     = download_now;
         stats.upload_total     = upload_total;
         stats.upload_now       = upload_now;
         stats.currentFilename  = out->currentFilename;

         MutexSafeWrapper safeMutex(out->ftpServer->getProgressMutex(),string(__FILE__) + "_" + intToStr(__LINE__));
         out->ftpServer->getCallBackObject()->FTPClient_CallbackEvent(out->itemName, ftp_cct_DownloadProgress, ftp_crt_SUCCESS, &stats);
  }

  return 0;
}

FTPClientThread::FTPClientThread(int portNumber, string serverUrl,
		std::pair<string,string> mapsPath,
		std::pair<string,string> tilesetsPath,
		std::pair<string,string> techtreesPath,
		FTPClientCallbackInterface *pCBObject,
		string fileArchiveExtension,
		string fileArchiveExtractCommand,
		string fileArchiveExtractCommandParameters) : BaseThread() {
    this->portNumber    = portNumber;
    this->serverUrl     = serverUrl;
    this->mapsPath      = mapsPath;
    this->tilesetsPath  = tilesetsPath;
    this->techtreesPath = techtreesPath;
    this->pCBObject     = pCBObject;

    this->fileArchiveExtension = fileArchiveExtension;
    this->fileArchiveExtractCommand = fileArchiveExtractCommand;
    this->fileArchiveExtractCommandParameters = fileArchiveExtractCommandParameters;

    SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line %d] Using FTP port #: %d, serverUrl [%s]\n",__FILE__,__FUNCTION__,__LINE__,portNumber,serverUrl.c_str());
}

void FTPClientThread::signalQuit() {
    if(SystemFlags::VERBOSE_MODE_ENABLED) printf("===> FTP Client: signalQuit\n");
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"===> FTP Client: signalQuit\n");
    BaseThread::signalQuit();
}

bool FTPClientThread::shutdownAndWait() {
    if(SystemFlags::VERBOSE_MODE_ENABLED) printf("===> FTP Client: shutdownAndWait\n");
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"===> FTP Client: shutdownAndWait\n");

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

    if(SystemFlags::VERBOSE_MODE_ENABLED) printf("===> FTP Client thread about to try to RETR into [%s]\n",destFile.c_str());
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"===> FTP Client thread about to try to RETR into [%s]\n",destFile.c_str());

    struct FtpFile ftpfile = {
        NULL,
        destFile.c_str(), /* name to store the file as if succesful */
        NULL,
        NULL,
        this,
        "",
        false
    };

    //curl_global_init(CURL_GLOBAL_DEFAULT);

    CURL *curl = SystemFlags::initHTTP();
    if(curl) {
        ftpfile.stream = NULL;

        char szBuf[1024]="";
        sprintf(szBuf,"ftp://%s:%s@%s:%d/%s%s",ftpUser.c_str(),ftpUserPassword.c_str(),serverUrl.c_str(),portNumber,mapFileName.c_str(),destFileExt.c_str());

        curl_easy_setopt(curl, CURLOPT_URL,szBuf);
        curl_easy_setopt(curl, CURLOPT_FTP_USE_EPSV, 0L);

        /* Define our callback to get called when there's data to be written */
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_fwrite);
        /* Set a pointer to our struct to pass to the callback */
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ftpfile);

        // Max 10 minutes to transfer
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 600);

        /* Switch on full protocol/debug output */
        if(SystemFlags::VERBOSE_MODE_ENABLED) curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        CURLcode res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
          // we failed
          printf("curl FAILED with: %d [%s] szBuf [%s]\n", res,curl_easy_strerror(res),szBuf);
          SystemFlags::OutputDebug(SystemFlags::debugNetwork,"curl FAILED with: %d [%s] szBuf [%s]\n", res,curl_easy_strerror(res),szBuf);

          if(res == CURLE_PARTIAL_FILE) {
        	  result = ftp_crt_PARTIALFAIL;
          }
        }
        else {
            result = ftp_crt_SUCCESS;
        }

        SystemFlags::cleanupHTTP(&curl);
    }

    if(ftpfile.stream) {
        fclose(ftpfile.stream);
        ftpfile.stream = NULL;
    }
    if(result != ftp_crt_SUCCESS) {
    	removeFile(destFile);
    }

    return result;
}

void FTPClientThread::getMapFromServer(string mapFileName) {
    FTP_Client_ResultType result = getMapFromServer(mapFileName + ".mgm", FTP_MAPS_CUSTOM_USERNAME, FTP_COMMON_PASSWORD);
    if(result == ftp_crt_FAIL && this->getQuitStatus() == false) {
        result = getMapFromServer(mapFileName + ".gbm", FTP_MAPS_CUSTOM_USERNAME, FTP_COMMON_PASSWORD);
        if(result == ftp_crt_FAIL && this->getQuitStatus() == false) {
            result = getMapFromServer(mapFileName + ".mgm", FTP_MAPS_USERNAME, FTP_COMMON_PASSWORD);
            if(result == ftp_crt_FAIL && this->getQuitStatus() == false) {
                result = getMapFromServer(mapFileName + ".gbm", FTP_MAPS_USERNAME, FTP_COMMON_PASSWORD);
            }
        }
    }

    MutexSafeWrapper safeMutex(this->getProgressMutex(),string(__FILE__) + "_" + intToStr(__LINE__));
    if(this->pCBObject != NULL) {
        this->pCBObject->FTPClient_CallbackEvent(mapFileName,ftp_cct_Map,result,NULL);
    }
}

void FTPClientThread::addMapToRequests(string mapFilename) {
    MutexSafeWrapper safeMutex(&mutexMapFileList,string(__FILE__) + "_" + intToStr(__LINE__));
    if(std::find(mapFileList.begin(),mapFileList.end(),mapFilename) == mapFileList.end()) {
        mapFileList.push_back(mapFilename);
    }
}

void FTPClientThread::addTilesetToRequests(string tileSetName) {
    MutexSafeWrapper safeMutex(&mutexTilesetList,string(__FILE__) + "_" + intToStr(__LINE__));
    if(std::find(tilesetList.begin(),tilesetList.end(),tileSetName) == tilesetList.end()) {
        tilesetList.push_back(tileSetName);
    }
}

void FTPClientThread::addTechtreeToRequests(string techtreeName) {
    MutexSafeWrapper safeMutex(&mutexTechtreeList,string(__FILE__) + "_" + intToStr(__LINE__));
    if(std::find(techtreeList.begin(),techtreeList.end(),techtreeName) == techtreeList.end()) {
    	techtreeList.push_back(techtreeName);
    }
}

void FTPClientThread::getTilesetFromServer(string tileSetName) {
	bool findArchive = executeShellCommand(this->fileArchiveExtractCommand);

    FTP_Client_ResultType result = getTilesetFromServer(tileSetName, "", FTP_TILESETS_CUSTOM_USERNAME, FTP_COMMON_PASSWORD, findArchive);
    if(result == ftp_crt_FAIL && this->getQuitStatus() == false) {
    	if(findArchive == true) {
    		result = getTilesetFromServer(tileSetName, "", FTP_TILESETS_CUSTOM_USERNAME, FTP_COMMON_PASSWORD, false);
    	}
    	if(result == ftp_crt_FAIL && this->getQuitStatus() == false) {
    		result = getTilesetFromServer(tileSetName, "", FTP_TILESETS_USERNAME, FTP_COMMON_PASSWORD, findArchive);

        	if(findArchive == true) {
        		result = getTilesetFromServer(tileSetName, "", FTP_TILESETS_USERNAME, FTP_COMMON_PASSWORD, false);
        	}
    	}
    }

    MutexSafeWrapper safeMutex(this->getProgressMutex(),string(__FILE__) + "_" + intToStr(__LINE__));
    if(this->pCBObject != NULL) {
        this->pCBObject->FTPClient_CallbackEvent(tileSetName,ftp_cct_Tileset,result,NULL);
    }
}

FTP_Client_ResultType FTPClientThread::getTilesetFromServer(string tileSetName,
		string tileSetNameSubfolder, string ftpUser, string ftpUserPassword,
		bool findArchive) {
    FTP_Client_ResultType result = ftp_crt_FAIL;

    string destFile = this->tilesetsPath.second;

    // Root folder for the tileset
    string destRootArchiveFolder = "";
    string destRootFolder = "";
    if(tileSetNameSubfolder == "") {
        destRootFolder = this->tilesetsPath.second;
        if( EndsWith(destRootFolder,"/") == false &&
        	EndsWith(destRootFolder,"\\") == false) {
             destRootFolder += "/";
        }
        destRootArchiveFolder = destRootFolder;
        destRootFolder += tileSetName;
        if( EndsWith(destRootFolder,"/") == false &&
        	EndsWith(destRootFolder,"\\") == false) {
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

    if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> FTP Client thread about to try to RETR into [%s] findArchive = %d\n",destFile.c_str(),findArchive);
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"===> FTP Client thread about to try to RETR into [%s] findArchive = %d\n",destFile.c_str(),findArchive);

    struct FtpFile ftpfile = {
        tileSetName.c_str(),
        destFile.c_str(), // name to store the file as if succesful
        destFile.c_str(),
        NULL,
        this,
        "",
        false
    };

    if(findArchive == true) {
    	ftpfile.filepath = destRootArchiveFolder.c_str();
    }

    //curl_global_init(CURL_GLOBAL_DEFAULT);

    CURL *curl = SystemFlags::initHTTP();
    if(curl) {
        ftpfile.stream = NULL;

        char szBuf[1024]="";
        if(tileSetNameSubfolder == "") {
            if(findArchive == true) {
            	sprintf(szBuf,"ftp://%s:%s@%s:%d/%s%s",ftpUser.c_str(),ftpUserPassword.c_str(),serverUrl.c_str(),portNumber,tileSetName.c_str(),this->fileArchiveExtension.c_str());
            }
            else {
            	sprintf(szBuf,"ftp://%s:%s@%s:%d/%s/*",ftpUser.c_str(),ftpUserPassword.c_str(),serverUrl.c_str(),portNumber,tileSetName.c_str());
            }
        }
        else {
            sprintf(szBuf,"ftp://%s:%s@%s:%d/%s/%s/*",ftpUser.c_str(),ftpUserPassword.c_str(),serverUrl.c_str(),portNumber,tileSetName.c_str(),tileSetNameSubfolder.c_str());
        }

        curl_easy_setopt(curl, CURLOPT_URL,szBuf);
        curl_easy_setopt(curl, CURLOPT_FTP_USE_EPSV, 0L);

        // turn on wildcard matching
        curl_easy_setopt(curl, CURLOPT_WILDCARDMATCH, 1L);

        // callback is called before download of concrete file started
        curl_easy_setopt(curl, CURLOPT_CHUNK_BGN_FUNCTION, file_is_comming);
        // callback is called after data from the file have been transferred
        curl_easy_setopt(curl, CURLOPT_CHUNK_END_FUNCTION, file_is_downloaded);

        curl_easy_setopt(curl, CURLOPT_CHUNK_DATA, &ftpfile);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ftpfile);

        // Define our callback to get called when there's data to be written
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_fwrite);
        // Set a pointer to our struct to pass to the callback
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ftpfile);

        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, file_progress);
        curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &ftpfile);

        // Max 10 minutes to transfer
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 600);

        // Switch on full protocol/debug output
        if(SystemFlags::VERBOSE_MODE_ENABLED) curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        CURLcode res = curl_easy_perform(curl);

        if(res != CURLE_OK) {
          // we failed
          printf("curl FAILED with: %d [%s] attempting to remove folder contents [%s] szBuf [%s] ftpfile.isValidXfer = %d\n", res,curl_easy_strerror(res),destRootFolder.c_str(),szBuf,ftpfile.isValidXfer);
          SystemFlags::OutputDebug(SystemFlags::debugNetwork,"curl FAILED with: %d [%s] attempting to remove folder contents [%s] szBuf [%s] ftpfile.isValidXfer = %d\n", res,curl_easy_strerror(res),destRootFolder.c_str(),szBuf,ftpfile.isValidXfer);

          if(res == CURLE_PARTIAL_FILE || ftpfile.isValidXfer == true) {
        	  result = ftp_crt_PARTIALFAIL;
          }

          if(destRootFolder != "") {
              removeFolder(destRootFolder);
          }
        }
        else {
            result = ftp_crt_SUCCESS;

            bool requireMoreFolders = false;

            if(findArchive == false) {
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
            }

            if(requireMoreFolders == true) {
                result = getTilesetFromServer(tileSetName, tileSetNameSubfolder, ftpUser, ftpUserPassword, false);
                if(result != ftp_crt_SUCCESS) {
                  if(destRootFolder != "") {
                      removeFolder(destRootFolder);
                  }
                }
            }
        }

        SystemFlags::cleanupHTTP(&curl);
    }

    if(ftpfile.stream) {
        fclose(ftpfile.stream);
        ftpfile.stream = NULL;
    }

    // Extract the archive
    if(findArchive == true && result == ftp_crt_SUCCESS) {
        string extractCmd = getFullFileArchiveExtractCommand(this->fileArchiveExtractCommand,
        		this->fileArchiveExtractCommandParameters, destRootArchiveFolder,
        		destRootArchiveFolder + tileSetName + this->fileArchiveExtension);

        if(executeShellCommand(extractCmd) == false) {
        	result = ftp_crt_FAIL;
        }
    }

    return result;
}

void FTPClientThread::getTechtreeFromServer(string techtreeName) {
	FTP_Client_ResultType result = ftp_crt_FAIL;
	bool findArchive = executeShellCommand(this->fileArchiveExtractCommand);
	if(findArchive == true) {
		result = getTechtreeFromServer(techtreeName, FTP_TECHTREES_CUSTOM_USERNAME, FTP_COMMON_PASSWORD);
		if(result == ftp_crt_FAIL && this->getQuitStatus() == false) {
			result = getTechtreeFromServer(techtreeName, FTP_TECHTREES_USERNAME, FTP_COMMON_PASSWORD);
		}
	}

    MutexSafeWrapper safeMutex(this->getProgressMutex(),string(__FILE__) + "_" + intToStr(__LINE__));
    if(this->pCBObject != NULL) {
        this->pCBObject->FTPClient_CallbackEvent(techtreeName,ftp_cct_Techtree,result,NULL);
    }
}

FTP_Client_ResultType FTPClientThread::getTechtreeFromServer(string techtreeName,
		string ftpUser, string ftpUserPassword) {
    FTP_Client_ResultType result = ftp_crt_FAIL;

    string destFile = this->techtreesPath.second;

    // Root folder for the techtree
    string destRootArchiveFolder = "";
    string destRootFolder = "";
	destRootFolder = this->techtreesPath.second;
	if( EndsWith(destRootFolder,"/") == false &&
		EndsWith(destRootFolder,"\\") == false) {
		 destRootFolder += "/";
	}
	destRootArchiveFolder = destRootFolder;
	destRootFolder += techtreeName;
	if( EndsWith(destRootFolder,"/") == false &&
		EndsWith(destRootFolder,"\\") == false) {
		 destRootFolder += "/";
	}

	createDirectoryPaths(destRootFolder);

    if(EndsWith(destFile,"/") == false && EndsWith(destFile,"\\") == false) {
        destFile += "/";
    }
    destFile += techtreeName;
    if(EndsWith(destFile,"/") == false && EndsWith(destFile,"\\") == false) {
        destFile += "/";
    }

    if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> FTP Client thread about to try to RETR into [%s]\n",destFile.c_str());
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"===> FTP Client thread about to try to RETR into [%s]\n",destFile.c_str());

    struct FtpFile ftpfile = {
    	techtreeName.c_str(),
        destFile.c_str(), // name to store the file as if succesful
        destFile.c_str(),
        NULL,
        this,
        "",
        false
    };

   	ftpfile.filepath = destRootArchiveFolder.c_str();

    CURL *curl = SystemFlags::initHTTP();
    if(curl) {
        ftpfile.stream = NULL;

        char szBuf[1024]="";
        sprintf(szBuf,"ftp://%s:%s@%s:%d/%s%s",ftpUser.c_str(),ftpUserPassword.c_str(),serverUrl.c_str(),portNumber,techtreeName.c_str(),this->fileArchiveExtension.c_str());

        curl_easy_setopt(curl, CURLOPT_URL,szBuf);
        curl_easy_setopt(curl, CURLOPT_FTP_USE_EPSV, 0L);

        // turn on wildcard matching
        curl_easy_setopt(curl, CURLOPT_WILDCARDMATCH, 1L);

        // callback is called before download of concrete file started
        curl_easy_setopt(curl, CURLOPT_CHUNK_BGN_FUNCTION, file_is_comming);
        // callback is called after data from the file have been transferred
        curl_easy_setopt(curl, CURLOPT_CHUNK_END_FUNCTION, file_is_downloaded);

        curl_easy_setopt(curl, CURLOPT_CHUNK_DATA, &ftpfile);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ftpfile);

        // Define our callback to get called when there's data to be written
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_fwrite);
        // Set a pointer to our struct to pass to the callback
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ftpfile);

        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, file_progress);
        curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &ftpfile);

        // Max 10 minutes to transfer
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 600);

        // Switch on full protocol/debug output
        if(SystemFlags::VERBOSE_MODE_ENABLED) curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        CURLcode res = curl_easy_perform(curl);

        if(res != CURLE_OK) {
          // we failed
          printf("curl FAILED with: %d [%s] attempting to remove folder contents [%s] szBuf [%s] ftpfile.isValidXfer = %d\n", res,curl_easy_strerror(res),destRootFolder.c_str(),szBuf,ftpfile.isValidXfer);
          SystemFlags::OutputDebug(SystemFlags::debugNetwork,"curl FAILED with: %d [%s] attempting to remove folder contents [%s] szBuf [%s] ftpfile.isValidXfer = %d\n", res,curl_easy_strerror(res),destRootFolder.c_str(),szBuf,ftpfile.isValidXfer);

          if(res == CURLE_PARTIAL_FILE || ftpfile.isValidXfer == true) {
        	  result = ftp_crt_PARTIALFAIL;
          }

          if(destRootFolder != "") {
              removeFolder(destRootFolder);
          }
        }
        else {
            result = ftp_crt_SUCCESS;
        }

        SystemFlags::cleanupHTTP(&curl);
    }

    if(ftpfile.stream) {
        fclose(ftpfile.stream);
        ftpfile.stream = NULL;
    }

    // Extract the archive
    if(result == ftp_crt_SUCCESS) {
        string extractCmd = getFullFileArchiveExtractCommand(this->fileArchiveExtractCommand,
        		this->fileArchiveExtractCommandParameters, destRootArchiveFolder,
        		destRootArchiveFolder + techtreeName + this->fileArchiveExtension);

        if(executeShellCommand(extractCmd) == false) {
        	result = ftp_crt_FAIL;
        }
    }

    return result;
}


FTPClientCallbackInterface * FTPClientThread::getCallBackObject() {
    MutexSafeWrapper safeMutex(this->getProgressMutex(),string(__FILE__) + "_" + intToStr(__LINE__));
    return pCBObject;
}

void FTPClientThread::setCallBackObject(FTPClientCallbackInterface *value) {
    MutexSafeWrapper safeMutex(this->getProgressMutex(),string(__FILE__) + "_" + intToStr(__LINE__));
    pCBObject = value;
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
                MutexSafeWrapper safeMutex(&mutexMapFileList,string(__FILE__) + "_" + intToStr(__LINE__));
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

                MutexSafeWrapper safeMutex2(&mutexTilesetList,string(__FILE__) + "_" + intToStr(__LINE__));
                if(tilesetList.size() > 0) {
                    string tileset = tilesetList[0];
                    tilesetList.erase(tilesetList.begin() + 0);
                    safeMutex2.ReleaseLock();

                    getTilesetFromServer(tileset);
                }
                else {
                    safeMutex2.ReleaseLock();
                }

                MutexSafeWrapper safeMutex3(&mutexTechtreeList,string(__FILE__) + "_" + intToStr(__LINE__));
                if(techtreeList.size() > 0) {
                    string techtree = techtreeList[0];
                    techtreeList.erase(techtreeList.begin() + 0);
                    safeMutex3.ReleaseLock();

                    getTechtreeFromServer(techtree);
                }
                else {
                    safeMutex3.ReleaseLock();
                }

                if(this->getQuitStatus() == false) {
                    sleep(25);
                }
            }

            if(SystemFlags::VERBOSE_MODE_ENABLED) printf("===> FTP Client exiting!\n");
            SystemFlags::OutputDebug(SystemFlags::debugNetwork,"===> FTP Client exiting!\n");
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
