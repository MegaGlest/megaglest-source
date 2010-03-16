// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_PLATFORM_SOCKET_H_
#define _SHARED_PLATFORM_SOCKET_H_

#include <string>
#include <winsock.h>
#include <map>

using std::string;

const char* WSAGetLastErrorMessage(const char* pcMessagePrefix,int nErrorID = 0);

namespace Shared{ namespace Platform{

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

class Socket{
private:
	class SocketManager{
	public:
		SocketManager();
		~SocketManager();
	};

protected:
	static SocketManager socketManager;
	SOCKET sock;

public:
	Socket(SOCKET sock);
	Socket();
	~Socket();

    static bool enableDebugText;
    static bool enableNetworkDebugInfo;

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
	bool isReadable();
	bool isWritable(bool waitOnDelayedResponse);
	bool isConnected();

	string getHostName() const;
	string getIp() const;

protected:
	static void throwException(const string &str);

};

// =====================================================
//	class ClientSocket
// =====================================================

class ClientSocket: public Socket{
public:
	void connect(const Ip &ip, int port);
};

// =====================================================
//	class ServerSocket
// =====================================================

class ServerSocket: public Socket{
public:
	void bind(int port);
	void listen(int connectionQueueSize= SOMAXCONN);
	Socket *accept();
};

}}//end namespace

#endif
