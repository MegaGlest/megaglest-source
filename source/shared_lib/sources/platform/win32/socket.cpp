// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2007 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#include "socket.h"

#include <stdexcept>

#include "conversion.h"

#include "leak_dumper.h"

using namespace std;
using namespace Shared::Util;

namespace Shared{ namespace Platform{

// =====================================================
//	class Ip
// =====================================================

Ip::Ip(){
	bytes[0]= 0;
	bytes[1]= 0;
	bytes[2]= 0;
	bytes[3]= 0;
}

Ip::Ip(unsigned char byte0, unsigned char byte1, unsigned char byte2, unsigned char byte3){
	bytes[0]= byte0;
	bytes[1]= byte1;
	bytes[2]= byte2;
	bytes[3]= byte3;
}


Ip::Ip(const string& ipString){
	int offset= 0; 
	int byteIndex= 0;

	for(byteIndex= 0; byteIndex<4; ++byteIndex){
		int dotPos= ipString.find_first_of('.', offset);

		bytes[byteIndex]= atoi(ipString.substr(offset, dotPos-offset).c_str());
		offset= dotPos+1;
	}
}

string Ip::getString() const{
	return intToStr(bytes[0]) + "." + intToStr(bytes[1]) + "." + intToStr(bytes[2]) + "." + intToStr(bytes[3]);
}

// =====================================================
//	class Socket
// =====================================================

Socket::SocketManager Socket::socketManager;

Socket::SocketManager::SocketManager(){
	WSADATA wsaData; 
	WORD wVersionRequested = MAKEWORD(2, 0);
	WSAStartup(wVersionRequested, &wsaData);
	//dont throw exceptions here, this is a static initializacion
}

Socket::SocketManager::~SocketManager(){
	WSACleanup();
}

Socket::Socket(SOCKET sock){
	this->sock= sock;
}

Socket::Socket(){
	sock= socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if(sock==INVALID_SOCKET){
		throwException("Error creating socket");
	}
}

Socket::~Socket(){
	int err= closesocket(sock);
	if(err==INVALID_SOCKET){
		throwException("Error closing socket");
	}
}

int Socket::getDataToRead(){
	u_long size;
	
	int err= ioctlsocket(sock, FIONREAD, &size);

	if(err==SOCKET_ERROR){
		if(WSAGetLastError()!=WSAEWOULDBLOCK){
			throwException("Can not get data to read");
		}
	}

	return static_cast<int>(size);
}

int Socket::send(const void *data, int dataSize){
	int err= ::send(sock, reinterpret_cast<const char*>(data), dataSize, 0);
	if(err==SOCKET_ERROR){
		if(WSAGetLastError()!=WSAEWOULDBLOCK){
			throwException("Can not send data");
		}
	}
	return err;
}

int Socket::receive(void *data, int dataSize){
	int err= recv(sock, reinterpret_cast<char*>(data), dataSize, 0);

	if(err==SOCKET_ERROR){
		if(WSAGetLastError()!=WSAEWOULDBLOCK){
			throwException("Can not receive data");
		}
	}

	return err;
}

int Socket::peek(void *data, int dataSize){
	int err= recv(sock, reinterpret_cast<char*>(data), dataSize, MSG_PEEK);

	if(err==SOCKET_ERROR){
		if(WSAGetLastError()!=WSAEWOULDBLOCK){
			throwException("Can not receive data");
		}
	}

	return err;
}

void Socket::setBlock(bool block){
	u_long iMode= !
		block;
	int err= ioctlsocket(sock, FIONBIO, &iMode);
	if(err==SOCKET_ERROR){
		throwException("Error setting I/O mode for socket");
	}
}

bool Socket::isReadable(){
	TIMEVAL tv;
	tv.tv_sec= 0;
	tv.tv_usec= 10;

	fd_set set;
	FD_ZERO(&set);
	FD_SET(sock, &set);

	int i= select(0, &set, NULL, NULL, &tv);
	if(i==SOCKET_ERROR){
		throwException("Error selecting socket");
	}
	return i==1;
}

bool Socket::isWritable(){
	TIMEVAL tv;
	tv.tv_sec= 0;
	tv.tv_usec= 10;

	fd_set set;
	FD_ZERO(&set);
	FD_SET(sock, &set);

	int i= select(0, NULL, &set, NULL, &tv);
	if(i==SOCKET_ERROR){
		throwException("Error selecting socket");
	}
	return i==1;
}

bool Socket::isConnected(){

	//if the socket is not writable then it is not conencted
	if(!isWritable()){
		return false;
	}

	//if the socket is readable it is connected if we can read a byte from it
	if(isReadable()){
		char tmp;
		return recv(sock, &tmp, sizeof(tmp), MSG_PEEK) > 0;
	}

	//otherwise the socket is connected
	return true;
}

string Socket::getHostName() const{
	const int strSize= 256;
	char hostname[strSize];
	gethostname(hostname, strSize);
	return hostname;
}

string Socket::getIp() const{
	hostent* info= gethostbyname(getHostName().c_str());
	unsigned char* address;

	if(info==NULL){
		throwException("Error getting host by name");
	}

	address= reinterpret_cast<unsigned char*>(info->h_addr_list[0]);

	if(address==NULL)
	{
		throwException("Error getting host ip");
	}

	return 
		intToStr(address[0]) + "." + 
		intToStr(address[1]) + "." + 
		intToStr(address[2]) + "." +
		intToStr(address[3]);
}

void Socket::throwException(const string &str){
	throw runtime_error("Network error: " + str+" (Code: " + intToStr(WSAGetLastError())+")");
}

// =====================================================
//	class ClientSocket
// =====================================================

void ClientSocket::connect(const Ip &ip, int port){
	sockaddr_in addr;

    addr.sin_family= AF_INET;
	addr.sin_addr.s_addr= inet_addr(ip.getString().c_str());
	addr.sin_port= htons(port);

	int err= ::connect(sock, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
	if(err==SOCKET_ERROR){
		int lastError= WSAGetLastError();

		if(lastError!=WSAEWOULDBLOCK && lastError!=WSAEALREADY){
			throwException("Can not connect");
		}
	}
}

// =====================================================
//	class ServerSocket
// =====================================================

void ServerSocket::bind(int port){
	//sockaddr structure
	sockaddr_in addr;
	addr.sin_family= AF_INET;
	addr.sin_addr.s_addr= INADDR_ANY;
	addr.sin_port= htons(port);
	
	int err= ::bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
	if(err==SOCKET_ERROR){
		throwException("Error binding socket");
	}
}

void ServerSocket::listen(int connectionQueueSize){
	int err= ::listen(sock, connectionQueueSize);
	if(err==SOCKET_ERROR){
		throwException("Error listening socket");
	}
}

Socket *ServerSocket::accept(){
	SOCKET newSock= ::accept(sock, NULL, NULL);
	if(newSock==INVALID_SOCKET){
		if(WSAGetLastError()==WSAEWOULDBLOCK){
			return NULL;
		}
		throwException("Error accepting socket connection");
	}
	return new Socket(newSock);
}

}}//end namespace
