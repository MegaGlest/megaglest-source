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

#import "IRCClientChannel.h"
#import "IRCClientSession.h"
#import "NSObject+DDExtensions.h"

@implementation IRCClientChannel

@synthesize delegate;
@synthesize name;
@synthesize encoding;
@synthesize session;
@synthesize topic;
@synthesize modes;
@synthesize names;

-(id)init
{
	return [self initWithName:nil];
}

-(id)initWithName:(NSString *)aName
{
    if ((self = [super init])) {
		[self setName:aName];
		topic = [[NSString alloc] init];
		encoding = NSASCIIStringEncoding;
	}
	
	return self;
}

-(void)dealloc
{
	[super dealloc];
}

- (int)part
{
	return irc_cmd_part([session session], [name cStringUsingEncoding:NSASCIIStringEncoding]);
}

- (int)invite:(NSString *)nick
{
	return irc_cmd_invite([session session], [nick cStringUsingEncoding:NSASCIIStringEncoding], [name cStringUsingEncoding:NSASCIIStringEncoding]);
}

- (int)refreshNames
{
	return irc_cmd_names([session session], [name cStringUsingEncoding:NSASCIIStringEncoding]);
}

- (void)setTopic:(NSString *)aTopic
{
	irc_cmd_topic([session session], [name cStringUsingEncoding:NSASCIIStringEncoding], [topic cStringUsingEncoding:encoding]);
}

- (int)setMode:(NSString *)mode params:(NSString *)params
{
	return irc_cmd_channel_mode([session session], [name cStringUsingEncoding:NSASCIIStringEncoding], [mode cStringUsingEncoding:NSASCIIStringEncoding]);
}

- (int)message:(NSString *)message
{
	return irc_cmd_msg([session session], [name cStringUsingEncoding:NSASCIIStringEncoding], [message cStringUsingEncoding:encoding]);
}

- (int)action:(NSString *)action
{
	return irc_cmd_me([session session], [name cStringUsingEncoding:NSASCIIStringEncoding], [action cStringUsingEncoding:encoding]);
}

- (int)notice:(NSString *)notice
{
	return irc_cmd_notice([session session], [name cStringUsingEncoding:NSASCIIStringEncoding], [notice cStringUsingEncoding:encoding]);
}

- (int)kick:(NSString *)nick reason:(NSString *)reason
{
	return irc_cmd_kick([session session], [nick cStringUsingEncoding:NSASCIIStringEncoding], [name cStringUsingEncoding:NSASCIIStringEncoding], [reason cStringUsingEncoding:encoding]);
}

- (int)ctcpRequest:(NSString *)request
{
	return irc_cmd_ctcp_request([session session], [name cStringUsingEncoding:NSASCIIStringEncoding], [request cStringUsingEncoding:encoding]);
}


// event handlers
//
// These farm events out to the delegate on the main thread.

- (void)onJoin:(NSString *)nick
{
	if ([delegate respondsToSelector:@selector(onJoin:)])
		[[delegate dd_invokeOnMainThread] onJoin:nick];
}

- (void)onPart:(NSString *)nick reason:(NSString *)reason
{
	if ([delegate respondsToSelector:@selector(onPart:reason:)])
		[[delegate dd_invokeOnMainThread] onPart:nick reason:reason];
}

- (void)onMode:(NSString *)mode params:(NSString *)params nick:(NSString *)nick
{
	if ([delegate respondsToSelector:@selector(onMode:params:nick:)])
		[[delegate dd_invokeOnMainThread] onMode:mode params:params nick:nick];
}

- (void)onTopic:(NSString *)aTopic nick:(NSString *)nick
{
	[topic release];
	topic = [NSString stringWithString:aTopic];
	
	if ([delegate respondsToSelector:@selector(onTopic:nick:)])
		[[delegate dd_invokeOnMainThread] onTopic:aTopic nick:nick];
}

- (void)onKick:(NSString *)nick reason:(NSString *)reason byNick:(NSString *)byNick
{
	if ([delegate respondsToSelector:@selector(onKick:reason:byNick:)])
		[[delegate dd_invokeOnMainThread] onKick:nick reason:reason byNick:byNick];
}

- (void)onPrivmsg:(NSString *)message nick:(NSString *)nick
{
	if ([delegate respondsToSelector:@selector(onPrivmsg:nick:)])
		[[delegate dd_invokeOnMainThread] onPrivmsg:message nick:nick];
}

- (void)onNotice:(NSString *)notice nick:(NSString *)nick
{
	if ([delegate respondsToSelector:@selector(onNotice:nick:)])
		[[delegate dd_invokeOnMainThread] onNotice:notice nick:nick];
}

- (void)onAction:(NSString *)action nick:(NSString *)nick
{
	if ([delegate respondsToSelector:@selector(onAction:nick:)])
		[[delegate dd_invokeOnMainThread] onAction:action nick:nick];
}


@end
