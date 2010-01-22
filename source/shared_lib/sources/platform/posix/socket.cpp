//This file is part of Glest Shared Library (www.glest.org)
//Copyright (C) 2005 Matthias Braun <matze@braunis.de>

//You can redistribute this code and/or modify it under 
//the terms of the GNU General Public License as published by the Free Software 
//Foundation; either version 2 of the License, or (at your option) any later 
//version.

#include "socket.h"

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <sstream>
#if defined(HAVE_SYS_IOCTL_H)
#define BSD_COMP /* needed for FIONREAD on Solaris2 */
#include <sys/ioctl.h>
#endif
#if defined(HAVE_SYS_FILIO_H) /* needed for FIONREAD on Solaris 2.5 */
#include <sys/filio.h>
#endif

#include "conversion.h"

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

// ===============================================
//	class Socket
// ===============================================

Socket::Socket(int sock){
	this->sock= sock;
}

Socket::Socket(){
	sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sock<0) {
		throwException("Error creating socket");
	}
}

Socket::~Socket() {
	::close(sock);
}

int Socket::getDataToRead(){
	unsigned long size;

	/* ioctl isn't posix, but the following seems to work on all modern
	 * unixes */
	int err= ioctl(sock, FIONREAD, &size);

	if(err < 0 && errno != EAGAIN){
		throwException("Can not get data to read");
	}

	return static_cast<int>(size);
}

int Socket::send(const void *data, int dataSize) {
	ssize_t bytesSent= ::send(sock, reinterpret_cast<const char*>(data), dataSize, 0);
	if(bytesSent<0 && errno != EAGAIN) {
		throwException("error while receiving socket data");
	}
	return static_cast<int>(bytesSent);
}

int Socket::receive(void *data, int dataSize) {
	ssize_t bytesReceived= recv(sock, reinterpret_cast<char*>(data), dataSize, 0);
	if(bytesReceived<0 && errno != EAGAIN) {
		throwException("error while receiving socket data");
	}
	return static_cast<int>(bytesReceived);
}

int Socket::peek(void *data, int dataSize){
	ssize_t err= recv(sock, reinterpret_cast<char*>(data), dataSize, MSG_PEEK);
	if(err<0 && errno != EAGAIN){
		throwException("Can not receive data");
	}

	return static_cast<int>(err);
}

void Socket::setBlock(bool block){
	int err= fcntl(sock, F_SETFL, block ? 0 : O_NONBLOCK);
	if(err<0){
		throwException("Error setting I/O mode for socket");
	}
}

bool Socket::isReadable(){
	struct timeval tv;
	tv.tv_sec= 0;
	tv.tv_usec= 10;

	fd_set set;
	FD_ZERO(&set);
	FD_SET(sock, &set);

	int i= select(sock+1, &set, NULL, NULL, &tv);
	if(i<0){
		throwException("Error selecting socket");
	}
	return i==1;
}

bool Socket::isWritable(){
	struct timeval tv;
	tv.tv_sec= 0;
	tv.tv_usec= 10;

	fd_set set;
	FD_ZERO(&set);
	FD_SET(sock, &set);

	int i= select(sock+1, NULL, &set, NULL, &tv);
	if(i<0){
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

string Socket::getHostName() const {
	const int strSize= 256;
	char hostname[strSize];
	gethostname(hostname, strSize);
	return hostname;
}

string Socket::getIp() const{
	hostent* info= gethostbyname(getHostName().c_str());
	unsigned char* address;

	if(info==NULL){
		throw runtime_error("Error getting host by name");
	}

	address= reinterpret_cast<unsigned char*>(info->h_addr_list[0]);

	if(address==NULL){
		throw runtime_error("Error getting host ip");
	}

	return 
		intToStr(address[0]) + "." + 
		intToStr(address[1]) + "." + 
		intToStr(address[2]) + "." +
		intToStr(address[3]);
}

void Socket::throwException(const string &str){
	std::stringstream msg;
	msg << str << " (Error: " << strerror(errno) << ")";
	throw runtime_error(msg.str());
}

// ===============================================
//	class ClientSocket
// ===============================================

void ClientSocket::connect(const Ip &ip, int port){
	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));

    addr.sin_family= AF_INET;
	addr.sin_addr.s_addr= inet_addr(ip.getString().c_str());
	addr.sin_port= htons(port);

	int err= ::connect(sock, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
	if(err < 0) {
	    char szBuf[1024]="";
	    sprintf(szBuf,"#2 Error connecting socket for IP: %s for Port: %d err = %d errno = %d [%s]",ip.getString().c_str(),port,err,errno,strerror(errno));
	    fprintf(stderr, "%s", szBuf);

        if (errno == EINPROGRESS) {

            fd_set myset;
            struct timeval tv;
            int valopt;
            socklen_t lon;

            fprintf(stderr, "EINPROGRESS in connect() - selecting\n");
            do {
               tv.tv_sec = 10;
               tv.tv_usec = 0;

               FD_ZERO(&myset);
               FD_SET(sock, &myset);

               err = select(sock+1, NULL, &myset, NULL, &tv);

               if (err < 0 && errno != EINTR) {
                  sprintf(szBuf, "Error connecting %d - %s\n", errno, strerror(errno));
                  throwException(szBuf);
               }
               else if (err > 0) {
                  // Socket selected for write
                  lon = sizeof(int);
                  if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0) {
                     sprintf(szBuf, "Error in getsockopt() %d - %s\n", errno, strerror(errno));
                     throwException(szBuf);
                  }
                  // Check the value returned...
                  if (valopt) {
                     sprintf(szBuf, "Error in delayed connection() %d - %s\n", valopt, strerror(valopt));
                     throwException(szBuf);
                  }

                  errno = 0;
                  fprintf(stderr, "Apparent recovery for connection sock = %d, err = %d, errno = %d\n",sock,err,errno);

                  break;
               }
               else {
                  sprintf(szBuf, "Timeout in select() - Cancelling!\n");
                  throwException(szBuf);
               }
            } while (1);
        }

        if(err < 0)
        {
            throwException(szBuf);
        }

        fprintf(stderr, "Valid recovery for connection sock = %d, err = %d, errno = %d\n",sock,err,errno);
	}
}

// ===============================================
//	class ServerSocket
// ===============================================

void ServerSocket::bind(int port){
	//sockaddr structure
	sockaddr_in addr;
	addr.sin_family= AF_INET;
	addr.sin_addr.s_addr= INADDR_ANY;
	addr.sin_port= htons(port);

	int val = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
	
	int err= ::bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
	if(err < 0) {
		throwException("Error binding socket");
	}
}

void ServerSocket::listen(int connectionQueueSize){
	int err= ::listen(sock, connectionQueueSize);
	if(err < 0) {
		throwException("Error listening socket");
	}
}

Socket *ServerSocket::accept(){
	int newSock= ::accept(sock, NULL, NULL);
	if(newSock < 0) {
		if(errno == EAGAIN)
			return NULL;

		throwException("Error accepting socket connection");
	}
	return new Socket(newSock);
}

}}//end namespace

