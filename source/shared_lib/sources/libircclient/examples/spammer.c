/*
 * Copyright (C) 2004-2009 Georgy Yunaev gyunaev@ulduzsoft.com
 *
 * This example is free, and not covered by LGPL license. There is no 
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either 
 * completely or partially. By using it you may give me some credits in your
 * program, but you don't have to.
 *
 *
 * This example spams the specified channels with words 'HAHA', 'HEHE' and 
 * 'HUHU' using three threads. Its main purpose is to test multithreading 
 * support of libircclient.
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>

#if defined (WIN32)
	#include <windows.h>

	#define CREATE_THREAD(id,func,param)	(CreateThread(0, 0, func, param, 0, id) == 0)
	#define THREAD_FUNCTION(funcname)		static DWORD WINAPI funcname (LPVOID arg)
	#define thread_id_t		DWORD
	#define sleep(a)		Sleep (a*1000)
#else
	#include <unistd.h>
	#include <pthread.h>

	#define CREATE_THREAD(id,func,param)	(pthread_create (id, 0, func, (void *) param) != 0)
	#define THREAD_FUNCTION(funcname)		static void * funcname (void * arg)
	#define thread_id_t		pthread_t
#endif

#include "libircclient.h"


/*
 * We store data in IRC session context.
 */
typedef struct
{
	char 	* channel;
	char 	* nick;

} irc_ctx_t;


/*
 * Params that we give to our threads.
 */
typedef struct
{
	irc_session_t * session;
	const char *	phrase;
	const char *	channel;
	int				timer;

} spam_params_t;


THREAD_FUNCTION(gen_spam)
{
	spam_params_t * sp = (spam_params_t *) arg;

	while ( 1 )
	{
		if ( irc_cmd_msg (sp->session, sp->channel, sp->phrase) )
			break;

		sleep(sp->timer);
	}

	return 0;
}



void event_join (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
{
	irc_ctx_t * ctx = (irc_ctx_t *) irc_get_ctx (session);

	if ( !origin )
		return;

	// We need to know whether WE are joining the channel, or someone else.
	// To do this, we compare the origin with our nick.
    // Note that we have set LIBIRC_OPTION_STRIPNICKS to obtain 'parsed' nicks.
	if ( !strcmp(origin, ctx->nick) )
	{
		static spam_params_t spam1;
		static spam_params_t spam2;
		static spam_params_t spam3;
		thread_id_t tid;

		spam1.session = spam2.session = spam3.session = session;
		spam1.channel = spam2.channel = spam3.channel = ctx->channel;

		spam1.phrase = "HEHE";
		spam2.phrase = "HAHA";
		spam3.phrase = "HUHU";

		spam1.timer = 2;
		spam2.timer = 3;
		spam3.timer = 4;

		printf ("We just joined the channel %s; starting the spam threads\n", params[1]);

		if ( CREATE_THREAD (&tid, gen_spam, &spam1)
		|| CREATE_THREAD (&tid, gen_spam, &spam2)
		|| CREATE_THREAD (&tid, gen_spam, &spam3) )
			printf ("CREATE_THREAD failed: %s\n", strerror(errno));
		else
			printf ("Spammer thread was started successfully.\n");
	}
	else
	{
		char textbuf[168];
		sprintf (textbuf, "Hey, %s, hi!", origin);
		irc_cmd_msg (session, params[0], textbuf);
	}
}


void event_connect (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
{
	irc_ctx_t * ctx = (irc_ctx_t *) irc_get_ctx (session);
	irc_cmd_join (session, ctx->channel, 0);
}


void event_numeric (irc_session_t * session, unsigned int event, const char * origin, const char ** params, unsigned int count)
{
	if ( event > 400 )
	{
		printf ("ERROR %d: %s: %s %s %s %s\n", 
				event,
				origin ? origin : "unknown",
				params[0],
				count > 1 ? params[1] : "",
				count > 2 ? params[2] : "",
				count > 3 ? params[3] : "");
	}
}


int main (int argc, char **argv)
{
	irc_callbacks_t	callbacks;
	irc_ctx_t ctx;
	irc_session_t * s;

	if ( argc != 4 )
	{
		printf ("Usage: %s <server> <nick> <channel>\n", argv[0]);
		return 1;
	}

	// Initialize the callbacks
	memset (&callbacks, 0, sizeof(callbacks));

	// Set up the callbacks we will use
	callbacks.event_connect = event_connect;
	callbacks.event_join = event_join;
	callbacks.event_numeric = event_numeric;

	ctx.channel = argv[3];
    ctx.nick = argv[2];

	// And create the IRC session; 0 means error
	s = irc_create_session (&callbacks);

	if ( !s )
	{
		printf ("Could not create IRC session\n");
		return 1;
	}

	irc_set_ctx (s, &ctx);
	irc_option_set (s, LIBIRC_OPTION_STRIPNICKS);

	// Initiate the IRC server connection
	if ( irc_connect (s, argv[1], 6667, 0, argv[2], 0, 0) )
	{
		printf ("Could not connect: %s\n", irc_strerror (irc_errno(s)));
		return 1;
	}

	// and run into forever loop, generating events
	irc_run (s);

	return 1;
}
