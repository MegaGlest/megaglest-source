/* 
 * Copyright (C) 2009 Nathan Ollerenshaw chrome@stupendous.net
 *
 * This library is free software; you can redistribute it and/or modify it 
 * under the terms of the GNU Lesser General Public License as published by 
 * the Free Software Foundation; either version 2 of the License, or (at your 
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public 
 * License for more details.
 */

#define IRCCLIENTVERSION "1.0"

#import "IRCClientSession.h"
#import "NSObject+DDExtensions.h"
#import "IRCClientChannel.h"
#include "string.h"

static void onConnect(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
static void onNick(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
static void onQuit(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
static void onJoinChannel(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
static void onPartChannel(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
static void onMode(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
static void onUserMode(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
static void onTopic(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
static void onKick(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
static void onChannelPrvmsg(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
static void onPrivmsg(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
static void onNotice(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
static void onInvite(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
static void onCtcpRequest(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
static void onCtcpReply(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
static void onCtcpAction(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
static void onUnknownEvent(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
static void onNumericEvent(irc_session_t * session, unsigned int event, const char * origin, const char ** params, unsigned int count);

@implementation IRCClientSession

@synthesize delegate;
@synthesize session;
@synthesize version;
@synthesize server;
@synthesize port;
@synthesize password;
@synthesize nickname;
@synthesize username;
@synthesize realname;
@synthesize channels;
@synthesize encoding;

-(id)init
{
    if ((self = [super init])) {
		callbacks.event_connect = onConnect;
		callbacks.event_nick = onNick;
		callbacks.event_quit = onQuit;
		callbacks.event_join = onJoinChannel;
		callbacks.event_part = onPartChannel;
		callbacks.event_mode = onMode;
		callbacks.event_umode = onUserMode;
		callbacks.event_topic = onTopic;
		callbacks.event_kick = onKick;
		callbacks.event_channel = onChannelPrvmsg;
		callbacks.event_privmsg = onPrivmsg;
		callbacks.event_notice = onNotice;
		callbacks.event_invite = onInvite;
		callbacks.event_ctcp_req = onCtcpRequest;
		callbacks.event_ctcp_rep = onCtcpReply;
		callbacks.event_ctcp_action = onCtcpAction;
		callbacks.event_unknown = onUnknownEvent;
		callbacks.event_numeric = onNumericEvent;
		callbacks.event_dcc_chat_req = NULL;
		callbacks.event_dcc_send_req = NULL;
		
		session = irc_create_session(&callbacks);
		
		if (!session) {
			NSLog(@"Could not create irc_session.");
			return nil;
		}
		
		irc_set_ctx(session, self);
		
		unsigned int high, low;
		irc_get_version (&high, &low);
		
		[self setVersion:[NSString stringWithFormat:@"IRCClient Framework v%s (Nathan Ollerenshaw) - libirc v%d.%d (Georgy Yunaev)", IRCCLIENTVERSION, high, low]];
		
		channels = [[[NSMutableDictionary alloc] init] retain];		
		encoding = NSASCIIStringEncoding;
    }
    return self;
}

-(void)dealloc
{
	if (irc_is_connected(session))
		NSLog(@"Warning: IRC Session is not disconnected on dealloc");
		
	irc_destroy_session(session);
	
	[channels release];
	
	[super dealloc];
}

- (int)connect;
{
	unsigned short sPort = [port intValue];
		
	return irc_connect(session, [server cStringUsingEncoding:encoding], sPort, [password length] > 0 ? [password cStringUsingEncoding:encoding] : NULL , [nickname cStringUsingEncoding:encoding], [username cStringUsingEncoding:encoding], [realname cStringUsingEncoding:encoding]);
}

- (void)disconnect
{
	irc_disconnect(session);
}

- (bool)isConnected
{
	return irc_is_connected(session);
}

- (void)startThread
{
	NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
	
	irc_run(session);
	
	[pool drain];
}

- (void)run
{
	if (thread) {
		NSLog(@"Thread already running!");
		return;
	}
	
	thread = [[NSThread alloc] initWithTarget:self selector:@selector(startThread) object:nil];
	[thread retain];
	[thread start];
}

- (int)sendRawWithFormat:(NSString *)format, ...
{
	va_list		ap;
	
	va_start(ap, format);
	NSString *line = [[NSString alloc] initWithFormat:format arguments:ap];
	va_end(ap);
	
	return irc_send_raw(session, [line cStringUsingEncoding:encoding]);
}

- (int)quit:(NSString *)reason
{
	return irc_cmd_quit(session, [reason cStringUsingEncoding:encoding]);
}

- (int)join:(NSString *)channel key:(NSString *)key
{
	NSLog(@"Joining %@", channel);
	
	if (!key || ![key length] > 0)
		return irc_cmd_join(session, [channel cStringUsingEncoding:encoding], NULL);

	return irc_cmd_join(session, [channel cStringUsingEncoding:encoding], [key cStringUsingEncoding:encoding]);
}

- (int)list:(NSString *)channel
{
	return irc_cmd_list(session, [channel cStringUsingEncoding:encoding]);
}

- (int)userMode:(NSString *)mode
{
	return irc_cmd_user_mode(session, [mode cStringUsingEncoding:encoding]);
}

- (int)nick:(NSString *)newnick
{
	return irc_cmd_nick(session, [newnick cStringUsingEncoding:encoding]);
}

- (int)whois:(NSString *)nick
{
	return irc_cmd_whois(session, [nick cStringUsingEncoding:encoding]);
}

- (int)message:(NSString *)message to:(NSString *)target
{
	return irc_cmd_msg(session, [target cStringUsingEncoding:encoding], [message cStringUsingEncoding:encoding]);
}

- (int)action:(NSString *)action to:(NSString *)target
{
	return irc_cmd_me(session, [target cStringUsingEncoding:encoding], [action cStringUsingEncoding:encoding]);
}

- (int)notice:(NSString *)notice to:(NSString *)target
{
	return irc_cmd_notice(session, [target cStringUsingEncoding:encoding], [notice cStringUsingEncoding:encoding]);
}

- (int)ctcpRequest:(NSString *)request target:(NSString *)target
{
	return irc_cmd_ctcp_request(session, [target cStringUsingEncoding:encoding], [request cStringUsingEncoding:encoding]);
}

- (int)ctcpReply:(NSString *)reply target:(NSString *)target
{
	return irc_cmd_ctcp_reply(session, [target cStringUsingEncoding:encoding], [reply cStringUsingEncoding:encoding]);
}


	
@end

NSString *
getNickFromNickUserHost(NSString *nuh)
{
	NSArray *nuhArray = [nuh componentsSeparatedByCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"!@"]];
	
	if ([nuhArray count] == 3)
		return [NSString stringWithString:[nuhArray objectAtIndex:0]];
	else
		return [NSString stringWithString:nuh];
}

NSString *
getUserFromNickUserHost(NSString *nuh)
{
	NSArray *nuhArray = [nuh componentsSeparatedByCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"!@"]];
	
	if ([nuhArray count] == 3)
		return [NSString stringWithString:[nuhArray objectAtIndex:1]];
	else
		return nil;
}

NSString *
getHostFromNickUserHost(NSString *nuh)
{
	NSArray *nuhArray = [nuh componentsSeparatedByCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"!@"]];
	
	if ([nuhArray count] == 3)
		return [NSString stringWithString:[nuhArray objectAtIndex:2]];
	else
		return nil;
}

/*!
 * The "on_connect" event is triggered when the client successfully 
 * connects to the server, and could send commands to the server.
 * No extra params supplied; \a params is 0.
 */
static void onConnect(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
	IRCClientSession *client = (IRCClientSession *) irc_get_ctx(session);
	
	if ([[client delegate] respondsToSelector:@selector(onConnect)])
		[[[client delegate] dd_invokeOnMainThread] onConnect];
}

/*!
 * The "nick" event is triggered when the client receives a NICK message,
 * meaning that someone (including you) on a channel with the client has 
 * changed their nickname. 
 *
 * \param origin the person, who changes the nick. Note that it can be you!
 * \param params[0] mandatory, contains the new nick.
 */
static void onNick(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
	IRCClientSession *client = (IRCClientSession *) irc_get_ctx(session);
	
	NSString *nick = [NSString stringWithCString:params[0] encoding:NSASCIIStringEncoding];
	NSString *oldNick = [NSString stringWithCString:origin encoding:NSASCIIStringEncoding];

	if ([[client nickname] compare:oldNick] == NSOrderedSame) {
		[client setNickname:nick];
	}
	
	if ([[client delegate] respondsToSelector:@selector(onNick:oldNick:)])
		[[[client delegate] dd_invokeOnMainThread] onNick:nick oldNick:oldNick];
}

/*!
 * The "quit" event is triggered upon receipt of a QUIT message, which
 * means that someone on a channel with the client has disconnected.
 *
 * \param origin the person, who is disconnected
 * \param params[0] optional, contains the reason message (user-specified).
 */
static void onQuit(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
	IRCClientSession *client = (IRCClientSession *) irc_get_ctx(session);
	
	NSString *nick = [NSString stringWithCString:origin encoding:NSASCIIStringEncoding];
	NSString *reason = [NSString stringWithCString:params[0] encoding:[client encoding]];
	
	if ([[client delegate] respondsToSelector:@selector(onQuit:reason:)])
		[[[client delegate] dd_invokeOnMainThread] onQuit:nick reason:reason];
}

/*!
 * The "join" event is triggered upon receipt of a JOIN message, which
 * means that someone has entered a channel that the client is on.
 *
 * \param origin the person, who joins the channel. By comparing it with 
 *               your own nickname, you can check whether your JOIN 
 *               command succeed.
 * \param params[0] mandatory, contains the channel name.
 */
static void onJoinChannel(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
	IRCClientSession *client = (IRCClientSession *) irc_get_ctx(session);
	NSString *nick = [NSString stringWithCString:origin encoding:NSASCIIStringEncoding];
	NSString *channel = [NSString stringWithCString:params[0] encoding:NSASCIIStringEncoding];

	NSString *nickOnly = getNickFromNickUserHost(nick);
	
	if ([[client nickname] compare:nickOnly] == NSOrderedSame) {
		// We just joined a channel; allocate an IRCClientChannel object and send it
		// to the main thread.
		
		IRCClientChannel *newChannel = [[IRCClientChannel alloc] initWithName:channel];
		[[client channels] setObject:newChannel forKey:channel];
		
		if ([[client delegate] respondsToSelector:@selector(onJoinChannel:)])
			[[[client delegate] dd_invokeOnMainThread] onJoinChannel:newChannel];
	} else {
		// Someone joined a channel we're on.
		
		IRCClientChannel *currentChannel = [[client channels] objectForKey:channel];
		[currentChannel onJoin:nick];
	}
}

/*!
 * The "part" event is triggered upon receipt of a PART message, which
 * means that someone has left a channel that the client is on.
 *
 * \param origin the person, who leaves the channel. By comparing it with 
 *               your own nickname, you can check whether your PART 
 *               command succeed.
 * \param params[0] mandatory, contains the channel name.
 * \param params[1] optional, contains the reason message (user-defined).
 */
static void onPartChannel(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
	IRCClientSession *client = (IRCClientSession *) irc_get_ctx(session);
	
	NSString *nick = [NSString stringWithCString:origin encoding:NSASCIIStringEncoding];
	NSString *channel = [NSString stringWithCString:params[0] encoding:NSASCIIStringEncoding];
	NSData *reason = nil;
	IRCClientChannel *currentChannel = nil;
	
	if (count > 1)
		reason = [NSData dataWithBytes:params[1] length:strlen(params[1])];

	if ([[client nickname] compare:nick] == NSOrderedSame) {
		// We just left a channel; remove it from the channels dict.	
		
		currentChannel = [[client channels] objectForKey:channel];
		[[client channels] removeObjectForKey:channel];
	} else {
		// Someone left a channel we're on.
		
		currentChannel = [[client channels] objectForKey:channel];
	}
	
	[currentChannel onPart:nick reason:[[NSString alloc] initWithData:reason encoding:[currentChannel encoding]]];
}

/*!
 * The "mode" event is triggered upon receipt of a channel MODE message,
 * which means that someone on a channel with the client has changed the
 * channel's parameters.
 *
 * \param origin the person, who changed the channel mode.
 * \param params[0] mandatory, contains the channel name.
 * \param params[1] mandatory, contains the changed channel mode, like 
 *        '+t', '-i' and so on.
 * \param params[2] optional, contains the mode argument (for example, a
 *      key for +k mode, or user who got the channel operator status for 
 *      +o mode)
 */
static void onMode(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
	IRCClientSession *client = (IRCClientSession *) irc_get_ctx(session);
	NSString *nick = [NSString stringWithCString:origin encoding:NSASCIIStringEncoding];
	NSString *channel = [NSString stringWithCString:params[0] encoding:NSASCIIStringEncoding];
	NSString *mode = [NSString stringWithCString:params[1] encoding:NSASCIIStringEncoding];
	NSString *modeParams = nil;
	
	if (count > 2)
		modeParams = [NSString stringWithCString:params[2] encoding:NSASCIIStringEncoding];
	
	IRCClientChannel *currentChannel = [[client channels] objectForKey:channel];

	[currentChannel onMode:mode params:modeParams nick:nick];
}

/*!
 * The "umode" event is triggered upon receipt of a user MODE message, 
 * which means that your user mode has been changed.
 *
 * \param origin the person, who changed the channel mode.
 * \param params[0] mandatory, contains the user changed mode, like 
 *        '+t', '-i' and so on.
 */
static void onUserMode(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
	IRCClientSession *client = (IRCClientSession *) irc_get_ctx(session);
	NSString *mode = [NSString stringWithCString:params[0] encoding:NSASCIIStringEncoding];
	
	if ([[client delegate] respondsToSelector:@selector(onMode:)])
		[[[client delegate] dd_invokeOnMainThread] onMode:mode];
}

/*!
 * The "topic" event is triggered upon receipt of a TOPIC message, which
 * means that someone on a channel with the client has changed the 
 * channel's topic.
 *
 * \param origin the person, who changes the channel topic.
 * \param params[0] mandatory, contains the channel name.
 * \param params[1] optional, contains the new topic.
 */
static void onTopic(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
	IRCClientSession *client = (IRCClientSession *) irc_get_ctx(session);
	NSString *nick = [NSString stringWithCString:origin encoding:NSASCIIStringEncoding];
	NSString *channel = [NSString stringWithCString:params[0] encoding:NSASCIIStringEncoding];
	NSData *topic = nil;
	
	if (count > 1)
		topic = [NSData dataWithBytes:params[1] length:strlen(params[1])];
	
	IRCClientChannel *currentChannel = [[client channels] objectForKey:channel];

	[currentChannel onTopic:[[NSString alloc] initWithData:topic encoding:[currentChannel encoding]] nick:nick];
}

/*!
 * The "kick" event is triggered upon receipt of a KICK message, which
 * means that someone on a channel with the client (or possibly the
 * client itself!) has been forcibly ejected.
 *
 * \param origin the person, who kicked the poor.
 * \param params[0] mandatory, contains the channel name.
 * \param params[1] optional, contains the nick of kicked person.
 * \param params[2] optional, contains the kick text
 */
static void onKick(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
	IRCClientSession *client = (IRCClientSession *) irc_get_ctx(session);
	NSString *byNick = [NSString stringWithCString:origin encoding:NSASCIIStringEncoding];
	NSString *channel = [NSString stringWithCString:params[0] encoding:NSASCIIStringEncoding];
	NSString *nick = nil;
	NSData *reason = nil;
	
	if (count > 1)
		nick = [NSString stringWithCString:params[1] encoding:NSASCIIStringEncoding];
	
	if (count > 2)
		reason = [NSData dataWithBytes:params[2] length:strlen(params[2])];
	
	if (nick == nil) {
		// we got kicked
		IRCClientChannel *currentChannel = [[client channels] objectForKey:channel];
		[[client channels] removeObjectForKey:channel];
		
		[currentChannel onKick:[client nickname] reason:[[NSString alloc] initWithData:reason encoding:[currentChannel encoding]] byNick:byNick];
	} else {
		// someone else got booted
		IRCClientChannel *currentChannel = [[client channels] objectForKey:channel];
		
		[currentChannel onKick:nick reason:[[NSString alloc] initWithData:reason encoding:[currentChannel encoding]] byNick:byNick];
	}
}

/*!
 * The "channel" event is triggered upon receipt of a PRIVMSG message
 * to an entire channel, which means that someone on a channel with
 * the client has said something aloud. Your own messages don't trigger
 * PRIVMSG event.
 *
 * \param origin the person, who generates the message.
 * \param params[0] mandatory, contains the channel name.
 * \param params[1] optional, contains the message text
 */
static void onChannelPrvmsg(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
	IRCClientSession *client = (IRCClientSession *) irc_get_ctx(session);
	NSString *nick = [NSString stringWithCString:origin encoding:NSASCIIStringEncoding];
	NSString *channel = [NSString stringWithCString:params[0] encoding:NSASCIIStringEncoding];
	NSData *message = nil;
	
	if (count > 1) {
		message = [NSData dataWithBytes:params[1] length:strlen(params[1])];
	
		IRCClientChannel *currentChannel = [[client channels] objectForKey:channel];
		
		[currentChannel onPrivmsg:[[NSString alloc] initWithData:message encoding:[currentChannel encoding]] nick:nick];
	}
}

/*!
 * The "privmsg" event is triggered upon receipt of a PRIVMSG message
 * which is addressed to one or more clients, which means that someone
 * is sending the client a private message.
 *
 * \param origin the person, who generates the message.
 * \param params[0] mandatory, contains your nick.
 * \param params[1] optional, contains the message text
 */
static void onPrivmsg(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
	IRCClientSession *client = (IRCClientSession *) irc_get_ctx(session);
	NSString *nick = [NSString stringWithCString:origin encoding:NSASCIIStringEncoding];
	NSData *message = nil;

	if (count > 1) {
		message = [NSData dataWithBytes:params[1] length:strlen(params[1])];

		if ([[client delegate] respondsToSelector:@selector(onPrivmsg:nick:)])
			[[[client delegate] dd_invokeOnMainThread] onPrivmsg:message nick:nick];
	}
	
	// we eat privmsgs with no message
}

/*!
 * The "notice" event is triggered upon receipt of a NOTICE message
 * which means that someone has sent the client a public or private
 * notice. According to RFC 1459, the only difference between NOTICE 
 * and PRIVMSG is that you should NEVER automatically reply to NOTICE
 * messages. Unfortunately, this rule is frequently violated by IRC 
 * servers itself - for example, NICKSERV messages require reply, and 
 * are NOTICEs.
 *
 * \param origin the person, who generates the message.
 * \param params[0] mandatory, contains the channel name.
 * \param params[1] optional, contains the message text
 */
static void onNotice(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
	IRCClientSession *client = (IRCClientSession *) irc_get_ctx(session);
	NSString *nick = [NSString stringWithCString:origin encoding:NSASCIIStringEncoding];
	NSString *target = [NSString stringWithCString:params[0] encoding:NSASCIIStringEncoding];
	NSData *notice = nil;
	
	IRCClientChannel *currentChannel = [[client channels] objectForKey:target];
	
	if (count > 1) {
		notice = [NSData dataWithBytes:params[1] length:strlen(params[1])];
		
		if (currentChannel != nil) {
			[currentChannel onNotice:[[NSString alloc] initWithData:notice encoding:[currentChannel encoding]] nick:nick];
		} else {
			if ([[client delegate] respondsToSelector:@selector(onNotice:nick:)])
				[[[client delegate] dd_invokeOnMainThread] onNotice:notice nick:nick];
		}
	}
	
	// we eat notices with no message
}

/*!
 * The "invite" event is triggered upon receipt of an INVITE message,
 * which means that someone is permitting the client's entry into a +i
 * channel.
 *
 * \param origin the person, who INVITEs you.
 * \param params[0] mandatory, contains your nick.
 * \param params[1] mandatory, contains the channel name you're invited into.
 *
 * \sa irc_cmd_invite irc_cmd_chanmode_invite
 */
static void onInvite(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
	IRCClientSession *client = (IRCClientSession *) irc_get_ctx(session);
	NSString *nick = [NSString stringWithCString:origin encoding:NSASCIIStringEncoding];
	NSString *channel = [NSString stringWithCString:params[1] encoding:NSASCIIStringEncoding];
	
	if ([[client delegate] respondsToSelector:@selector(onInvite:nick:)])
		[[[client delegate] dd_invokeOnMainThread] onInvite:channel nick:nick];
}

/*!
 * The "ctcp" event is triggered when the client receives the CTCP 
 * request. By default, the built-in CTCP request handler is used. The 
 * build-in handler automatically replies on most CTCP messages, so you
 * will rarely need to override it.
 *
 * \param origin the person, who generates the message.
 * \param params[0] mandatory, the complete CTCP message, including its 
 *                  arguments.
 * 
 * Mirc generates PING, FINGER, VERSION, TIME and ACTION messages,
 * check the source code of \c libirc_event_ctcp_internal function to 
 * see how to write your own CTCP request handler. Also you may find 
 * useful this question in FAQ: \ref faq4
 */
static void onCtcpRequest(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
	IRCClientSession *client = (IRCClientSession *) irc_get_ctx(session);
	NSString *nick = [NSString stringWithCString:origin encoding:NSASCIIStringEncoding];

	if ( origin )
	{
		char nickbuf[128];
		irc_target_get_nick (origin, nickbuf, sizeof(nickbuf));
		
		if ( strstr (params[0], "PING") == params[0] ) {
			irc_cmd_ctcp_reply (session, nickbuf, params[0]);
		}
		else if ( !strcmp (params[0], "VERSION") )
		{
			irc_cmd_ctcp_reply (session, nickbuf, [[NSString stringWithFormat:@"VERSION %@", [client version]] UTF8String]);
		}
		else if ( !strcmp (params[0], "FINGER") )
		{
			irc_cmd_ctcp_reply (session, nickbuf, [[NSString stringWithFormat:@"FINGER %@ (%@) Idle 0 seconds", [client username], [client realname]] UTF8String]);
		}
		else if ( !strcmp (params[0], "TIME") )
		{
			irc_cmd_ctcp_reply(session, nickbuf, [[[NSDate dateWithTimeIntervalSinceNow:0] descriptionWithCalendarFormat:@"TIME %a %b %e %H:%M:%S %Z %Y" timeZone:nil locale:[[NSUserDefaults standardUserDefaults] dictionaryRepresentation]] UTF8String]);			
		} else {
			if ([[client delegate] respondsToSelector:@selector(onCtcpRequest:type:nick:)]) {
				NSString *requestString = [[NSString alloc] initWithData:[NSData dataWithBytes:params[0] length:strlen(params[0])] encoding:[client encoding]];
				
				NSRange firstSpace = [requestString rangeOfString:@" "];
				
				NSString *type = [requestString substringToIndex:firstSpace.location];
				NSString *request = [requestString substringFromIndex:(firstSpace.location + 1)];
				
				[[[client delegate] dd_invokeOnMainThread] onCtcpRequest:request type:type nick:nick];
			}
		}
	}	
}

/*!
 * The "ctcp" event is triggered when the client receives the CTCP reply.
 *
 * \param origin the person, who generates the message.
 * \param params[0] mandatory, the CTCP message itself with its arguments.
 */
static void onCtcpReply(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
	IRCClientSession *client = (IRCClientSession *) irc_get_ctx(session);
	
	NSString *nick = [NSString stringWithCString:origin encoding:NSASCIIStringEncoding];
	NSData *reply = [NSData dataWithBytes:params[0] length:strlen(params[0])];
	
	if ([[client delegate] respondsToSelector:@selector(onCtcpReply:nick:)])
		[[[client delegate] dd_invokeOnMainThread] onCtcpReply:reply nick:nick];
}

/*!
 * The "action" event is triggered when the client receives the CTCP 
 * ACTION message. These messages usually looks like:\n
 * \code
 * [23:32:55] * Tim gonna sleep.
 * \endcode
 *
 * \param origin the person, who generates the message.
 * \param params[0] mandatory, the target of the message.
 * \param params[1] mandatory, the ACTION message.
 */
static void onCtcpAction(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
	IRCClientSession *client = (IRCClientSession *) irc_get_ctx(session);
	
	NSString *nick = [NSString stringWithCString:origin encoding:NSASCIIStringEncoding];
	NSString *target = [NSString stringWithCString:params[0] encoding:NSASCIIStringEncoding];
	NSData *action = [NSData dataWithBytes:params[1] length:strlen(params[1])];
	
	IRCClientChannel *currentChannel = [[client channels] objectForKey:target];

	if (currentChannel) {
		// An action on a channel we're on
		
		[currentChannel onAction:[[NSString alloc] initWithData:action encoding:[currentChannel encoding]] nick:nick];
	} else {
		// An action in a private message
		
		if ([[client delegate] respondsToSelector:@selector(onAction:nick:)])
			[[[client delegate] dd_invokeOnMainThread] onAction:action nick:nick];
	}
}

/*!
 * The "unknown" event is triggered upon receipt of any number of 
 * unclassifiable miscellaneous messages, which aren't handled by the
 * library.
 */
static void onUnknownEvent(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
	IRCClientSession *client = (IRCClientSession *) irc_get_ctx(session);
	NSString *eventString = [NSString stringWithCString:event encoding:NSASCIIStringEncoding];
	NSString *sender = nil;
	
	if (origin != NULL)
		sender = [NSString stringWithCString:origin encoding:NSASCIIStringEncoding];
	
	NSMutableArray *paramsArray = [[NSMutableArray alloc] init];
	
	for (unsigned int i = 0; i < count; i++)
		[paramsArray addObject:[[NSString alloc] initWithData:[NSData dataWithBytes:params[i] length:strlen(params[i])] encoding:[client encoding]]];
	
	if ([[client delegate] respondsToSelector:@selector(onUnknownEvent:origin:params:)])
		[[[client delegate] dd_invokeOnMainThread] onUnknownEvent:eventString origin:sender params:paramsArray];
}

/*!
 * The "numeric" event is triggered upon receipt of any numeric response
 * from the server. There is a lot of such responses, see the full list
 * here: \ref rfcnumbers.
 *
 * See the params in ::irc_eventcode_callback_t specification.
 */
static void onNumericEvent(irc_session_t * session, unsigned int event, const char * origin, const char ** params, unsigned int count)
{
	IRCClientSession *client = (IRCClientSession *) irc_get_ctx(session);
	NSUInteger eventNumber = event;
	NSString *originString = [NSString stringWithCString:origin encoding:NSASCIIStringEncoding];
	
	NSMutableArray *paramsArray = [[NSMutableArray alloc] init];
	
	for (unsigned int i = 0; i < count; i++)
		[paramsArray addObject:[[NSString alloc] initWithData:[NSData dataWithBytes:params[i] length:strlen(params[i])] encoding:[client encoding]]];
	
	if ([[client delegate] respondsToSelector:@selector(onNumericEvent:origin:params:)])
		[[[client delegate] dd_invokeOnMainThread] onNumericEvent:eventNumber origin:originString params:paramsArray];
}
