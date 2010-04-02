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

#include <net/if.h>

#include "conversion.h"
#include "util.h"
#include "platform_util.h"

using namespace std;
using namespace Shared::Util;

namespace Shared{ namespace Platform{

int Socket::broadcast_portno = 61357;
BroadCastClientSocketThread *ClientSocket::broadCastClientThread = NULL;

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

std::vector<std::string> Socket::getLocalIPAddressList() {
	std::vector<std::string> ipList;

	/* get my host name */
	char myhostname[101]="";
	gethostname(myhostname,100);

	struct hostent* myhostent = gethostbyname(myhostname);

	// get all host IP addresses (Except for loopback)
	char myhostaddr[101] = "";
	int ipIdx = 0;
	while (myhostent->h_addr_list[ipIdx] != 0) {
	   sprintf(myhostaddr, "%s",inet_ntoa(*(struct in_addr *)myhostent->h_addr_list[ipIdx]));
	   //printf("%s\n",myhostaddr);
	   SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] myhostaddr = [%s]\n",__FILE__,__FUNCTION__,__LINE__,myhostaddr);

	   if(strlen(myhostaddr) > 0 && strncmp(myhostaddr,"127.",4) != 0) {
		   ipList.push_back(myhostaddr);
	   }
	   ipIdx++;
	}

	// Now check all linux network devices
	int fd = socket(AF_INET, SOCK_DGRAM, 0);

	/* I want to get an IPv4 IP address */
	struct ifreq ifr;
	ifr.ifr_addr.sa_family = AF_INET;

	/* I want IP address attached to "eth0" */
	strncpy(ifr.ifr_name, "eth1", IFNAMSIZ-1);
	ioctl(fd, SIOCGIFADDR, &ifr);
	close(fd);

    sprintf(myhostaddr, "%s",inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
	printf("%s\n",myhostaddr);

    if(strlen(myhostaddr) > 0 && strncmp(myhostaddr,"127.",4) != 0) {
	   ipList.push_back(myhostaddr);
    }

	return ipList;
}

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
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] START closing socket = %d...\n",__FILE__,__FUNCTION__,sock);

    disconnectSocket();

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] END closing socket = %d...\n",__FILE__,__FUNCTION__,sock);
}

void Socket::disconnectSocket()
{
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] START closing socket = %d...\n",__FILE__,__FUNCTION__,sock);

    if(sock > 0)
    {
        SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] calling shutdown and close for socket = %d...\n",__FILE__,__FUNCTION__,sock);
        ::shutdown(sock,2);
        ::close(sock);
        sock = -1;
    }

    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] END closing socket = %d...\n",__FILE__,__FUNCTION__,sock);
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

                SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] select detected data imaxsocket = %d...\n",__FILE__,__FUNCTION__,imaxsocket);

                for(std::map<int,bool>::iterator itermap = socketTriggeredList.begin();
                    itermap != socketTriggeredList.end(); itermap++)
                {
                    int socket = itermap->first;
                    if (FD_ISSET(socket, &rfds))
                    {
                        SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s] FD_ISSET true for socket %d...\n",__FUNCTION__,socket);

                        itermap->second = true;
                    }
                    else
                    {
                        itermap->second = false;
                    }
                }

                SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] socketTriggeredList->size() = %d\n",__FILE__,__FUNCTION__,socketTriggeredList.size());
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

    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] sock = %d, bytesSent = %d\n",__FILE__,__FUNCTION__,sock,bytesSent);

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
	setBlock(block,this->sock);
}

void Socket::setBlock(bool block, int socket){
	int err= fcntl(socket, F_SETFL, block ? 0 : O_NONBLOCK);
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

string Socket::getHostName()  {
	const int strSize= 256;
	char hostname[strSize];
	gethostname(hostname, strSize);
	return hostname;
}

string Socket::getIp() {
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

ClientSocket::ClientSocket() : Socket() {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//broadCastClientThread = NULL;
	stopBroadCastClientThread();

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

ClientSocket::~ClientSocket() {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	stopBroadCastClientThread();

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void ClientSocket::stopBroadCastClientThread() {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(broadCastClientThread != NULL) {
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		broadCastClientThread->signalQuit();

		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		for( time_t elapsed = time(NULL); difftime(time(NULL),elapsed) <= 5; ) {
			if(broadCastClientThread->getRunningStatus() == false) {
				break;
			}
			sleep(100);
			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		}
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		delete broadCastClientThread;
		broadCastClientThread = NULL;
	}

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void ClientSocket::startBroadCastClientThread(DiscoveredServersInterface *cb) {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	ClientSocket::stopBroadCastClientThread();

	broadCastClientThread = new BroadCastClientSocketThread(cb);
	broadCastClientThread->start();

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void ClientSocket::discoverServers(DiscoveredServersInterface *cb) {
	ClientSocket::startBroadCastClientThread(cb);
}

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

//=======================================================================
// Function :		discovery response thread
// Description:		Runs in its own thread to listen for broadcasts from
//					other servers
//
BroadCastClientSocketThread::BroadCastClientSocketThread(DiscoveredServersInterface *cb) {

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	setQuitStatus(false);
	setRunningStatus(false);
	discoveredServersCB = cb;

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void BroadCastClientSocketThread::signalQuit() {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	setQuitStatus(true);

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void BroadCastClientSocketThread::setQuitStatus(bool value) {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	mutexQuit.p();
	quit = value;
	mutexQuit.v();

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

bool BroadCastClientSocketThread::getQuitStatus() {
	//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	bool retval = false;
	mutexQuit.p();
	retval = quit;
	mutexQuit.v();

	//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	return retval;
}

bool BroadCastClientSocketThread::getRunningStatus() {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	bool retval = false;
	mutexRunning.p();
	retval = running;
	mutexRunning.v();

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] running = %d\n",__FILE__,__FUNCTION__,__LINE__,retval);

	return retval;
}

void BroadCastClientSocketThread::setRunningStatus(bool value) {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] value = %d\n",__FILE__,__FUNCTION__,__LINE__,value);

	mutexRunning.p();
	running = value;
	mutexRunning.v();

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] running = %d\n",__FILE__,__FUNCTION__,__LINE__,value);
}

//=======================================================================
// Function :		broadcast thread
// Description:		Runs in its own thread to send out a broadcast to the local network
//					the current broadcast message is <myhostname:my.ip.address.dotted>
//
void BroadCastClientSocketThread::execute() {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	std::vector<string> foundServers;

	short port;                 // The port for the broadcast.
    struct sockaddr_in bcSender; // local socket address for the broadcast.
    struct sockaddr_in bcaddr;  // The broadcast address for the receiver.
    int bcfd;                // The file descriptor used for the broadcast.
    bool one = true;            // Parameter for "setscokopt".
    char buff[10024];            // Buffers the data to be broadcasted.
    socklen_t alen;
    int nb;                     // The number of bytes read.

    port = htons( Socket::getBroadCastPort() );

    // Prepare to receive the broadcast.
    bcfd = socket(AF_INET, SOCK_DGRAM, 0);
    if( bcfd <= 0 )	{
    	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"socket failed: %d\n", errno);
		//exit(-1);
	}
    else {
		// Create the address we are receiving on.
		memset( (char*)&bcaddr, 0, sizeof(bcaddr));
		bcaddr.sin_family = AF_INET;
		bcaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		bcaddr.sin_port = port;

		int val = 1;
		setsockopt(bcfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

		if(bind( bcfd,  (struct sockaddr *)&bcaddr, sizeof(bcaddr) ) < 0 )	{
			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"bind failed: %d\n", errno);
		}
		else {
			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			Socket::setBlock(false, bcfd);

			setRunningStatus(true);
			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Broadcast Client thread is running\n");

			try
			{
				// Keep getting packets forever.
				for( time_t elapsed = time(NULL); difftime(time(NULL),elapsed) <= 5; )
				{
					alen = sizeof(struct sockaddr);
					if( (nb = recvfrom(bcfd, buff, 10024, 0, (struct sockaddr *) &bcSender, &alen)) <= 0  )
					{
						SystemFlags::OutputDebug(SystemFlags::debugNetwork,"recvfrom failed: %d\n", errno);
						//exit(-1);
					}
					else {
						SystemFlags::OutputDebug(SystemFlags::debugNetwork,"broadcast message received: [%s] from: [%s]\n", buff,inet_ntoa(bcSender.sin_addr) );

						vector<string> tokens;
						Tokenize(buff,tokens,":");
						for(int idx = 1; idx < tokens.size(); idx++) {
							foundServers.push_back(tokens[idx]);
						}
						break;
					}

					if(getQuitStatus() == true) {
						SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
						break;
					}
					sleep( 100 ); // send out broadcast every 1 seconds
				}
			}
			catch(const exception &ex) {
				SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
				setRunningStatus(false);
			}
			catch(...) {
				SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] unknown error\n",__FILE__,__FUNCTION__,__LINE__);
				setRunningStatus(false);
			}

			setRunningStatus(false);
			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Broadcast Client thread is exiting\n");
		}
    }

    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    // Here we callback into the implementer class
    if(discoveredServersCB != NULL) {
    	discoveredServersCB->DiscoveredServers(foundServers);
    }
    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

// ===============================================
//	class ServerSocket
// ===============================================

ServerSocket::ServerSocket() : Socket() {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	broadCastThread = NULL;

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

ServerSocket::~ServerSocket() {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	stopBroadCastThread();

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void ServerSocket::stopBroadCastThread() {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(broadCastThread != NULL) {
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		broadCastThread->signalQuit();

		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		for( time_t elapsed = time(NULL); difftime(time(NULL),elapsed) <= 5; ) {
			if(broadCastThread->getRunningStatus() == false) {
				break;
			}
			sleep(100);
			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		}
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		delete broadCastThread;
		broadCastThread = NULL;
	}

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void ServerSocket::startBroadCastThread() {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	stopBroadCastThread();

	broadCastThread = new BroadCastSocketThread();
	broadCastThread->start();

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

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

	broadCastThread = new BroadCastSocketThread();
	broadCastThread->start();

}

Socket *ServerSocket::accept()
{
	int newSock= ::accept(sock, NULL, NULL);
	if(newSock < 0)
	{
	    char szBuf[1024]="";
	    sprintf(szBuf, "In [%s::%s] Error accepting socket connection sock = %d, err = %d, errno = %d\n",__FILE__,__FUNCTION__,sock,newSock,errno);

		if(errno == EAGAIN)
		{
			return NULL;
		}
		throwException(szBuf);

	}
	return new Socket(newSock);
}


BroadCastSocketThread::BroadCastSocketThread() {

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	setQuitStatus(false);
	setRunningStatus(false);

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void BroadCastSocketThread::signalQuit() {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	setQuitStatus(true);

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void BroadCastSocketThread::setQuitStatus(bool value) {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	mutexQuit.p();
	quit = value;
	mutexQuit.v();

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

bool BroadCastSocketThread::getQuitStatus() {
	//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	bool retval = false;
	mutexQuit.p();
	retval = quit;
	mutexQuit.v();

	//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	return retval;
}

bool BroadCastSocketThread::getRunningStatus() {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	bool retval = false;
	mutexRunning.p();
	retval = running;
	mutexRunning.v();

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] running = %d\n",__FILE__,__FUNCTION__,__LINE__,retval);

	return retval;
}

void BroadCastSocketThread::setRunningStatus(bool value) {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] value = %d\n",__FILE__,__FUNCTION__,__LINE__,value);

	mutexRunning.p();
	running = value;
	mutexRunning.v();

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] running = %d\n",__FILE__,__FUNCTION__,__LINE__,value);
}

//=======================================================================
// Function :		broadcast_thread
// in		:		none
// return	:		none
// Description:		To be forked in its own thread to send out a broadcast to the local subnet
//					the current broadcast message is <myhostname:my.ip.address.dotted>
//
void BroadCastSocketThread::execute() {

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    unsigned int tbcaddr;       // The broadcast address.
    short port;                 // The port for the broadcast.
    struct sockaddr_in bcLocal; // local socket address for the broadcast.
    struct sockaddr_in bcaddr;  // The broadcast address for the receiver.
    int bcfd;                // The socket used for the broadcast.
    bool one = true;            // Parameter for "setscokopt".
    int pn;                     // The number of the packet broadcasted.
    char buff[1024];            // Buffers the data to be broadcasted.
    char myhostname[100];       // hostname of local machine
    struct in_addr myaddr;      // My host address in net format
    struct hostent* myhostent;
    char * ptr;                 // some transient vars
    int len,i;

    /* get my host name */
    gethostname(myhostname,100);
    myhostent = gethostbyname(myhostname);

    // get only the first host IP address
    std::vector<std::string> ipList = Socket::getLocalIPAddressList();

	port = htons( Socket::getBroadCastPort() );

	// Create the broadcast socket
	memset( &bcLocal, 0, sizeof( struct sockaddr_in));
	bcLocal.sin_family = AF_INET;
	bcLocal.sin_addr.s_addr = htonl( INADDR_BROADCAST );
	bcLocal.sin_port = port;  // We are letting the OS fill in the port number for the local machine.
	bcfd  = socket( AF_INET, SOCK_DGRAM, 0 );

	// If there is an error, report it and  terminate.
	if( bcfd <= 0  ) {
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Unable to allocate broadcast socket.: %d\n", errno);
		//exit(-1);
	}
	// Mark the socket for broadcast.
	else if( setsockopt( bcfd, SOL_SOCKET, SO_BROADCAST, (const char *) &one, sizeof( int ) ) < 0 ) {
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Could not set socket to broadcast.: %d\n", errno);
		//exit(-1);
	}
	// Bind the address to the broadcast socket.
	else {

		// Record the broadcast address of the receiver.
		bcaddr.sin_family = AF_INET;
		bcaddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);//tbcaddr;
		bcaddr.sin_port = port;

		setRunningStatus(true);
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Broadcast Server thread is running\n");

		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		try {
			// Send this machine's host name and address in hostname:n.n.n.n format
			sprintf(buff,"%s",myhostname);
			for(int idx = 0; idx < ipList.size(); idx++) {
				sprintf(buff,"%s:%s",buff,ipList[idx].c_str());
			}

			time_t elapsed = 0;
			for( pn = 1; ; pn++ )
			{
				if(difftime(time(NULL),elapsed) >= 1) {
					time_t elapsed = time(NULL);
					// Broadcast the packet to the subnet
					if( sendto( bcfd, buff, sizeof(buff) + 1, 0 , (struct sockaddr *)&bcaddr, sizeof(struct sockaddr_in) ) != sizeof(buff) + 1 )
					{
						SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Sendto error: %d\n", errno);
						//exit(-1);
					}
					else {
						//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Broadcasting to [%s] the message: [%s]\n",subnetmask,buff);
						SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Broadcasting on port [%d] the message: [%s]\n",Socket::getBroadCastPort(),buff);
					}

					SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
				}
				if(getQuitStatus() == true) {
					SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
					break;
				}
				sleep( 100 ); // send out broadcast every 1 seconds
			}
			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		}
		catch(const exception &ex) {
			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
			setRunningStatus(false);
		}
		catch(...) {
			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] unknown error\n",__FILE__,__FUNCTION__,__LINE__);
			setRunningStatus(false);
		}

		setRunningStatus(false);
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Broadcast thread is exiting\n");
	}

    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}


}}//end namespace

