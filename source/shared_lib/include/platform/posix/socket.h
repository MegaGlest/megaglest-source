// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2005 Matthias Braun <matze@braunis.de>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_PLATFORM_SOCKET_H_
#define _SHARED_PLATFORM_SOCKET_H_

#include <string>

#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <map>
#include <vector>
#include "thread.h"

using std::string;

namespace Shared{ namespace Platform{

//
// This interface describes the methods a callback object must implement
// when signalled with detected servers
//
class DiscoveredServersInterface {
public:
	virtual void DiscoveredServers(std::vector<string> serverList) = 0;
};

// =====================================================
//	class IP
// =====================================================

class Ip{
private:
	unsigned char bytes[4];

public:
	Ip();
	Ip(unsigned char byte0, unsigned char byte1, unsigned char byte2, unsigned char byte3);
	Ip(const string& ipString);

	unsigned char getByte(int byteIndex)	{return bytes[byteIndex];}
	string getString() const;
};

// =====================================================
//	class Socket
// =====================================================

class Socket {
protected:
	int sock;
	long lastDebugEvent;
	static int broadcast_portno;

public:
	Socket(int sock);
	Socket();
	virtual ~Socket();

	static int getBroadCastPort() { return broadcast_portno; }
	static void setBroadCastPort(int value) { broadcast_portno = value; }
	static std::vector<std::string> getLocalIPAddressList();

    // Int lookup is socket fd while bool result is whether or not that socket was signalled for reading
    static bool hasDataToRead(std::map<int,bool> &socketTriggeredList);
    static bool hasDataToRead(int socket);
    bool hasDataToRead();
    void disconnectSocket();

    int getSocketId() const { return sock; }

	int getDataToRead();
	int send(const void *data, int dataSize);
	int receive(void *data, int dataSize);
	int peek(void *data, int dataSize);

	void setBlock(bool block);
	static void setBlock(bool block, int socket);

	bool isReadable();
	bool isWritable(bool waitOnDelayedResponse);
	bool isConnected();

	static string getHostName();
	static string getIp();

protected:
	static void throwException(const string &str);
};

class BroadCastClientSocketThread : public Thread
{
private:
	Mutex mutexRunning;
	Mutex mutexQuit;

	bool quit;
	bool running;

	DiscoveredServersInterface *discoveredServersCB;

	void setRunningStatus(bool value);
	void setQuitStatus(bool value);

public:
	BroadCastClientSocketThread(DiscoveredServersInterface *cb);
    virtual void execute();
    void signalQuit();
    bool getQuitStatus();
    bool getRunningStatus();
};

// =====================================================
//	class ClientSocket
// =====================================================

class ClientSocket: public Socket{
public:
	ClientSocket();
	virtual ~ClientSocket();

	void connect(const Ip &ip, int port);
	static void discoverServers(DiscoveredServersInterface *cb);

	static void stopBroadCastClientThread();

protected:

	static BroadCastClientSocketThread *broadCastClientThread;
	static void startBroadCastClientThread(DiscoveredServersInterface *cb);
};

class BroadCastSocketThread : public Thread
{
private:
	Mutex mutexRunning;
	Mutex mutexQuit;

	bool quit;
	bool running;

	void setRunningStatus(bool value);
	void setQuitStatus(bool value);

public:
	BroadCastSocketThread();
    virtual void execute();
    void signalQuit();
    bool getQuitStatus();
    bool getRunningStatus();
};

// =====================================================
//	class ServerSocket
// =====================================================

class ServerSocket: public Socket{
public:
	ServerSocket();
	virtual ~ServerSocket();
	void bind(int port);
	void listen(int connectionQueueSize= SOMAXCONN);
	Socket *accept();
	void stopBroadCastThread();

protected:

	BroadCastSocketThread *broadCastThread;
	void startBroadCastThread();

};

}}//end namespace

#endif
