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

#include "ircclient.h"
#include "util.h"
#include "platform_common.h"
#include "libircclient.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "conversion.h"

using namespace Shared::Util;
using namespace Shared::PlatformCommon;

namespace Shared { namespace PlatformCommon {

const int IRC_SERVER_PORT = 6667;

void addlog (const char * fmt, ...) {
	FILE * fp;
	char buf[1024];
	va_list va_alist;

	va_start (va_alist, fmt);
#if defined (WIN32)
	_vsnprintf (buf, sizeof(buf), fmt, va_alist);
#else
	vsnprintf (buf, sizeof(buf), fmt, va_alist);
#endif
	va_end (va_alist);

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> IRC: %s\n", buf);

    if(SystemFlags::VERBOSE_MODE_ENABLED == true) {
        if ( (fp = fopen ("irctest.log", "ab")) != 0 ) {
            fprintf (fp, "%s\n", buf);
            fclose (fp);
        }
    }
}

void dump_event (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count) {
	char buf[512]="";
	unsigned int cnt=0;
	buf[0] = '\0';

	for ( cnt = 0; cnt < count; cnt++ )	{
		if ( cnt ) {
			strcat (buf, "|");
        }
		strcat (buf, params[cnt]);
	}

	addlog ("Event \"%s\", origin: \"%s\", params: %d [%s]", event, origin ? origin : "NULL", cnt, buf);

    IRCThread *ctx = (IRCThread *)irc_get_ctx(session);
	if(ctx != NULL) {
        if(difftime(time(NULL),ctx->getLastNickListUpdate()) >= 7) {
            ctx->setLastNickListUpdate(time(NULL));
            ctx->GetIRCConnectedNickList(ctx->getChannel(),false);
        }
	}
}

void get_nickname(const char *sourceNick,char *destNick,size_t maxDestBufferSize) {
	string sourceNickStr = sourceNick;
	if(sourceNickStr != "" && sourceNickStr[0] == '@') {
		sourceNickStr.erase(0,1);
	}

	irc_target_get_nick(sourceNickStr.c_str(),destNick,maxDestBufferSize);
}

void event_join(irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count) {
	dump_event (session, event, origin, params, count);

    IRCThread *ctx = (IRCThread *)irc_get_ctx(session);
	if(ctx != NULL) {
        if(ctx->getHasJoinedChannel() == false) {
            irc_cmd_user_mode (session, "+i");
            //irc_cmd_msg (session, params[0], "MG Bot says hello!");
            ctx->setHasJoinedChannel(true);

            ctx->GetIRCConnectedNickList(ctx->getChannel(),true);
        }
        else {
            char realNick[128]="";
            get_nickname(origin,realNick,127);
            if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> IRC: user joined channel realNick [%s] origin [%s]\n", realNick,origin);

            bool foundNick = false;

            MutexSafeWrapper safeMutex(ctx->getMutexNickList(),string(__FILE__) + "_" + intToStr(__LINE__));
            std::vector<string> nickList = ctx->getCachedNickList();
            for(unsigned int i = 0;
                             i < nickList.size(); ++i) {
                if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> IRC: looking for match [%s] realNick [%s]\n", nickList[i].c_str(),realNick);

                if(nickList[i] == realNick) {
                    foundNick = true;
                    break;
                }
            }
            if(foundNick == false) {
                nickList.push_back(realNick);
            }
        }
    }
}

void event_connect (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count) {
    IRCThread *ctx = (IRCThread *)irc_get_ctx(session);

	dump_event(session, event, origin, params, count);

    if(ctx != NULL) {
        irc_cmd_join(session, ctx->getChannel().c_str(), 0);
    }
}

void event_privmsg (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count) {
	dump_event (session, event, origin, params, count);

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("'%s' said me (%s): %s\n",origin ? origin : "someone",params[0], params[1] );
}

void dcc_recv_callback (irc_session_t * session, irc_dcc_t id, int status, void * ctx, const char * data, unsigned int length) {
	static int count = 1;
	char buf[12]="";

	switch (status)
	{
	case LIBIRC_ERR_CLOSED:
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("DCC %d: chat closed\n", id);
		break;

	case 0:
		if ( !data ) {
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("DCC %d: chat connected\n", id);
			irc_dcc_msg	(session, id, "Hehe");
		}
		else {
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("DCC %d: %s\n", id, data);
			sprintf (buf, "DCC [%d]: %d", id, count++);
			irc_dcc_msg	(session, id, buf);
		}
		break;

	default:
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("DCC %d: error %s\n", id, irc_strerror(status));
		break;
	}
}

void dcc_file_recv_callback (irc_session_t * session, irc_dcc_t id, int status, void * ctx, const char * data, unsigned int length) {
	if ( status == 0 && length == 0 ) {
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("File sent successfully\n");

		if ( ctx ) {
			fclose ((FILE*) ctx);
		}
	}
	else if ( status ) {
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("File sent error: %d\n", status);

		if ( ctx ) {
			fclose ((FILE*) ctx);
		}
	}
	else {
		if ( ctx ) {
			fwrite (data, 1, length, (FILE*) ctx);
		}
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("File sent progress: %d\n", length);
	}
}

void event_channel(irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count) {
	if ( count != 2 )
		return;

	if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> IRC: '%s' said in channel %s: %s\n",origin ? origin : "someone",params[0], params[1] );

	if ( !origin ) {
		return;
	}

    char realNick[128]="";
    get_nickname(origin,realNick,127);
    if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> IRC: event signalled realNick [%s] origin [%s]\n", realNick,origin);

    IRCThread *ctx = (IRCThread *)irc_get_ctx(session);
	if(ctx != NULL) {
        MutexSafeWrapper safeMutex(ctx->getMutexIRCCB(),string(__FILE__) + "_" + intToStr(__LINE__));
        IRCCallbackInterface *cb = ctx->getCallbackObj(false);
        if(cb != NULL) {
            cb->IRC_CallbackEvent(IRC_evt_chatText, realNick, params, count);
        }
	}

	if ( !strcmp (params[1], "quit") )
		irc_cmd_quit (session, "of course, Master!");

	if ( !strcmp (params[1], "help") ) {
		irc_cmd_msg (session, params[0], "quit, help, dcc chat, dcc send, ctcp");
	}

	if ( !strcmp (params[1], "ctcp") ) {
		irc_cmd_ctcp_request (session, realNick, "PING 223");
		irc_cmd_ctcp_request (session, realNick, "FINGER");
		irc_cmd_ctcp_request (session, realNick, "VERSION");
		irc_cmd_ctcp_request (session, realNick, "TIME");
	}

	if ( !strcmp (params[1], "dcc chat") ) {
		irc_dcc_t dccid;
		irc_dcc_chat (session, 0, realNick, dcc_recv_callback, &dccid);
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("DCC chat ID: %d\n", dccid);
	}

	if ( !strcmp (params[1], "dcc send") ) {
		irc_dcc_t dccid;
		irc_dcc_sendfile (session, 0, realNick, "irctest.c", dcc_file_recv_callback, &dccid);
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("DCC send ID: %d\n", dccid);
	}

	if ( !strcmp (params[1], "topic") ) {
		irc_cmd_topic (session, params[0], 0);
    }
	else if ( strstr (params[1], "topic ") == params[1] ) {
		irc_cmd_topic (session, params[0], params[1] + 6);
    }

	if ( strstr (params[1], "mode ") == params[1] )
		irc_cmd_channel_mode (session, params[0], params[1] + 5);

	if ( strstr (params[1], "nick ") == params[1] )
		irc_cmd_nick (session, params[1] + 5);

	if ( strstr (params[1], "whois ") == params[1] )
		irc_cmd_whois (session, params[1] + 5);
}

void irc_event_dcc_chat(irc_session_t * session, const char * nick, const char * addr, irc_dcc_t dccid) {
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("DCC chat [%d] requested from '%s' (%s)\n", dccid, nick, addr);

	irc_dcc_accept (session, dccid, 0, dcc_recv_callback);
}

void irc_event_dcc_send(irc_session_t * session, const char * nick, const char * addr, const char * filename, unsigned long size, irc_dcc_t dccid) {
	FILE * fp;
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("DCC send [%d] requested from '%s' (%s): %s (%lu bytes)\n", dccid, nick, addr, filename, size);

	if ( (fp = fopen ("file", "wb")) == 0 ) {
		abort();
	}
	irc_dcc_accept (session, dccid, fp, dcc_file_recv_callback);
}

void event_leave(irc_session_t *session, const char *event, const char *origin, const char ** params, unsigned count) {
	char buf[24]="";
	sprintf (buf, "%s", event);

	// someone left the channel.
	if(origin) {
	    if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> IRC: user left channel [%s]\n", origin);

        char realNick[128]="";
        get_nickname(origin,realNick,127);
        if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> IRC: user left channel realNick [%s] origin [%s]\n", realNick,origin);

        IRCThread *ctx = (IRCThread *)irc_get_ctx(session);
        if(ctx != NULL) {
            MutexSafeWrapper safeMutex(ctx->getMutexNickList(),string(__FILE__) + "_" + intToStr(__LINE__));
            std::vector<string> &nickList = ctx->getCachedNickList();
            for(unsigned int i = 0;
                             i < nickList.size(); ++i) {
                if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> IRC: lookingfor match [%s] realNick [%s]\n", nickList[i].c_str(),realNick);

                if(nickList[i] == realNick) {
                    nickList.erase(nickList.begin() + i);
                    break;
                }
            }
        }
	}

	dump_event (session, buf, origin, params, count);
}
void event_numeric(irc_session_t * session, unsigned int event, const char * origin, const char ** params, unsigned int count) {
	char buf[24]="";
	sprintf (buf, "%d", event);

    switch (event) {
		case LIBIRC_RFC_ERR_NICKNAMEINUSE :
		case LIBIRC_RFC_ERR_NICKCOLLISION :
			//irc_auto_rename_nick(session);
			//IRCThread *ctx = (IRCThread *)irc_get_ctx(session);
			//if(ctx != NULL) {

			break;
		case LIBIRC_RFC_RPL_TOPIC :
			break;
		case LIBIRC_RFC_RPL_NAMREPLY :
            {
                if(event == LIBIRC_RFC_RPL_NAMREPLY) {
                    std::vector<string> nickList;
                    if(count >= 4) {
                        for(unsigned int i = 3; i < count && params[i]; ++i) {
                            vector<string> tokens;
                            Tokenize(params[i],tokens," ");

                            for(unsigned int j = 0; j < tokens.size(); ++j) {

                                char realNick[128]="";
                                get_nickname(tokens[j].c_str(),realNick,127);
                                if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> IRC: user joined channel realNick [%s] tokens[j] [%s]\n", realNick,tokens[j].c_str());

                                // Only show Megaglest users in the user list
                                if(strncmp(&realNick[0],"MG_",3) == 0) {
                                	nickList.push_back(realNick);
                                }
                            }
                        }
                    }

                    IRCThread *ctx = (IRCThread *)irc_get_ctx(session);
                    if(ctx != NULL) {
                        MutexSafeWrapper safeMutex(ctx->getMutexNickList(),string(__FILE__) + "_" + intToStr(__LINE__));
                        ctx->setCachedNickList(nickList);
                    }
                }
                break;
            }
        case LIBIRC_RFC_RPL_ENDOFNAMES:
            {
                IRCThread *ctx = (IRCThread *)irc_get_ctx(session);
                if(ctx != NULL) {
                    ctx->setEventDataDone(true);
                }
            }
            break;
    }

	dump_event (session, buf, origin, params, count);
}

IRCThread::IRCThread(const std::vector<string> &argv, IRCCallbackInterface *callbackObj) : BaseThread() {
    this->argv = argv;
    this->callbackObj = callbackObj;
    ircSession = NULL;
    eventData.clear();
    eventDataDone = false;
    hasJoinedChannel = false;
    lastNickListUpdate = time(NULL);
}

void IRCThread::signalQuit() {
    if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> IRC: signalQuit [%p]\n",ircSession);

    if(ircSession != NULL) {
        setCallbackObj(NULL);
        if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> IRC: Quitting Channel\n");
        irc_cmd_quit(ircSession, "MG Bot is closing!");
        BaseThread::signalQuit();
        hasJoinedChannel = false;
    }
}

bool IRCThread::shutdownAndWait() {
    if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> IRC: shutdownAndWait [%p]\n",ircSession);

    signalQuit();
    return BaseThread::shutdownAndWait();
}

void IRCThread::SendIRCCmdMessage(string target, string msg) {
    if(ircSession != NULL && hasJoinedChannel == true) {
        SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] sending IRC command to [%s] cmd [%s]\n",__FILE__,__FUNCTION__,__LINE__,target.c_str(),msg.c_str());
        int ret = irc_cmd_msg (ircSession, target.c_str(), msg.c_str());
        SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] sending IRC command to [%s] cmd [%s] ret = %d\n",__FILE__,__FUNCTION__,__LINE__,target.c_str(),msg.c_str(),ret);
    }
}

std::vector<string> IRCThread::GetIRCConnectedNickList(string target, bool waitForCompletion) {
    eventDataDone = false;
    if(ircSession != NULL && hasJoinedChannel == true) {
        SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] sending IRC nick list command to [%s]\n",__FILE__,__FUNCTION__,__LINE__,target.c_str());
        int ret = irc_cmd_names (ircSession, target.c_str());
        SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] sending IRC nick list command to [%s] ret = %d\n",__FILE__,__FUNCTION__,__LINE__,target.c_str(),ret);

        if(waitForCompletion == true) {
            for(time_t tElapsed = time(NULL);
                eventDataDone == false &&
                this->getQuitStatus() == false &&
                difftime(time(NULL),tElapsed) <= 5;) {
                sleep(50);
            }
        }
    }

    MutexSafeWrapper safeMutex(&mutexNickList,string(__FILE__) + "_" + intToStr(__LINE__));
    std::vector<string> nickList = eventData;
    safeMutex.ReleaseLock();

    return nickList;
}

bool IRCThread::isConnected() {
    bool ret = false;
    if(ircSession != NULL) {
        ret = (irc_is_connected(ircSession) != 0);
    }

    return ret;
}

std::vector<string> IRCThread::getNickList() {
    MutexSafeWrapper safeMutex(&mutexNickList,string(__FILE__) + "_" + intToStr(__LINE__));
    std::vector<string> nickList = eventData;
    safeMutex.ReleaseLock();

    return nickList;
}

IRCCallbackInterface * IRCThread::getCallbackObj(bool lockObj) {
    MutexSafeWrapper safeMutex(NULL,string(__FILE__) + "_" + intToStr(__LINE__));
    if(lockObj == true) {
        safeMutex.setMutex(&mutexIRCCB);
    }
    return callbackObj;
}
void IRCThread::setCallbackObj(IRCCallbackInterface *cb) {
    MutexSafeWrapper safeMutex(&mutexIRCCB,string(__FILE__) + "_" + intToStr(__LINE__));
    callbackObj=cb;
}

void IRCThread::execute() {
    {
        RunningStatusSafeWrapper runningStatus(this);
        SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] argv.size() = %d\n",__FILE__,__FUNCTION__,__LINE__,argv.size());

        if(getQuitStatus() == true) {
            return;
        }

        SystemFlags::OutputDebug(SystemFlags::debugNetwork,"IRC thread is running\n");

        try	{
            irc_callbacks_t	callbacks;
            ircSession=NULL;

            if(argv.size() != 3) {
                if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> IRC Usage: <server> <nick> <channel> : got params [%ld]\n",(long int)argv.size());
                return;
            }

            memset (&callbacks, 0, sizeof(callbacks));

            callbacks.event_connect = event_connect;
            callbacks.event_join = event_join;
            callbacks.event_nick = dump_event;
            callbacks.event_quit = dump_event;
            callbacks.event_part = event_leave;
            callbacks.event_mode = dump_event;
            callbacks.event_topic = dump_event;
            callbacks.event_kick = dump_event;
            callbacks.event_channel = event_channel;
            callbacks.event_privmsg = event_privmsg;
            callbacks.event_notice = dump_event;
            callbacks.event_invite = dump_event;
            callbacks.event_umode = dump_event;
            callbacks.event_ctcp_rep = dump_event;
            callbacks.event_ctcp_action = dump_event;
            callbacks.event_unknown = dump_event;
            callbacks.event_numeric = event_numeric;

            callbacks.event_dcc_chat_req = irc_event_dcc_chat;
            callbacks.event_dcc_send_req = irc_event_dcc_send;

            if(this->getQuitStatus() == true) {
                return;
            }
            ircSession = irc_create_session (&callbacks);

            if(!ircSession) {
                if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> IRC Could not create session\n");
                return;
            }

            this->channel = argv[2];
            this->nick    = argv[1];
            irc_set_ctx(ircSession, this);

            if(this->getQuitStatus() == true) {
                return;
            }

            if(irc_connect(ircSession, argv[0].c_str(), IRC_SERVER_PORT, 0, this->nick.c_str(), this->nick.c_str(), "megaglest")) {
                if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> IRC Could not connect: %s\n", irc_strerror (irc_errno(ircSession)));
                return;
            }

            if(this->getQuitStatus() == true) {
                return;
            }

            if(this->getQuitStatus() == true) {
                return;
            }

            for(int iAttempts=1;
                this->getQuitStatus() == false && iAttempts <= 5;
                ++iAttempts) {
                if(irc_run(ircSession)) {
                    if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> IRC Could not run the session: %s\n", irc_strerror (irc_errno(ircSession)));
                }
            }

            if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("===> IRC exiting IRC CLient!\n");
        }
        catch(const exception &ex) {
            SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
            SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
        }
        catch(...) {
            SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] UNKNOWN Error\n",__FILE__,__FUNCTION__,__LINE__);
            SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] unknown error\n",__FILE__,__FUNCTION__,__LINE__);
        }

        SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] IRC thread is exiting\n",__FILE__,__FUNCTION__,__LINE__);
    }

    // Delete ourself when the thread is done (no other actions can happen after this
    // such as the mutex which modifies the running status of this method
    MutexSafeWrapper safeMutex(&mutexIRCCB,string(__FILE__) + "_" + intToStr(__LINE__));
    IRCCallbackInterface *cb = getCallbackObj(false);
    if(cb != NULL) {
       cb->IRC_CallbackEvent(IRC_evt_exitThread, NULL, NULL, 0);
    }
    safeMutex.ReleaseLock();

	delete this;
}

}}//end namespace
