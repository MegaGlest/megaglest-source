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
#ifndef _SHARED_PLATFORMCOMMON_MINIFTPSERVERTHREAD_H_
#define _SHARED_PLATFORMCOMMON_MINIFTPSERVERTHREAD_H_

#include "base_thread.h"
#include <vector>
#include <string>
#include "types.h"

#include "leak_dumper.h"

using namespace std;

namespace Shared { namespace PlatformCommon {

// =====================================================
//	class FTPServerThread
// =====================================================
uint32 FindExternalFTPServerIp(uint32 clientIp);

class FTPServerThread : public BaseThread
{
protected:
    std::pair<string,string> mapsPath;
    int portNumber;
    //uint32 externalIp;

public:

    FTPServerThread(std::pair<string,string> mapsPath, int portNumber);
    ~FTPServerThread();
    virtual void execute();
    virtual void signalQuit();
    virtual bool shutdownAndWait();

    static void addClientToServerIPAddress(uint32 clientIp,uint32 ServerIp);

};

}}//end namespace

#endif
