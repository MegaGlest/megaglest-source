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
 * This example 'guards' the channel against using abusive language. When 
 * someone says a bad word, it takes some action against him/her, taking 
 * in account the number of times the person uses it. The first time, it just 
 * warns the person through a private message, the second time it warns him 
 * publically in channel, and after the second time it will kicks the insolent
 * out of the channel.
 *
 * To keep it simple, this example reacts only on 'fuck' word, however 
 * is is easy to add more.
 *
 * Features used:
 * - nickname parsing;
 * - handling 'channel' event to track the messages;
 * - handling 'nick' event to track nickname changes;
 * - generating channel and private messages, and kicking.
 */

#include <map>
#include <string>

#include <stdio.h>
#include <errno.h>
#include <string.h>

#if !defined (WIN32)
	#include <unistd.h>
#endif

#include "libircclient.h"


/*
 * We store data in IRC session context.
 */
typedef struct
{
	char 	* channel;
	char 	* nick;
	std::map <std::string, unsigned int> insolents;

} irc_ctx_t;




void event_connect (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
{
	irc_ctx_t * ctx = (irc_ctx_t *) irc_get_ctx (session);
	irc_cmd_join (session, ctx->channel, 0);
}


void event_nick (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
{
	char nickbuf[128];

	irc_ctx_t * ctx = (irc_ctx_t *) irc_get_ctx (session);

	if ( !origin || count != 1 )
		return;

	irc_target_get_nick (origin, nickbuf, sizeof(nickbuf));

	if ( ctx->insolents.find(nickbuf) != ctx->insolents.end() )
	{
		printf ("%s has changed its nick to %s to prevent penalties - no way!\n",
			nickbuf, params[0]);
		ctx->insolents[params[0]] = ctx->insolents[nickbuf];
		ctx->insolents.erase (nickbuf);
	}
}


void event_channel (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
{
	irc_ctx_t * ctx = (irc_ctx_t *) irc_get_ctx (session);

	if ( !origin || count != 2 )
		return;

	if ( strstr (params[1], "fuck") == 0 )
		return;

	char nickbuf[128], text[256];

	irc_target_get_nick (origin, nickbuf, sizeof(nickbuf));

	if ( ctx->insolents.find(nickbuf) == ctx->insolents.end() )
		ctx->insolents[nickbuf] = 0;

	ctx->insolents[nickbuf]++;

	printf ("'%s' swears in the channel '%s' %d times\n",
			nickbuf,
			params[1],
			ctx->insolents[nickbuf]);

	switch (ctx->insolents[nickbuf])
	{
	case 1:
		// Send a private message
		sprintf (text, "%s, please do not swear in this channel.", nickbuf);
		irc_cmd_msg (session, nickbuf, text);
		break;

	case 2:
		// Send a channel message
		sprintf (text, "%s, do not swear in this channel, or you'll leave it.", nickbuf);
		irc_cmd_msg (session, params[0], text);
		break;

	default:
		// Send a channel notice, and kick the insolent
		sprintf (text, "kicked %s from %s for swearing.", nickbuf, params[0]);
		irc_cmd_me (session, params[0], text);
		irc_cmd_kick (session, nickbuf, params[0], "swearing");
		break;
	}
}


void event_numeric (irc_session_t * session, unsigned int event, const char * origin, const char ** params, unsigned int count)
{
	if ( event > 400 )
	{
		std::string fulltext;
		for ( unsigned int i = 0; i < count; i++ )
		{
			if ( i > 0 )
				fulltext += " ";

			fulltext += params[i];
		}

		printf ("ERROR %d: %s: %s\n", event, origin ? origin : "?", fulltext.c_str());
	}
}


int main (int argc, char **argv)
{
	irc_callbacks_t	callbacks;
	irc_ctx_t ctx;

	if ( argc != 4 )
	{
		printf ("Usage: %s <server> <nick> <channel>\n", argv[0]);
		return 1;
	}

	// Initialize the callbacks
	memset (&callbacks, 0, sizeof(callbacks));

	// Set up the callbacks we will use
	callbacks.event_connect = event_connect;
	callbacks.event_channel = event_channel;
	callbacks.event_nick = event_nick;
	callbacks.event_numeric = event_numeric;

	// And create the IRC session; 0 means error
	irc_session_t * s = irc_create_session (&callbacks);

	if ( !s )
	{
		printf ("Could not create IRC session\n");
		return 1;
	}

	ctx.channel = argv[3];
    ctx.nick = argv[2];
	irc_set_ctx (s, &ctx);

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
