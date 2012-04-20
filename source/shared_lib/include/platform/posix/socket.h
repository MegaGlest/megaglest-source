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

#ifdef WIN32
    #ifdef __MINGW32__
	   #include <winsock2.h>
    #else
       #include <winsock2.h>
       #include <winsock.h>
    #endif

	typedef SOCKET PLATFORM_SOCKET;
#else
	#include <unistd.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>

	typedef int PLATFORM_SOCKET;

#endif

#include <string>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <map>
#include <vector>
#include "base_thread.h"
#include "simple_threads.h"
#include "data_types.h"

using std::string;

#include "leak_dumper.h"

using namespace Shared::PlatformCommon;

namespace Shared { namespace Platform {

// The callback Interface used by the UPNP discovery process
class FTPClientValidationInterface {
public:
	virtual int isValidClientType(uint32 clientIp) = 0;
	virtual int isClientAllowedToGetFile(uint32 clientIp, const char *username, const char *filename) = 0;
};


// The callback Interface used by the UPNP discovery process
class UPNPInitInterface {
public:
	virtual void UPNPInitStatus(bool result) = 0;
};

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
	static void Inet_NtoA(uint32 addr, char * ipbuf);

	unsigned char getByte(int byteIndex)	{return bytes[byteIndex];}
	string getString() const;
};

// =====================================================
//	class Socket
// =====================================================

#ifdef WIN32
class SocketManager{
public:
	SocketManager();
	~SocketManager();
};
#endif

class Socket {

protected:
#ifdef WIN32
	static SocketManager wsaManager;
#endif
	PLATFORM_SOCKET sock;
	time_t lastDebugEvent;
	static int broadcast_portno;
	std::string ipAddress;
	std::string connectedIpAddress;

	//SimpleTaskThread *pingThread;
	std::map<string,double> pingCache;
	time_t lastThreadedPing;
	//Mutex pingThreadAccessor;

	Mutex *dataSynchAccessorRead;
	Mutex *dataSynchAccessorWrite;

	Mutex *inSocketDestructorSynchAccessor;
	bool inSocketDestructor;

public:
	Socket(PLATFORM_SOCKET sock);
	Socket();
	virtual ~Socket();

	static bool disableNagle;
	static int DEFAULT_SOCKET_SENDBUF_SIZE;
	static int DEFAULT_SOCKET_RECVBUF_SIZE;

	//virtual void simpleTask(BaseThread *callingThread);

	static int getBroadCastPort() 			{ return broadcast_portno; }
	static void setBroadCastPort(int value) { broadcast_portno = value; }
	static std::vector<std::string> getLocalIPAddressList();

    // Int lookup is socket fd while bool result is whether or not that socket was signalled for reading
    static bool hasDataToRead(std::map<PLATFORM_SOCKET,bool> &socketTriggeredList);
    static bool hasDataToRead(PLATFORM_SOCKET socket);
    bool hasDataToRead();

    static bool hasDataToReadWithWait(PLATFORM_SOCKET socket,int waitMilliseconds);
    bool hasDataToReadWithWait(int waitMilliseconds);

    virtual void disconnectSocket();

    PLATFORM_SOCKET getSocketId() const { return sock; }

	int getDataToRead(bool wantImmediateReply=false);
	int send(const void *data, int dataSize);
	int receive(void *data, int dataSize, bool tryReceiveUntilDataSizeMet);
	int peek(void *data, int dataSize, bool mustGetData=true,int *pLastSocketError=NULL);

	void setBlock(bool block);
	static void setBlock(bool block, PLATFORM_SOCKET socket);

	bool isReadable();
	bool isWritable(struct timeval *timeVal=NULL);
	bool isConnected();

	static string getHostName();
	static string getIp();
	bool isSocketValid() const;
	static bool isSocketValid(const PLATFORM_SOCKET *validateSocket);

	static double getAveragePingMS(std::string host, int pingCount=5);
	float getThreadedPingMS(std::string host);

	virtual std::string getIpAddress();
	virtual void setIpAddress(std::string value) { ipAddress = value; }

	uint32 getConnectedIPAddress(string IP="");

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
	Mutex *mutexPauseBroadcast;
	bool pauseBroadcast;

public:
	BroadCastSocketThread();
	virtual ~BroadCastSocketThread();
    virtual void execute();
    virtual bool canShutdown(bool deleteSelfIfShutdownDelayed=false);

    bool getPauseBroadcast();
    void setPauseBroadcast(bool value);
};

// =====================================================
//	class ServerSocket
// =====================================================
class ServerSocket: public Socket, public UPNPInitInterface {
protected:

	bool portBound;
	int boundPort;
	string bindSpecificAddress;

	static int externalPort;
	static int ftpServerPort;

	static int maxPlayerCount;

    virtual void UPNPInitStatus(bool result);
	BroadCastSocketThread *broadCastThread;
	void startBroadCastThread();
	void resumeBroadcast();
	bool isBroadCastThreadRunning();
	vector<string> blockIPList;

	bool basicMode;

public:
	ServerSocket(bool basicMode = false);
	virtual ~ServerSocket();
	void bind(int port);
	void listen(int connectionQueueSize= SOMAXCONN);
	Socket *accept(bool errorOnFail=true);
	void stopBroadCastThread();
    void pauseBroadcast();


	void addIPAddressToBlockedList(string value);
	bool isIPAddressBlocked(string value) const;
	void removeBlockedIPAddress(string value);
	void clearBlockedIPAddress();
	bool hasBlockedIPAddresses() const;

	void setBindPort(int port) { boundPort = port; }
	int getBindPort() const { return boundPort; }
	bool isPortBound() const { return portBound; }

	void setBindSpecificAddress(string value) { bindSpecificAddress = value;}

	static void setExternalPort(int port) { externalPort = port; }
	static int getExternalPort() { return externalPort; }

	static void setFTPServerPort(int port) { ftpServerPort = port; }
	static int getFTPServerPort() { return ftpServerPort; }

	virtual void disconnectSocket();

    void NETdiscoverUPnPDevices();

    static void setMaxPlayerCount(int value) { maxPlayerCount=value; }

	static Mutex mutexUpnpdiscoverThread;
	static SDL_Thread *upnpdiscoverThread;
};

// =====================================================
//	class UPNP_Tools
// =====================================================
class UPNP_Tools {

public:

    static bool isUPNP;
    static bool enabledUPNP;
    static Mutex mutexUPNP;

    static int upnp_init(void *param);

    static bool upnp_add_redirect(int ports[2],bool mutexLock=true);
    static void upnp_rem_redirect(int ext_port);

    static void NETaddRedirects(std::vector<int> UPNPPortForwardList, bool mutexLock=true);
    static void NETremRedirects(int ext_port);

    static void AddUPNPPortForward(int internalPort, int externalPort);
    static void RemoveUPNPPortForward(int internalPort, int externalPort);
};

}}//end namespace

#endif
