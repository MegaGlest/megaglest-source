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

#import <Cocoa/Cocoa.h>
#include <IRCClient/libircclient.h>

/** 
 *	@file IRCClientChannelDelegate.h
 *	@author Nathan Ollerenshaw
 *	@version 1.0
 *	@date 01.2009
 *	@brief Receives delegate messages from an IRCClientSession.
 *	@protocol IRCClientSessionDelegate
 */

@class IRCClientChannel;

/** @brief Receives delegate messages from an IRCClientSession.
 *
 *	Each IRCClientSession object needs a single delegate. Methods are called
 *  for each event that occurs on an IRC server that the client is connected to.
 *
 *  Note that for any given parameter, it may be optional, in which case a nil
 *  object may be supplied instead of the given parameter.
 */

@interface NSObject (IRCClientSessionDelegate)

/** The client has successfully connected to the IRC server. */

- (void)onConnect;

/** An IRC client on a channel that this client is connected to has changed nickname,
 *	or this IRC client has changed nicknames.
 *
 *  @param nick the new nickname
 *	@param oldNick the old nickname
 */

- (void)onNick:(NSString *)nick oldNick:(NSString *)oldNick;

/** An IRC client on a channel that this client is connected to has quit IRC.
 *
 *  @param nick the nickname of the client that quit.
 *  @param reason (optional) the quit message, if any.
 */

- (void)onQuit:(NSString *)nick reason:(NSString *)reason;

/** The IRC client has joined (connected) successfully to a new channel. This
 *  event creates an IRCClientChannel object, which you are expected to asign a
 *  delegate to, to handle events from the channel.
 *
 *	For example, on receipt of this message, a graphical IRC client would most
 *  likely open a new window, create an IRCClientChannelDelegate for the window,
 *  set the new IRCClientChannel's delegate to the new delegate, and then hook
 *  it up so that new events sent to the IRCClientChannelDelegate are sent to 
 *  the window.
 *
 *  @param channel the IRCClientChannel object for the newly joined channel.
 */

- (void)onJoinChannel:(IRCClientChannel *)channel;

/** The client has changed it's user mode.
 *
 *  @param mode the new mode.
 */

- (void)onMode:(NSString *)mode;

/** The client has received a private PRIVMSG from another IRC client.
 *
 *  @param message the text of the message
 *  @param nick the other IRC Client that sent the message.
 */

- (void)onPrivmsg:(NSData *)message nick:(NSString *)nick;

/** The client has received a private NOTICE from another client.
 *
 *  @param notice the text of the message
 *  @param nick the nickname of the other IRC client that sent the message.
 */

- (void)onNotice:(NSData *)notice nick:(NSString *)nick;

/** The IRC client has been invited to a channel.
 *
 *  @param channel the channel for the invitation.
 *  @param nick the nickname of the user that sent the invitation.
 */

- (void)onInvite:(NSString *)channel nick:(NSString *)nick;

/** A private CTCP request was sent to the IRC client.
 *
 *  @param request the CTCP request string (after the type)
 *  @param type the CTCP request type
 *  @param nick the nickname of the user that sent the request.
 */

- (void)onCtcpRequest:(NSString *)request type:(NSString *)type nick:(NSString *)nick;  

/** A private CTCP reply was sent to the IRC client.
 *
 *  @param reply an NSData containing the raw C string of the reply.
 *  @param nick the nickname of the user that sent the reply.
 */

- (void)onCtcpReply:(NSData *)reply nick:(NSString *)nick;

/** A private CTCP ACTION was sent to the IRC client.
 *
 *  CTCP ACTION is not limited to channels; it may also be sent directly to other users.
 *
 *  @param action the action message text.
 *  @param nick the nickname of the client that sent the action.
 */

- (void)onAction:(NSData *)action nick:(NSString *)nick;

/** An unhandled event was received from the IRC server.
 *
 *  @param event the unknown event name
 *  @param origin the sender of the event
 *  @param params an NSArray of NSData objects that are the raw C strings of the event.
 */

- (void)onUnknownEvent:(NSString *)event origin:(NSString *)origin params:(NSArray *)params;

/** An unhandled numeric was received from the IRC server
 *
 *  @param event the unknown event number
 *  @param origin the sender of the event
 *  @param params an NSArray of NSData objects that are the raw C strings of the event.
 */

- (void)onNumericEvent:(NSUInteger)event origin:(NSString *)origin params:(NSArray *)params;

@end
