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

#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <map>
#include <vector>
#include "base_thread.h"
#include "simple_threads.h"

using std::string;

#ifdef WIN32
	#include <winsock.h>
	typedef SOCKET PLATFORM_SOCKET;
#else
	#include <unistd.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>

	typedef int PLATFORM_SOCKET;

#endif

using namespace Shared::PlatformCommon;

namespace Shared{ namespace Platform{

//
// This interface describes the methods a callback object must implement
// when signaled with detected servers
//
class DiscoveredServersInterface {
public:
	virtual void DiscoveredServers(std::vector<string> serverList) = 0;
};

// =====================================================
//	class IP
// =====================================================
class Ip {
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

class Socket : public SimpleTaskCallbackInterface {

#ifdef WIN32

private:
	class SocketManager{
	public:
		SocketManager();
		~SocketManager();
	};

protected:
	static SocketManager socketManager;

#endif

protected:
	PLATFORM_SOCKET sock;
	time_t lastDebugEvent;
	static int broadcast_portno;
	std::string ipAddress;

	SimpleTaskThread *pingThread;
	std::map<string,float> pingCache;
	time_t lastThreadedPing;
	Mutex pingThreadAccessor;

public:
	Socket(PLATFORM_SOCKET sock);
	Socket();
	virtual ~Socket();

	virtual void simpleTask();

	static int getBroadCastPort() 			{ return broadcast_portno; }
	static void setBroadCastPort(int value) { broadcast_portno = value; }
	static std::vector<std::string> getLocalIPAddressList();

    // Int lookup is socket fd while bool result is whether or not that socket was signalled for reading
    static bool hasDataToRead(std::map<PLATFORM_SOCKET,bool> &socketTriggeredList);
    static bool hasDataToRead(PLATFORM_SOCKET socket);
    bool hasDataToRead();
    void disconnectSocket();

    PLATFORM_SOCKET getSocketId() const { return sock; }

	int getDataToRead();
	int send(const void *data, int dataSize);
	int receive(void *data, int dataSize);
	int peek(void *data, int dataSize);

	void setBlock(bool block);
	static void setBlock(bool block, PLATFORM_SOCKET socket);

	bool isReadable();
	bool isWritable(bool waitOnDelayedResponse);
	bool isConnected();

	static string getHostName();
	static string getIp();
	bool isSocketValid() const;
	static bool isSocketValid(const PLATFORM_SOCKET *validateSocket);

	static float getAveragePingMS(std::string host, int pingCount=5);
	float getThreadedPingMS(std::string host);

	virtual std::string getIpAddress();
	virtual void setIpAddress(std::string value) { ipAddress = value; }

protected:
	static void throwException(string str);
};

class BroadCastClientSocketThread : public BaseThread
{
private:
	DiscoveredServersInterface *discoveredServersCB;

public:
	BroadCastClientSocketThread(DiscoveredServersInterface *cb);
    virtual void execute();
};

// =====================================================
//	class ClientSocket
// =====================================================
class ClientSocket: public Socket {
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

class BroadCastSocketThread : public BaseThread
{
private:

public:
	BroadCastSocketThread();
    virtual void execute();
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

	int boundPort;
	BroadCastSocketThread *broadCastThread;
	void startBroadCastThread();
	bool isBroadCastThreadRunning();

};

}}//end namespace

#endif
