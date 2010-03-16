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

bool Socket::enableDebugText = true;
bool Socket::enableNetworkDebugInfo = true;

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

Socket::Socket()
{
	sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sock < 0)
	{
		throwException("Error creating socket");
	}
}

Socket::~Socket()
{
    if(Socket::enableNetworkDebugInfo) printf("In [%s::%s] START closing socket = %d...\n",__FILE__,__FUNCTION__,sock);

    disconnectSocket();

	if(Socket::enableNetworkDebugInfo) printf("In [%s::%s] END closing socket = %d...\n",__FILE__,__FUNCTION__,sock);
}

void Socket::disconnectSocket()
{
    if(Socket::enableNetworkDebugInfo) printf("In [%s::%s] START closing socket = %d...\n",__FILE__,__FUNCTION__,sock);

    if(sock > 0)
    {
        if(Socket::enableNetworkDebugInfo) printf("In [%s::%s] calling shutdown and close for socket = %d...\n",__FILE__,__FUNCTION__,sock);
        ::shutdown(sock,2);
        ::close(sock);
        sock = -1;
    }

    if(Socket::enableNetworkDebugInfo) printf("In [%s::%s] END closing socket = %d...\n",__FILE__,__FUNCTION__,sock);
}

// Int lookup is socket fd while bool result is whether or not that socket was signalled for reading
bool Socket::hasDataToRead(std::map<int,bool> &socketTriggeredList)
{
    bool bResult = false;

    if(socketTriggeredList.size() > 0)
    {
        /* Watch stdin (fd 0) to see when it has input. */
        fd_set rfds;
        FD_ZERO(&rfds);

        int imaxsocket = 0;
        for(std::map<int,bool>::iterator itermap = socketTriggeredList.begin();
            itermap != socketTriggeredList.end(); itermap++)
        {
            int socket = itermap->first;
            if(socket > 0)
            {
                FD_SET(socket, &rfds);
                imaxsocket = max(socket,imaxsocket);
            }
        }

        if(imaxsocket > 0)
        {
            /* Wait up to 0 seconds. */
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 0;

            int retval = select(imaxsocket + 1, &rfds, NULL, NULL, &tv);
            if(retval < 0)
            {
                char szBuf[1024]="";
                sprintf(szBuf,"In [%s::%s] ERROR SELECTING SOCKET DATA retval = %d errno = %d [%s]",__FILE__,__FUNCTION__,retval,errno,strerror(errno));
                fprintf(stderr, "%s", szBuf);

            }
            else if(retval)
            {
                bResult = true;

                if(Socket::enableNetworkDebugInfo) printf("In [%s::%s] select detected data imaxsocket = %d...\n",__FILE__,__FUNCTION__,imaxsocket);

                for(std::map<int,bool>::iterator itermap = socketTriggeredList.begin();
                    itermap != socketTriggeredList.end(); itermap++)
                {
                    int socket = itermap->first;
                    if (FD_ISSET(socket, &rfds))
                    {
                        if(Socket::enableNetworkDebugInfo) printf("In [%s] FD_ISSET true for socket %d...\n",__FUNCTION__,socket);

                        itermap->second = true;
                    }
                    else
                    {
                        itermap->second = false;
                    }
                }

                if(Socket::enableNetworkDebugInfo) printf("In [%s::%s] socketTriggeredList->size() = %d\n",__FILE__,__FUNCTION__,socketTriggeredList.size());
            }
        }
    }

    return bResult;
}

bool Socket::hasDataToRead()
{
    return Socket::hasDataToRead(sock) ;
}

bool Socket::hasDataToRead(int socket)
{
    bool bResult = false;

    if(socket > 0)
    {
        fd_set rfds;
        struct timeval tv;

        /* Watch stdin (fd 0) to see when it has input. */
        FD_ZERO(&rfds);
        FD_SET(socket, &rfds);

        /* Wait up to 0 seconds. */
        tv.tv_sec = 0;
        tv.tv_usec = 0;

        int retval = select(socket + 1, &rfds, NULL, NULL, &tv);
        if(retval)
        {
            if (FD_ISSET(socket, &rfds))
            {
                bResult = true;
            }
        }
    }

    return bResult;
}

int Socket::getDataToRead(){
	unsigned long size = 0;

    //fd_set rfds;
    //struct timeval tv;
    //int retval;

    /* Watch stdin (fd 0) to see when it has input. */
    //FD_ZERO(&rfds);
    //FD_SET(sock, &rfds);

    /* Wait up to 0 seconds. */
    //tv.tv_sec = 0;
    //tv.tv_usec = 0;

    //retval = select(sock + 1, &rfds, NULL, NULL, &tv);
    //if(retval)
    if(sock > 0)
    {
        /* ioctl isn't posix, but the following seems to work on all modern
         * unixes */
        int err= ioctl(sock, FIONREAD, &size);

        if(err < 0 && errno != EAGAIN)
        {
            char szBuf[1024]="";
            sprintf(szBuf,"In [%s::%s] ERROR PEEKING SOCKET DATA, err = %d errno = %d [%s]\n",__FILE__,__FUNCTION__,err,errno,strerror(errno));
            //throwException(szBuf);
            printf("%s",szBuf);
        }
        else if(err == 0)
        {
            //if(Socket::enableNetworkDebugInfo) printf("In [%s] ioctl returned = %d, size = %ld\n",__FUNCTION__,err,size);
        }
    }

	return static_cast<int>(size);
}

int Socket::send(const void *data, int dataSize) {
	ssize_t bytesSent= 0;
	if(sock > 0)
	{
        bytesSent = ::send(sock, reinterpret_cast<const char*>(data), dataSize, 0);
	}
	if(bytesSent < 0 && errno != EAGAIN)
	{
        char szBuf[1024]="";
        sprintf(szBuf,"In [%s::%s] ERROR WRITING SOCKET DATA, err = %d errno = %d [%s]\n",__FILE__,__FUNCTION__,bytesSent,errno,strerror(errno));
		//throwException(szBuf);
		printf("%s",szBuf);
	}
	else if(bytesSent < 0 && errno == EAGAIN)
	{
	    printf("In [%s::%s] #1 EAGAIN during send, trying again...\n",__FILE__,__FUNCTION__);

	    time_t tStartTimer = time(NULL);
	    while((bytesSent < 0 && errno == EAGAIN) && (difftime(time(NULL),tStartTimer) <= 5))
	    {
	        if(Socket::isWritable(true) == true)
	        {
                bytesSent = ::send(sock, reinterpret_cast<const char*>(data), dataSize, 0);

                printf("In [%s::%s] #2 EAGAIN during send, trying again returned: %d\n",__FILE__,__FUNCTION__,bytesSent);
	        }
	    }
	}
	if(bytesSent <= 0)
	{
	    int iErr = errno;
	    disconnectSocket();

        char szBuf[1024]="";
        sprintf(szBuf,"[%s::%s] DISCONNECTED SOCKET error while sending socket data, bytesSent = %d, errno = %d [%s]\n",__FILE__,__FUNCTION__,bytesSent,iErr,strerror(iErr));
	    printf("%s",szBuf);
	    //throwException(szBuf);
	}

    if(Socket::enableNetworkDebugInfo) printf("In [%s::%s] sock = %d, bytesSent = %d\n",__FILE__,__FUNCTION__,sock,bytesSent);

	return static_cast<int>(bytesSent);
}

int Socket::receive(void *data, int dataSize)
{
	ssize_t bytesReceived = 0;

	if(sock > 0)
	{
	    bytesReceived = recv(sock, reinterpret_cast<char*>(data), dataSize, 0);
	}
	if(bytesReceived < 0 && errno != EAGAIN)
	{
        char szBuf[1024]="";
        sprintf(szBuf,"[%s::%s] ERROR READING SOCKET DATA error while sending socket data, bytesSent = %d, errno = %d [%s]\n",__FILE__,__FUNCTION__,bytesReceived,errno,strerror(errno));
		//throwException(szBuf);
		printf("%s",szBuf);
	}
	else if(bytesReceived < 0 && errno == EAGAIN)
	{
	    printf("In [%s::%s] #1 EAGAIN during receive, trying again...\n",__FILE__,__FUNCTION__);

	    time_t tStartTimer = time(NULL);
	    while((bytesReceived < 0 && errno == EAGAIN) && (difftime(time(NULL),tStartTimer) <= 5))
	    {
	        if(Socket::isReadable() == true)
	        {
                bytesReceived = recv(sock, reinterpret_cast<char*>(data), dataSize, 0);

                printf("In [%s::%s] #2 EAGAIN during receive, trying again returned: %d\n",__FILE__,__FUNCTION__,bytesReceived);
	        }
	    }
	}

	if(bytesReceived <= 0)
	{
	    int iErr = errno;
	    disconnectSocket();

        char szBuf[1024]="";
        sprintf(szBuf,"[%s::%s] DISCONNECTED SOCKET error while receiving socket data, bytesReceived = %d, errno = %d [%s]\n",__FILE__,__FUNCTION__,bytesReceived,iErr,strerror(iErr));
        printf("%s",szBuf);
	    //throwException(szBuf);
	}
	return static_cast<int>(bytesReceived);
}

int Socket::peek(void *data, int dataSize){
	ssize_t err = 0;
	if(sock > 0)
	{
	    err = recv(sock, reinterpret_cast<char*>(data), dataSize, MSG_PEEK);
	}
	if(err < 0 && errno != EAGAIN)
	{
	    char szBuf[1024]="";
        sprintf(szBuf,"[%s::%s] ERROR PEEKING SOCKET DATA error while sending socket data, bytesSent = %d, errno = %d [%s]\n",__FILE__,__FUNCTION__,err,errno,strerror(errno));
		//throwException(szBuf);

		disconnectSocket();
	}
	else if(err < 0 && errno == EAGAIN)
	{
	    printf("In [%s::%s] #1 EAGAIN during peek, trying again...\n",__FILE__,__FUNCTION__);

	    time_t tStartTimer = time(NULL);
	    while((err < 0 && errno == EAGAIN) && (difftime(time(NULL),tStartTimer) <= 5))
	    {
	        if(Socket::isReadable() == true)
	        {
                err = recv(sock, reinterpret_cast<char*>(data), dataSize, MSG_PEEK);

                printf("In [%s::%s] #2 EAGAIN during peek, trying again returned: %d\n",__FILE__,__FUNCTION__,err);
	        }
	    }
	}

	if(err <= 0)
	{
	    int iErr = errno;
	    disconnectSocket();

        char szBuf[1024]="";
        sprintf(szBuf,"[%s::%s] DISCONNECTED SOCKET error while peeking socket data, err = %d, errno = %d [%s]\n",__FILE__,__FUNCTION__,err,iErr,strerror(iErr));
        printf("%s",szBuf);
	    //throwException(szBuf);
	}

	return static_cast<int>(err);
}

void Socket::setBlock(bool block){
	int err= fcntl(sock, F_SETFL, block ? 0 : O_NONBLOCK);
	if(err<0){
		throwException("Error setting I/O mode for socket");
	}
}

bool Socket::isReadable()
{
    if(sock <= 0) return false;

	struct timeval tv;
	tv.tv_sec= 0;
	tv.tv_usec= 1;

	fd_set set;
	FD_ZERO(&set);
	FD_SET(sock, &set);

	int i= select(sock+1, &set, NULL, NULL, &tv);
	if(i < 0)
	{
        if(difftime(time(NULL),lastDebugEvent) >= 1)
        {
            lastDebugEvent = time(NULL);

            //throwException("Error selecting socket");
            char szBuf[1024]="";
            sprintf(szBuf,"[%s::%s] error while selecting socket data, err = %d, errno = %d [%s]\n",__FILE__,__FUNCTION__,i,errno,strerror(errno));
            printf("%s",szBuf);
        }
	}
	//return (i == 1 && FD_ISSET(sock, &set));
	return (i == 1);
}

bool Socket::isWritable(bool waitOnDelayedResponse)
{
    if(sock <= 0) return false;

	struct timeval tv;
	tv.tv_sec= 0;
	tv.tv_usec= 1;

	fd_set set;
	FD_ZERO(&set);
	FD_SET(sock, &set);

    bool result = false;
    do
    {
        int i = select(sock+1, NULL, &set, NULL, &tv);
        if(i < 0 )
        {
            if(difftime(time(NULL),lastDebugEvent) >= 1)
            {
                lastDebugEvent = time(NULL);

                char szBuf[1024]="";
                sprintf(szBuf,"[%s::%s] error while selecting socket data, err = %d, errno = %d [%s]\n",__FILE__,__FUNCTION__,i,errno,strerror(errno));
                printf("%s",szBuf);
            }
            waitOnDelayedResponse = false;

            //throwException("Error selecting socket");
        }
        else if(i == 0)
        {
            if(difftime(time(NULL),lastDebugEvent) >= 1)
            {
                lastDebugEvent = time(NULL);
                char szBuf[1024]="";
                sprintf(szBuf,"[%s::%s] TIMEOUT while selecting socket data, err = %d, errno = %d [%s]\n",__FILE__,__FUNCTION__,i,errno,strerror(errno));
                printf("%s",szBuf);
            }

            if(waitOnDelayedResponse == false)
            {
                result = true;
            }
        }
        else
        {
            result = true;
        }
    } while(waitOnDelayedResponse == true && result == false);

	//return (i == 1 && FD_ISSET(sock, &set));
	return result;
}

bool Socket::isConnected()
{
	//if the socket is not writable then it is not conencted
	if(isWritable(false) == false)
	{
		return false;
	}

	//if the socket is readable it is connected if we can read a byte from it
	if(isReadable())
	{
		char tmp;
		int err = peek(&tmp, sizeof(tmp));
		return (err > 0);
		/*
		int err = recv(sock, &tmp, sizeof(tmp), MSG_PEEK);

        if(err <= 0 && errno != EAGAIN)
        {
            int iErr = errno;
            disconnectSocket();

            char szBuf[1024]="";
            sprintf(szBuf,"[%s::%s] DISCONNECTED SOCKET error while peeking isconnected socket data, err = %d, errno = %d [%s]\n",__FILE__,__FUNCTION__,err,iErr,strerror(iErr));
            printf("%s",szBuf);

            return false;
        }
        else if(err <= 0)
        {
            int iErr = errno;
            //disconnectSocket();

            char szBuf[1024]="";
            sprintf(szBuf,"[%s::%s] #2 DISCONNECTED SOCKET error while peeking isconnected socket data, err = %d, errno = %d [%s]\n",__FILE__,__FUNCTION__,err,iErr,strerror(iErr));
            printf("%s",szBuf);
        }
        */
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

void ClientSocket::connect(const Ip &ip, int port)
{
	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));

    addr.sin_family= AF_INET;
	addr.sin_addr.s_addr= inet_addr(ip.getString().c_str());
	addr.sin_port= htons(port);

	int err= ::connect(sock, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
	if(err < 0)
	{
	    char szBuf[1024]="";
	    sprintf(szBuf,"In [%s::%s] #2 Error connecting socket for IP: %s for Port: %d err = %d errno = %d [%s]\n",__FILE__,__FUNCTION__,ip.getString().c_str(),port,err,errno,strerror(errno));
	    fprintf(stderr, "%s", szBuf);

        if (errno == EINPROGRESS)
        {
            fd_set myset;
            struct timeval tv;
            int valopt;
            socklen_t lon;

            fprintf(stderr, "In [%s::%s] EINPROGRESS in connect() - selecting\n",__FILE__,__FUNCTION__);

            do
            {
               tv.tv_sec = 10;
               tv.tv_usec = 0;

               FD_ZERO(&myset);
               FD_SET(sock, &myset);

               err = select(sock+1, NULL, &myset, NULL, &tv);

               if (err < 0 && errno != EINTR)
               {
                  sprintf(szBuf, "In [%s::%s] Error connecting %d - [%s]\n",__FILE__,__FUNCTION__,errno, strerror(errno));
                  //throwException(szBuf);
                  fprintf(stderr, "%s", szBuf);
                  break;
               }
               else if (err > 0)
               {
                  // Socket selected for write
                  lon = sizeof(int);
                  if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0)
                  {
                     sprintf(szBuf, "In [%s::%s] Error in getsockopt() %d - [%s]\n",__FILE__,__FUNCTION__,errno, strerror(errno));
                     //throwException(szBuf);
                     fprintf(stderr, "%s", szBuf);
                     break;
                  }
                  // Check the value returned...
                  if (valopt)
                  {
                     sprintf(szBuf, "In [%s::%s] Error in delayed connection() %d - [%s]\n",__FILE__,__FUNCTION__,valopt, strerror(valopt));
                     //throwException(szBuf);
                     fprintf(stderr, "%s", szBuf);
                     break;
                  }

                  errno = 0;
                  fprintf(stderr, "In [%s::%s] Apparent recovery for connection sock = %d, err = %d, errno = %d\n",__FILE__,__FUNCTION__,sock,err,errno);

                  break;
               }
               else
               {
                  sprintf(szBuf, "In [%s::%s] Timeout in select() - Cancelling!\n",__FILE__,__FUNCTION__);
                  //throwException(szBuf);
                  fprintf(stderr, "%s", szBuf);

                  disconnectSocket();
                  break;
               }
            } while (1);
        }

        if(err < 0)
        {
            fprintf(stderr, "In [%s::%s] Before END sock = %d, err = %d, errno = %d [%s]\n",__FILE__,__FUNCTION__,sock,err,errno,strerror(errno));
            //throwException(szBuf);
            disconnectSocket();
        }
        else
        {
            fprintf(stderr, "In [%s::%s] Valid recovery for connection sock = %d, err = %d, errno = %d\n",__FILE__,__FUNCTION__,sock,err,errno);
        }
	}
}

// ===============================================
//	class ServerSocket
// ===============================================

void ServerSocket::bind(int port)
{
	//sockaddr structure
	sockaddr_in addr;
	addr.sin_family= AF_INET;
	addr.sin_addr.s_addr= INADDR_ANY;
	addr.sin_port= htons(port);

	int val = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

	int err= ::bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
	if(err < 0)
	{
	    char szBuf[1024]="";
	    sprintf(szBuf, "In [%s::%s] Error binding socket sock = %d, err = %d, errno = %d\n",__FILE__,__FUNCTION__,sock,err,errno);
		throwException(szBuf);
	}
}

void ServerSocket::listen(int connectionQueueSize)
{
	int err= ::listen(sock, connectionQueueSize);
	if(err < 0)
	{
	    char szBuf[1024]="";
	    sprintf(szBuf, "In [%s::%s] Error listening socket sock = %d, err = %d, errno = %d\n",__FILE__,__FUNCTION__,sock,err,errno);
		throwException(szBuf);
	}
}

Socket *ServerSocket::accept()
{
	int newSock= ::accept(sock, NULL, NULL);
	if(newSock < 0)
	{
	    char szBuf[1024]="";
	    if(Socket::enableNetworkDebugInfo) printf(szBuf, "In [%s::%s] Error accepting socket connection sock = %d, err = %d, errno = %d\n",__FILE__,__FUNCTION__,sock,newSock,errno);

		if(errno == EAGAIN)
		{
			return NULL;
		}
		throwException(szBuf);

	}
	return new Socket(newSock);
}


}}//end namespace

