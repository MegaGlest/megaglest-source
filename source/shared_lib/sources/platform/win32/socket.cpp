// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2007 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "socket.h"

#include <stdexcept>

#include "conversion.h"
#include <time.h>
#include <strstream>
#include <algorithm>
#include "util.h"

#include "leak_dumper.h"

#define socklen_t int

using namespace std;
using namespace Shared::Util;

#define MAXHOSTNAME 254
//// Constants /////////////////////////////////////////////////////////
const int kBufferSize = 1024;

//// Statics ///////////////////////////////////////////////////////////
// List of Winsock error constants mapped to an interpretation string.
// Note that this list must remain sorted by the error constants'
// values, because we do a binary search on the list when looking up
// items.

static class ErrorEntry
{
public:
    int nID;
    const char* pcMessage;

    ErrorEntry(int id, const char* pc = 0) : nID(id), pcMessage(pc)
    {
    }

    bool operator<(const ErrorEntry& rhs)
    {
        return nID < rhs.nID;
    }

} gaErrorList[] =
  {
    ErrorEntry(0,                  "No error"),
    ErrorEntry(WSAEINTR,           "Interrupted system call"),
    ErrorEntry(WSAEBADF,           "Bad file number"),
    ErrorEntry(WSAEACCES,          "Permission denied"),
    ErrorEntry(WSAEFAULT,          "Bad address"),
    ErrorEntry(WSAEINVAL,          "Invalid argument"),
    ErrorEntry(WSAEMFILE,          "Too many open sockets"),
    ErrorEntry(WSAEWOULDBLOCK,     "Operation would block"),
    ErrorEntry(WSAEINPROGRESS,     "Operation now in progress"),
    ErrorEntry(WSAEALREADY,        "Operation already in progress"),
    ErrorEntry(WSAENOTSOCK,        "Socket operation on non-socket"),
    ErrorEntry(WSAEDESTADDRREQ,    "Destination address required"),
    ErrorEntry(WSAEMSGSIZE,        "Message too long"),
    ErrorEntry(WSAEPROTOTYPE,      "Protocol wrong type for socket"),
    ErrorEntry(WSAENOPROTOOPT,     "Bad protocol option"),
    ErrorEntry(WSAEPROTONOSUPPORT, "Protocol not supported"),
    ErrorEntry(WSAESOCKTNOSUPPORT, "Socket type not supported"),
    ErrorEntry(WSAEOPNOTSUPP,      "Operation not supported on socket"),
    ErrorEntry(WSAEPFNOSUPPORT,    "Protocol family not supported"),
    ErrorEntry(WSAEAFNOSUPPORT,    "Address family not supported"),
    ErrorEntry(WSAEADDRINUSE,      "Address already in use"),
    ErrorEntry(WSAEADDRNOTAVAIL,   "Can't assign requested address"),
    ErrorEntry(WSAENETDOWN,        "Network is down"),
    ErrorEntry(WSAENETUNREACH,     "Network is unreachable"),
    ErrorEntry(WSAENETRESET,       "Net connection reset"),
    ErrorEntry(WSAECONNABORTED,    "Software caused connection abort"),
    ErrorEntry(WSAECONNRESET,      "Connection reset by peer"),
    ErrorEntry(WSAENOBUFS,         "No buffer space available"),
    ErrorEntry(WSAEISCONN,         "Socket is already connected"),
    ErrorEntry(WSAENOTCONN,        "Socket is not connected"),
    ErrorEntry(WSAESHUTDOWN,       "Can't send after socket shutdown"),
    ErrorEntry(WSAETOOMANYREFS,    "Too many references, can't splice"),
    ErrorEntry(WSAETIMEDOUT,       "Connection timed out"),
    ErrorEntry(WSAECONNREFUSED,    "Connection refused"),
    ErrorEntry(WSAELOOP,           "Too many levels of symbolic links"),
    ErrorEntry(WSAENAMETOOLONG,    "File name too long"),
    ErrorEntry(WSAEHOSTDOWN,       "Host is down"),
    ErrorEntry(WSAEHOSTUNREACH,    "No route to host"),
    ErrorEntry(WSAENOTEMPTY,       "Directory not empty"),
    ErrorEntry(WSAEPROCLIM,        "Too many processes"),
    ErrorEntry(WSAEUSERS,          "Too many users"),
    ErrorEntry(WSAEDQUOT,          "Disc quota exceeded"),
    ErrorEntry(WSAESTALE,          "Stale NFS file handle"),
    ErrorEntry(WSAEREMOTE,         "Too many levels of remote in path"),
    ErrorEntry(WSASYSNOTREADY,     "Network system is unavailable"),
    ErrorEntry(WSAVERNOTSUPPORTED, "Winsock version out of range"),
    ErrorEntry(WSANOTINITIALISED,  "WSAStartup not yet called"),
    ErrorEntry(WSAEDISCON,         "Graceful shutdown in progress"),
    ErrorEntry(WSAHOST_NOT_FOUND,  "Host not found"),
    ErrorEntry(WSANO_DATA,         "No host data of that type was found")
};

bool operator<(const ErrorEntry& rhs1,const ErrorEntry& rhs2)
{
    return rhs1.nID < rhs2.nID;
}

const int kNumMessages = sizeof(gaErrorList) / sizeof(ErrorEntry);

//// WSAGetLastErrorMessage ////////////////////////////////////////////
// A function similar in spirit to Unix's perror() that tacks a canned
// interpretation of the value of WSAGetLastError() onto the end of a
// passed string, separated by a ": ".  Generally, you should implement
// smarter error handling than this, but for default cases and simple
// programs, this function is sufficient.
//
// This function returns a pointer to an internal static buffer, so you
// must copy the data from this function before you call it again.  It
// follows that this function is also not thread-safe.
const char* WSAGetLastErrorMessage(const char* pcMessagePrefix,
								   int nErrorID /* = 0 */)
{
    // Build basic error string
    static char acErrorBuffer[256];
    std::ostrstream outs(acErrorBuffer, sizeof(acErrorBuffer));
    outs << pcMessagePrefix << ": ";

    // Tack appropriate canned message onto end of supplied message
    // prefix. Note that we do a binary search here: gaErrorList must be
    // sorted by the error constant's value.
  	ErrorEntry* pEnd = gaErrorList + kNumMessages;
    ErrorEntry Target(nErrorID ? nErrorID : WSAGetLastError());
    ErrorEntry* it = std::lower_bound(gaErrorList, pEnd, Target);
    if ((it != pEnd) && (it->nID == Target.nID))
    {
        outs << it->pcMessage;
    }
    else
    {
        // Didn't find error in list, so make up a generic one
        outs << "unknown error";
    }
    outs << " (" << Target.nID << ")";

    // Finish error message off and return it.
    outs << std::ends;
    acErrorBuffer[sizeof(acErrorBuffer) - 1] = '\0';
    return acErrorBuffer;
}

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
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Winsock initialized.\n");
}

Socket::SocketManager::~SocketManager(){
	WSACleanup();
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Winsock cleanup complete.\n");
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
        ::shutdown(sock,2);
        ::closesocket(sock);
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
                sprintf(szBuf,"In [%s::%s] ERROR SELECTING SOCKET DATA retval = %d WSAGetLastError() = %d",__FILE__,__FUNCTION__,retval,WSAGetLastError());
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
        int err= ioctlsocket(sock, FIONREAD, &size);

        if(err < 0 && WSAGetLastError() != WSAEWOULDBLOCK)
        {
            char szBuf[1024]="";
            sprintf(szBuf,"In [%s::%s] ERROR PEEKING SOCKET DATA, err = %d WSAGetLastError() = %d\n",__FILE__,__FUNCTION__,err,WSAGetLastError());
            printf("%s",szBuf);
            //throwException(szBuf);
        }
        else if(err == 0)
        {
            //SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s] ioctl returned = %d, size = %ld\n",__FUNCTION__,err,size);
        }
    }

	return static_cast<int>(size);
}

int Socket::send(const void *data, int dataSize) {
	int bytesSent= 0;
	if(sock > 0)
	{
        bytesSent = ::send(sock, reinterpret_cast<const char*>(data), dataSize, 0);
	}
	if(bytesSent < 0 && WSAGetLastError() != WSAEWOULDBLOCK)
	{
        char szBuf[1024]="";
        sprintf(szBuf,"In [%s::%s] ERROR WRITING SOCKET DATA, err = %d WSAGetLastError() = %d\n",__FILE__,__FUNCTION__,bytesSent,WSAGetLastError());
		//throwException(szBuf);
		printf("%s",szBuf);
	}
	else if(bytesSent < 0 && WSAGetLastError() == WSAEWOULDBLOCK)
	{
	    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] #1 WSAEWOULDBLOCK during send, trying again...\n",__FILE__,__FUNCTION__);

	    time_t tStartTimer = time(NULL);
	    while((bytesSent < 0 && WSAGetLastError() == WSAEWOULDBLOCK) && (difftime(time(NULL),tStartTimer) <= 5))
	    {
	        if(Socket::isWritable(true) == true)
	        {
                bytesSent = ::send(sock, reinterpret_cast<const char*>(data), dataSize, 0);

                SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] #2 WSAEWOULDBLOCK during send, trying again returned: %d\n",__FILE__,__FUNCTION__,bytesSent);
	        }
	    }
	}
	if(bytesSent <= 0)
	{
	    int iErr = WSAGetLastError();
	    disconnectSocket();

        char szBuf[1024]="";
        sprintf(szBuf,"[%s::%s] DISCONNECTED SOCKET error while sending socket data, bytesSent = %d, WSAGetLastError() = %d\n",__FILE__,__FUNCTION__,bytesSent,iErr);
	    printf("%s",szBuf);
	    //throwException(szBuf);
	}

    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] sock = %d, bytesSent = %d\n",__FILE__,__FUNCTION__,sock,bytesSent);

	return static_cast<int>(bytesSent);
}

int Socket::receive(void *data, int dataSize)
{
	int bytesReceived = 0;

	if(sock > 0)
	{
	    bytesReceived = recv(sock, reinterpret_cast<char*>(data), dataSize, 0);
	}
	if(bytesReceived < 0 && WSAGetLastError() != WSAEWOULDBLOCK)
	{
        char szBuf[1024]="";
        sprintf(szBuf,"[%s::%s] ERROR READING SOCKET DATA error while sending socket data, bytesSent = %d, WSAGetLastError() = %d\n",__FILE__,__FUNCTION__,bytesReceived,WSAGetLastError());
		//throwException(szBuf);
		printf("%s",szBuf);
	}
	else if(bytesReceived < 0 && WSAGetLastError() == WSAEWOULDBLOCK)
	{
	    printf("In [%s::%s] #1 WSAEWOULDBLOCK during receive, trying again...\n",__FILE__,__FUNCTION__);

	    time_t tStartTimer = time(NULL);
	    while((bytesReceived < 0 && WSAGetLastError() == WSAEWOULDBLOCK) && (difftime(time(NULL),tStartTimer) <= 5))
	    {
	        if(Socket::isReadable() == true)
	        {
                bytesReceived = recv(sock, reinterpret_cast<char*>(data), dataSize, 0);

                printf("In [%s::%s] #2 WSAEWOULDBLOCK during receive, trying again returned: %d\n",__FILE__,__FUNCTION__,bytesReceived);
	        }
	    }
	}

	if(bytesReceived <= 0)
	{
	    int iErr = WSAGetLastError();
	    disconnectSocket();

        char szBuf[1024]="";
        sprintf(szBuf,"[%s::%s] DISCONNECTED SOCKET error while receiving socket data, bytesReceived = %d, WSAGetLastError() = %d\n",__FILE__,__FUNCTION__,bytesReceived,iErr);
        printf("%s",szBuf);
	    //throwException(szBuf);
	}
	return static_cast<int>(bytesReceived);
}

int Socket::peek(void *data, int dataSize){
	int err = 0;
	if(sock > 0)
	{
	    err = recv(sock, reinterpret_cast<char*>(data), dataSize, MSG_PEEK);
	}
	if(err < 0 && WSAGetLastError() != WSAEWOULDBLOCK)
	{
	    char szBuf[1024]="";
        sprintf(szBuf,"[%s::%s] ERROR PEEKING SOCKET DATA error while sending socket data, bytesSent = %d, WSAGetLastError() = %d\n",__FILE__,__FUNCTION__,err,WSAGetLastError());
		//throwException(szBuf);
		printf("%s",szBuf);
	}
	else if(err < 0 && WSAGetLastError() == WSAEWOULDBLOCK)
	{
	    printf("In [%s::%s] #1 WSAEWOULDBLOCK during peek, trying again...\n",__FILE__,__FUNCTION__);

	    time_t tStartTimer = time(NULL);
	    while((err < 0 && WSAGetLastError() == WSAEWOULDBLOCK) && (difftime(time(NULL),tStartTimer) <= 5))
	    {
	        if(Socket::isReadable() == true)
	        {
                err = recv(sock, reinterpret_cast<char*>(data), dataSize, MSG_PEEK);

                printf("In [%s::%s] #2 WSAEWOULDBLOCK during peek, trying again returned: %d\n",__FILE__,__FUNCTION__,err);
	        }
	    }
	}

	if(err <= 0)
	{
	    int iErr = WSAGetLastError();
	    disconnectSocket();

        char szBuf[1024]="";
        sprintf(szBuf,"[%s::%s] DISCONNECTED SOCKET error while peeking socket data, err = %d, WSAGetLastError() = %d\n",__FILE__,__FUNCTION__,err,iErr);
        printf("%s",szBuf);
	    //throwException(szBuf);
	}

	return static_cast<int>(err);
}

void Socket::setBlock(bool block){
	u_long iMode= !
		block;
	int err= ioctlsocket(sock, FIONBIO, &iMode);
	if(err==SOCKET_ERROR)
	{
		throwException("Error setting I/O mode for socket");
	}
}

bool Socket::isReadable()
{
    if(sock <= 0) return false;

	TIMEVAL tv;
	tv.tv_sec= 0;
	tv.tv_usec= 1;

	fd_set set;
	FD_ZERO(&set);
	FD_SET(sock, &set);

	int i= select(sock+1, &set, NULL, NULL, &tv);
	if(i==SOCKET_ERROR)
	{
        char szBuf[1024]="";
        sprintf(szBuf,"[%s::%s] error while selecting socket data, err = %d, errno = %d\n",__FILE__,__FUNCTION__,i,WSAGetLastError());
        printf("%s",szBuf);
    }
	//return (i == 1 && FD_ISSET(sock, &set));
	return (i == 1);
}

bool Socket::isWritable(bool waitOnDelayedResponse)
{
    if(sock <= 0) return false;

	TIMEVAL tv;
	tv.tv_sec= 0;
	tv.tv_usec= 1;

	fd_set set;
	FD_ZERO(&set);
	FD_SET(sock, &set);

    bool result = false;
    do
    {
        int i= select(sock+1, NULL, &set, NULL, &tv);
        if(i==SOCKET_ERROR)
        {
            char szBuf[1024]="";
            sprintf(szBuf,"[%s::%s] error while selecting socket data, err = %d, errno = %d\n",__FILE__,__FUNCTION__,i,WSAGetLastError());
            printf("%s",szBuf);
            waitOnDelayedResponse = false;
        }
        else if(i == 0)
        {
            char szBuf[1024]="";
            sprintf(szBuf,"[%s::%s] TIMEOUT while selecting socket data, err = %d, errno = %d\n",__FILE__,__FUNCTION__,i,WSAGetLastError());
            printf("%s",szBuf);

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

	return result;
}

bool Socket::isConnected(){

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

        if(err <= 0 && WSAGetLastError() != WSAEWOULDBLOCK)
        {
            int iErr = WSAGetLastError();
            disconnectSocket();

            char szBuf[1024]="";
            sprintf(szBuf,"[%s::%s] DISCONNECTED SOCKET error while peeking isconnected socket data, err = %d, WSAGetLastError() = %d\n",__FILE__,__FUNCTION__,err,iErr);
            printf("%s",szBuf);

            return false;
        }
        */
	}

	//otherwise the socket is connected
	return true;
}

string Socket::getHostName() {
	const int strSize= 256;
	char hostname[strSize];
	gethostname(hostname, strSize);
	return hostname;
}

string Socket::getIp() {
	hostent* info= gethostbyname(getHostName().c_str());
	unsigned char* address;

	if(info==NULL)
	{
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

void ClientSocket::connect(const Ip &ip, int port)
{
	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));

    addr.sin_family= AF_INET;
	addr.sin_addr.s_addr= inet_addr(ip.getString().c_str());
	addr.sin_port= htons(port);

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Connecting to host [%s] on port = %d\n", ip.getString().c_str(),port);

	int err= ::connect(sock, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
	if(err < 0)
	{
		char szBuf[1024]="";
	    sprintf(szBuf,"#2 Error connecting socket for IP: %s for Port: %d err = %d WSAGetLastError() = %d",ip.getString().c_str(),port,err,WSAGetLastError());
	    fprintf(stderr, "%s\n", WSAGetLastErrorMessage(szBuf));
		//fprintf(stderr, "%s", szBuf);

        if (WSAGetLastError() == WSAEINPROGRESS || WSAGetLastError() == WSAEWOULDBLOCK)
		{
            fd_set myset;
            struct timeval tv;
            int valopt;
            socklen_t lon;

            fprintf(stderr, "WSAEINPROGRESS or WSAEWOULDBLOCK in connect() - selecting\n");
            do {
               tv.tv_sec = 10;
               tv.tv_usec = 0;

               FD_ZERO(&myset);
               FD_SET(sock, &myset);

               err = select(0, NULL, &myset, NULL, &tv);

               if (err < 0 && WSAGetLastError() != WSAEWOULDBLOCK && WSAGetLastError() != WSAEWOULDBLOCK)
               {
                  sprintf(szBuf, "Error connecting %d\n", WSAGetLastError());
                  //throwException(szBuf);
                  fprintf(stderr, "%s", szBuf);
                  break;
               }
               else if (err > 0) {
                  // Socket selected for write
                  lon = sizeof(int);
                  if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *)(&valopt), &lon) < 0)
                  {
                     sprintf(szBuf, "Error in getsockopt() %d\n", WSAGetLastError());
                     //throwException(szBuf);
                     fprintf(stderr, "%s", szBuf);
                     break;
                  }
                  // Check the value returned...
                  if (valopt)
                  {
                     sprintf(szBuf, "Error in delayed connection() %d\n", valopt);
                     //throwException(szBuf);
                     fprintf(stderr, "%s", szBuf);
                     break;
                  }

                  fprintf(stderr, "Apparent recovery for connection sock = %d, err = %d, WSAGetLastError() = %d\n",sock,err,WSAGetLastError());

                  break;
               }
               else
               {
                  sprintf(szBuf, "Timeout in select() - Cancelling!\n");
                  //throwException(szBuf);
                  fprintf(stderr, "%s", szBuf);

				  disconnectSocket();

                  break;
               }
            } while (1);
        }

        if(err < 0)
        {
            fprintf(stderr, "In [%s::%s] Before END sock = %d, err = %d, errno = %d\n",__FILE__,__FUNCTION__,sock,err,WSAGetLastError());
            //throwException(szBuf);
            disconnectSocket();
        }

        fprintf(stderr, "Valid recovery for connection sock = %d, err = %d, WSAGetLastError() = %d\n",sock,err,WSAGetLastError());
	}
	else
	{
		fprintf(stderr, "Connected to host [%s] on port = %d sock = %d err = %d", ip.getString().c_str(),port,err);
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
