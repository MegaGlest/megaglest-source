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

#include "miniftpserver.h"
#include "util.h"
#include "platform_common.h"
#include "ftpIfc.h"
#include "socket.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

using namespace Shared::Util;
using namespace Shared::PlatformCommon;

namespace Shared { namespace PlatformCommon {

static std::map<uint32,uint32> clientToFTPServerList;
FTPClientValidationInterface * FTPServerThread::ftpValidationIntf = NULL;

ip_t FindExternalFTPServerIp(ip_t clientIp) {
    ip_t result = clientToFTPServerList[clientIp];

    if(SystemFlags::VERBOSE_MODE_ENABLED) printf("===> FTP Server thread clientIp = %u, result = %u\n",clientIp,result);
    if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"===> FTP Server thread clientIp = %u, result = %u\n",clientIp,result);

    return result;
}

int isValidClientType(ip_t clientIp) {
    int result = 0;
    if(FTPServerThread::getFtpValidationIntf() != NULL) {
        result = FTPServerThread::getFtpValidationIntf()->isValidClientType(clientIp);
    }
    return result;
}

FTPServerThread::FTPServerThread(std::pair<string,string> mapsPath,
		std::pair<string,string> tilesetsPath, std::pair<string,string> techtreesPath,
		bool internetEnabledFlag,
		bool allowInternetTilesetFileTransfers, bool allowInternetTechtreeFileTransfers,
		int portNumber, int maxPlayers,
		FTPClientValidationInterface *ftpValidationIntf) : BaseThread() {
    this->mapsPath              = mapsPath;
    this->tilesetsPath          = tilesetsPath;
    this->techtreesPath			= techtreesPath;
    this->allowInternetTilesetFileTransfers = allowInternetTilesetFileTransfers;
    this->allowInternetTechtreeFileTransfers = allowInternetTechtreeFileTransfers;
    setInternetEnabled(internetEnabledFlag,true);
    this->portNumber            = portNumber;
    this->maxPlayers            = maxPlayers;
    this->ftpValidationIntf     = ftpValidationIntf;

	ftpInit(&FindExternalFTPServerIp,&UPNP_Tools::AddUPNPPortForward,&UPNP_Tools::RemoveUPNPPortForward, &isValidClientType);
    VERBOSE_MODE_ENABLED        = SystemFlags::VERBOSE_MODE_ENABLED;

    if(SystemFlags::VERBOSE_MODE_ENABLED) printf("***FTP SERVER STARTED [%p]\n",this);
}

FTPServerThread::~FTPServerThread() {
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	ftpShutdown();

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	// Remove any UPNP port forwarded ports
	//printf("In [%s::%s] Line: %d ServerSocket::getFTPServerPort() = %d\n",__FILE__,__FUNCTION__,__LINE__,ServerSocket::getFTPServerPort());
    UPNP_Tools::upnp_rem_redirect(ServerSocket::getFTPServerPort());
    for(int clientIndex = 1; clientIndex <= maxPlayers; ++clientIndex) {
    	//printf("In [%s::%s] Line: %d ServerSocket::getFTPServerPort()+ clientIndex = %d\n",__FILE__,__FUNCTION__,__LINE__,ServerSocket::getFTPServerPort()+ clientIndex);
        UPNP_Tools::upnp_rem_redirect(ServerSocket::getFTPServerPort() + clientIndex);
    }
    if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    if(SystemFlags::VERBOSE_MODE_ENABLED) printf("***FTP SERVER ENDED [%p]\n",this);
}

void FTPServerThread::signalQuit() {
    if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> FTP Server: signalQuit\n");
    if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"===> FTP Server: signalQuit\n");
    ftpShutdown();

    BaseThread::signalQuit();
}

bool FTPServerThread::shutdownAndWait() {
    if(SystemFlags::VERBOSE_MODE_ENABLED) printf("===> FTP Server: shutdownAndWait\n");
    if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"===> FTP Server: shutdownAndWait\n");

    signalQuit();
    bool ret = BaseThread::shutdownAndWait();
    clientToFTPServerList.clear();
    return ret;
}

void FTPServerThread::addClientToServerIPAddress(uint32 clientIp,uint32 ServerIp) {
     clientToFTPServerList[clientIp] = ServerIp;
     if(SystemFlags::VERBOSE_MODE_ENABLED) printf("===> FTP Server thread clientIp = %u, ServerIp = %u\n",clientIp,ServerIp);
     if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"===> FTP Server thread clientIp = %u, ServerIp = %u\n",clientIp,ServerIp);
}

void FTPServerThread::setInternetEnabled(bool value, bool forceChange) {
	if(forceChange == true || this->internetEnabled != value) {
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> FTP Server thread, changing InternetEnabled = %d\n",value);

		this->internetEnabled = value;
		if(this->internetEnabled == true) {
			// Setup FTP Users and permissions for tilesets
			if(this->allowInternetTilesetFileTransfers == true) {
				if(tilesetsPath.first != "") {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] tilesetsPath #1 [%s]\n",__FILE__,__FUNCTION__,__LINE__,tilesetsPath.first.c_str());
					ftpCreateAccount("tilesets", "mg_ftp_server", tilesetsPath.first.c_str(), FTP_ACC_RD | FTP_ACC_LS | FTP_ACC_DIR);
				}
				if(tilesetsPath.second != "") {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] tilesetsPath #2 [%s]\n",__FILE__,__FUNCTION__,__LINE__,tilesetsPath.second.c_str());
					ftpCreateAccount("tilesets_custom", "mg_ftp_server", tilesetsPath.second.c_str(), FTP_ACC_RD | FTP_ACC_LS | FTP_ACC_DIR);
				}

				if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> FTP Server thread, tilesets users created\n");
			}
			else {
				ftpDeleteAccount("tilesets");
				ftpDeleteAccount("tilesets_custom");

				if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> FTP Server thread, tilesets users deleted\n");
			}

			if(this->allowInternetTechtreeFileTransfers == true) {
				// Setup FTP Users and permissions for tilesets
				if(techtreesPath.first != "") {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] techtreesPath #1 [%s]\n",__FILE__,__FUNCTION__,__LINE__,techtreesPath.first.c_str());
					ftpCreateAccount("techtrees", "mg_ftp_server", techtreesPath.first.c_str(), FTP_ACC_RD | FTP_ACC_LS | FTP_ACC_DIR);
				}
				if(techtreesPath.second != "") {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] techtreesPath #2 [%s]\n",__FILE__,__FUNCTION__,__LINE__,techtreesPath.second.c_str());
					ftpCreateAccount("techtrees_custom", "mg_ftp_server", techtreesPath.second.c_str(), FTP_ACC_RD | FTP_ACC_LS | FTP_ACC_DIR);
				}

				if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> FTP Server thread, techtrees users created\n");
			}
			else {
				ftpDeleteAccount("techtrees");
				ftpDeleteAccount("techtrees_custom");

				if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> FTP Server thread, techtrees users deleted\n");
			}
		}
		else {
			if(tilesetsPath.first != "") {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] tilesetsPath #1 [%s]\n",__FILE__,__FUNCTION__,__LINE__,tilesetsPath.first.c_str());
				ftpCreateAccount("tilesets", "mg_ftp_server", tilesetsPath.first.c_str(), FTP_ACC_RD | FTP_ACC_LS | FTP_ACC_DIR);
			}
			if(tilesetsPath.second != "") {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] tilesetsPath #2 [%s]\n",__FILE__,__FUNCTION__,__LINE__,tilesetsPath.second.c_str());
				ftpCreateAccount("tilesets_custom", "mg_ftp_server", tilesetsPath.second.c_str(), FTP_ACC_RD | FTP_ACC_LS | FTP_ACC_DIR);
			}

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> FTP Server thread, tilesets users created\n");

			// Setup FTP Users and permissions for tilesets
			if(techtreesPath.first != "") {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] techtreesPath #1 [%s]\n",__FILE__,__FUNCTION__,__LINE__,techtreesPath.first.c_str());
				ftpCreateAccount("techtrees", "mg_ftp_server", techtreesPath.first.c_str(), FTP_ACC_RD | FTP_ACC_LS | FTP_ACC_DIR);
			}
			if(techtreesPath.second != "") {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] techtreesPath #2 [%s]\n",__FILE__,__FUNCTION__,__LINE__,techtreesPath.second.c_str());
				ftpCreateAccount("techtrees_custom", "mg_ftp_server", techtreesPath.second.c_str(), FTP_ACC_RD | FTP_ACC_LS | FTP_ACC_DIR);
			}

			if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> FTP Server thread, techtrees users created\n");
		}
	}
}

void FTPServerThread::execute() {
    {
        RunningStatusSafeWrapper runningStatus(this);
        if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

        if(getQuitStatus() == true) {
            return;
        }

        if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> FTP Server thread is running\n");
        if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"FTP Server thread is running\n");

        try	{
            //ftpCreateAccount("anonymous", "", mapsPath.first.c_str(), FTP_ACC_RD | FTP_ACC_LS | FTP_ACC_DIR);

            // Setup FTP Users and permissions for maps
            if(mapsPath.first != "") {
            	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] mapsPath #1 [%s]\n",__FILE__,__FUNCTION__,__LINE__,mapsPath.first.c_str());
                ftpCreateAccount("maps", "mg_ftp_server", mapsPath.first.c_str(), FTP_ACC_RD | FTP_ACC_LS | FTP_ACC_DIR);
            }
            if(mapsPath.second != "") {
            	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] mapsPath #2 [%s]\n",__FILE__,__FUNCTION__,__LINE__,mapsPath.second.c_str());
                ftpCreateAccount("maps_custom", "mg_ftp_server", mapsPath.second.c_str(), FTP_ACC_RD | FTP_ACC_LS | FTP_ACC_DIR);
            }

/*
            ftpCreateAccount("anonymous", "", "./", FTP_ACC_RD | FTP_ACC_LS | FTP_ACC_DIR);
            ftpCreateAccount("nothing", "", "./", 0);
            ftpCreateAccount("reader", "", "./", FTP_ACC_RD);
            ftpCreateAccount("writer", "", "./", FTP_ACC_WR);
            ftpCreateAccount("lister", "", "./", FTP_ACC_LS);
            ftpCreateAccount("admin", "xxx", "./", FTP_ACC_RD | FTP_ACC_WR | FTP_ACC_LS | FTP_ACC_DIR);
*/
            if(getQuitStatus() == true) {
                return;
            }

            ftpStart(portNumber);
            while(this->getQuitStatus() == false) {
                ftpExecute();
                //if(processedWork == 0) {
                    //sleep(25);
                //}
            }
            ftpShutdown();

            if(SystemFlags::VERBOSE_MODE_ENABLED) printf("===> FTP Server exiting!\n");
            if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"===> FTP Server exiting!\n");
        }
        catch(const exception &ex) {
            SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
            if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
        }
        catch(...) {
            SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] UNKNOWN Error\n",__FILE__,__FUNCTION__,__LINE__);
            if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] unknown error\n",__FILE__,__FUNCTION__,__LINE__);
        }

        if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] FTP Server thread is exiting\n",__FILE__,__FUNCTION__,__LINE__);
        if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] FTP Server thread is exiting\n",__FILE__,__FUNCTION__,__LINE__);
    }
}

}}//end namespace
