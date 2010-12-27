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

/** 
 *	@file IRCClientChannel.h
 *	@author Nathan Ollerenshaw
 *	@version 1.0
 *	@date 01.2009
 *	@brief Represents a connected IRC Channel.
 */

#import <Cocoa/Cocoa.h>
#import <IRCClient/IRCClientChannelDelegate.h>

/** \class IRCClientChannel
 *	@brief Represents a connected IRC Channel.
 *
 *	IRCClientChannel objects are created by the IRCClientSession object
 *  for a given session when the client joins an IRC channel. At that time
 *  you are expected to register event handlers for each of the delegate
 *  methods described in the IRCClientChannelDelegate interface.
 */

@class IRCClientSession;
@interface IRCClientChannel : NSObject {
	id					delegate;
	NSString			*name;
	NSStringEncoding	encoding;
	IRCClientSession	*session;
	NSString			*topic;
	NSString			*modes;
	NSMutableArray		*names;
}

/** Delegate to send events to */
@property (assign)			id					delegate;

/** Name of the channel */
@property (copy)			NSString			*name;

/** Encoding used by this channel */
@property (assign)			NSStringEncoding	encoding;

/** Associated IRCClientSession object */
@property (assign)			IRCClientSession	*session;

/** Topic of the channel */
@property (copy)			NSString			*topic;

/** Mode of the channel */
@property (copy)			NSString			*modes;

/** An array of nicknames stored as NSStrings that list the connected users
    for the channel */
@property (assign, readonly) NSMutableArray		*names;

/** initWithName:
 *
 *	Returns an initialised IRCClientChannel with a given channel name. You
 *	are not expected to initialise your own IRCClientChannel objects; if you
 *	wish to join a channel you should send a [IRCClientSession join:key:] message
 *	to your IRCClientSession object.
 *
 *  @param aName Name of the channel.
 */

- (id)initWithName:(NSString *)aName;

/** Parts the channel.
 */

- (int)part;

/** Invites another IRC client to the channel.
 *
 *  @param nick the nickname of the client to invite.
 */

- (int)invite:(NSString *)nick;

/** Sets the topic of the channel.
 *
 *	Note that not all users on a channel have permission to change the topic; if you fail
 *	to set the topic, then you will not see an onTopic event on the IRCClientChannelDelegate.
 *
 *  @param aTopic the topic the client wishes to set for the channel.
 */

- (void)setTopic:(NSString *)aTopic;

/** Sets the mode of the channel.
 *
 *	Note that not all users on a channel have permission to change the mode; if you fail
 *	to set the mode, then you will not see an onMode event on the IRCClientChannelDelegate.
 *
 *  @param mode the mode to set the channel to
 *  @param params paramaters for the mode, if it requires parameters.
 */

- (int)setMode:(NSString *)mode params:(NSString *)params;

/** Sends a public PRIVMSG to the channel. If you try to send more than can fit on an IRC
 *	buffer, it will be truncated.
 *
 *  @param message the message to send to the channel.
 */

- (int)message:(NSString *)message;

/** Sends a public CTCP ACTION to the channel.
 *
 *  @param action action to send to the channel.
 */

- (int)action:(NSString *)action;

/** Sends a public NOTICE to the channel.
 *
 *  @param notice message to send to the channel.
 */

- (int)notice:(NSString *)notice;

/** Kicks someone from a channel.
 *
 *  @param nick the IRC client to kick from the channel.
 *  @param reason the message to give to the channel and the IRC client for the kick.
 */

- (int)kick:(NSString *)nick reason:(NSString *)reason;

/** Sends a CTCP request to the channel.
 *
 *	It is perfectly legal to send a CTCP request to an IRC channel, however many clients
 *	decline to respond to them, and often they are percieved as annoying.
 *
 *  @param request the string of the request, in CTCP format.
 */

- (int)ctcpRequest:(NSString *)request;

@end
