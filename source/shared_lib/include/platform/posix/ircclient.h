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
#ifndef _SHARED_PLATFORMCOMMON_IRCTHREAD_H_
#define _SHARED_PLATFORMCOMMON_IRCTHREAD_H_

#include "base_thread.h"
#include <vector>
#include <string>

#ifndef WIN32
#include <libircclient/libircclient.h>
#else
#include "libircclient.h"
#endif

#include "leak_dumper.h"

using namespace std;

namespace Shared { namespace PlatformCommon {

// =====================================================
//	class IRCThreadThread
// =====================================================

class IRCCallbackInterface {
public:
    virtual void IRC_CallbackEvent(const char* origin, const char **params, unsigned int count) = 0;
};

class IRCThread : public BaseThread
{
protected:
    std::vector<string> argv;
    irc_session_t *ircSession;

public:
	IRCThread(const std::vector<string> &argv,IRCCallbackInterface *callbackObj);
    virtual void execute();
    virtual void signalQuit();
    virtual bool shutdownAndWait();

    void SendIRCCmdMessage(string target, string msg);
    std::vector<string> GetIRCConnectedNickList(string target);

    static IRCCallbackInterface *callbackObj;

    std::vector<string> getNickList() { return eventData; }

    static std::vector<string> eventData;
    static bool eventDataDone;
    static bool isConnected;
};

}}//end namespace

#endif
