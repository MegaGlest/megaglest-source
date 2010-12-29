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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

using namespace Shared::Util;
using namespace Shared::PlatformCommon;

namespace Shared { namespace PlatformCommon {

FTPServerThread::FTPServerThread(std::pair<string,string> mapsPath) : BaseThread() {
    this->mapsPath = mapsPath;
}

void FTPServerThread::signalQuit() {
    if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> FTP Server: signalQuit\n");
    BaseThread::signalQuit();
}

bool FTPServerThread::shutdownAndWait() {
    if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> FTP Server: shutdownAndWait\n");

    signalQuit();
    return BaseThread::shutdownAndWait();
}

void FTPServerThread::execute() {
    {
        RunningStatusSafeWrapper runningStatus(this);
        SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

        if(getQuitStatus() == true) {
            return;
        }

        if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> FTP Server thread is running\n");
        SystemFlags::OutputDebug(SystemFlags::debugNetwork,"FTP Server thread is running\n");

        try	{
            //ftpCreateAccount("anonymous", "", mapsPath.first.c_str(), FTP_ACC_RD | FTP_ACC_LS | FTP_ACC_DIR);

            if(mapsPath.first != "") {
                SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] mapsPath #1 [%s]\n",__FILE__,__FUNCTION__,__LINE__,mapsPath.first.c_str());
                ftpCreateAccount("maps", "mg_ftp_server", mapsPath.first.c_str(), FTP_ACC_RD | FTP_ACC_LS | FTP_ACC_DIR);
            }
            if(mapsPath.second != "") {
                SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] mapsPath #2 [%s]\n",__FILE__,__FUNCTION__,__LINE__,mapsPath.second.c_str());
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
            ftpStart();
            while(this->getQuitStatus() == false) {
                ftpExecute();
                sleep(25);
            }
            ftpShutdown();

            if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> FTP Server exiting!\n");
        }
        catch(const exception &ex) {
            SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
            SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
        }
        catch(...) {
            SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] UNKNOWN Error\n",__FILE__,__FUNCTION__,__LINE__);
            SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] unknown error\n",__FILE__,__FUNCTION__,__LINE__);
        }

        SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] FTP Server thread is exiting\n",__FILE__,__FUNCTION__,__LINE__);
    }

    // Delete ourself when the thread is done (no other actions can happen after this
    // such as the mutex which modifies the running status of this method
	//delete this;
}

}}//end namespace
