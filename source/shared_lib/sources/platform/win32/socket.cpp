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
#include "platform_util.h"
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

	return ipList;
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
                SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] ERROR SELECTING SOCKET DATA retval = %d WSAGetLastError() = %d",__FILE__,__FUNCTION__,retval,WSAGetLastError());
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
        	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] ERROR PEEKING SOCKET DATA, err = %d WSAGetLastError() = %d\n",__FILE__,__FUNCTION__,err,WSAGetLastError());
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
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] ERROR WRITING SOCKET DATA, err = %d WSAGetLastError() = %d\n",__FILE__,__FUNCTION__,bytesSent,WSAGetLastError());
		//throwException(szBuf);
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

	    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"[%s::%s] DISCONNECTED SOCKET error while sending socket data, bytesSent = %d, WSAGetLastError() = %d\n",__FILE__,__FUNCTION__,bytesSent,iErr);
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
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"[%s::%s] ERROR READING SOCKET DATA error while sending socket data, bytesSent = %d, WSAGetLastError() = %d\n",__FILE__,__FUNCTION__,bytesReceived,WSAGetLastError());
		//throwException(szBuf);
	}
	else if(bytesReceived < 0 && WSAGetLastError() == WSAEWOULDBLOCK)
	{
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] #1 WSAEWOULDBLOCK during receive, trying again...\n",__FILE__,__FUNCTION__);

	    time_t tStartTimer = time(NULL);
	    while((bytesReceived < 0 && WSAGetLastError() == WSAEWOULDBLOCK) && (difftime(time(NULL),tStartTimer) <= 5))
	    {
	        if(Socket::isReadable() == true)
	        {
                bytesReceived = recv(sock, reinterpret_cast<char*>(data), dataSize, 0);

                SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] #2 WSAEWOULDBLOCK during receive, trying again returned: %d\n",__FILE__,__FUNCTION__,bytesReceived);
	        }
	    }
	}

	if(bytesReceived <= 0)
	{
	    int iErr = WSAGetLastError();
	    disconnectSocket();

	    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"[%s::%s] DISCONNECTED SOCKET error while receiving socket data, bytesReceived = %d, WSAGetLastError() = %d\n",__FILE__,__FUNCTION__,bytesReceived,iErr);
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
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"[%s::%s] ERROR PEEKING SOCKET DATA error while sending socket data, bytesSent = %d, WSAGetLastError() = %d\n",__FILE__,__FUNCTION__,err,WSAGetLastError());
		//throwException(szBuf);
	}
	else if(err < 0 && WSAGetLastError() == WSAEWOULDBLOCK)
	{
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] #1 WSAEWOULDBLOCK during peek, trying again...\n",__FILE__,__FUNCTION__);

	    time_t tStartTimer = time(NULL);
	    while((err < 0 && WSAGetLastError() == WSAEWOULDBLOCK) && (difftime(time(NULL),tStartTimer) <= 5))
	    {
	        if(Socket::isReadable() == true)
	        {
                err = recv(sock, reinterpret_cast<char*>(data), dataSize, MSG_PEEK);

                SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] #2 WSAEWOULDBLOCK during peek, trying again returned: %d\n",__FILE__,__FUNCTION__,err);
	        }
	    }
	}

	if(err <= 0)
	{
	    int iErr = WSAGetLastError();
	    disconnectSocket();

	    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"[%s::%s] DISCONNECTED SOCKET error while peeking socket data, err = %d, WSAGetLastError() = %d\n",__FILE__,__FUNCTION__,err,iErr);
	    //throwException(szBuf);
	}

	return static_cast<int>(err);
}

void Socket::setBlock(bool block){
	setBlock(block,this->sock);
}

void Socket::setBlock(bool block, SOCKET socket){
	u_long iMode= !block;
	int err= ioctlsocket(socket, FIONBIO, &iMode);
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
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"[%s::%s] error while selecting socket data, err = %d, errno = %d\n",__FILE__,__FUNCTION__,i,WSAGetLastError());
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
        	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"[%s::%s] error while selecting socket data, err = %d, errno = %d\n",__FILE__,__FUNCTION__,i,WSAGetLastError());
            waitOnDelayedResponse = false;
        }
        else if(i == 0)
        {
        	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"[%s::%s] TIMEOUT while selecting socket data, err = %d, errno = %d\n",__FILE__,__FUNCTION__,i,WSAGetLastError());

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

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Connecting to host [%s] on port = %d\n", ip.getString().c_str(),port);

	int err= ::connect(sock, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
	if(err < 0)
	{
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"#2 Error connecting socket for IP: %s for Port: %d err = %d WSAGetLastError() = %d",ip.getString().c_str(),port,err,WSAGetLastError());

        if (WSAGetLastError() == WSAEINPROGRESS || WSAGetLastError() == WSAEWOULDBLOCK)
		{
            fd_set myset;
            struct timeval tv;
            int valopt;
            socklen_t lon;

            SystemFlags::OutputDebug(SystemFlags::debugNetwork,"WSAEINPROGRESS or WSAEWOULDBLOCK in connect() - selecting\n");
            do {
               tv.tv_sec = 10;
               tv.tv_usec = 0;

               FD_ZERO(&myset);
               FD_SET(sock, &myset);

               err = select(0, NULL, &myset, NULL, &tv);

               if (err < 0 && WSAGetLastError() != WSAEWOULDBLOCK && WSAGetLastError() != WSAEWOULDBLOCK)
               {
            	   SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Error connecting %d\n", WSAGetLastError());
                  //throwException(szBuf);
                  break;
               }
               else if (err > 0) {
                  // Socket selected for write
                  lon = sizeof(int);
                  if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *)(&valopt), &lon) < 0)
                  {
                	  SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Error in getsockopt() %d\n", WSAGetLastError());
                     //throwException(szBuf);
                     break;
                  }
                  // Check the value returned...
                  if (valopt)
                  {
                	  SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Error in delayed connection() %d\n", valopt);
                     //throwException(szBuf);
                     break;
                  }

                  SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Apparent recovery for connection sock = %d, err = %d, WSAGetLastError() = %d\n",sock,err,WSAGetLastError());

                  break;
               }
               else
               {
            	   SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Timeout in select() - Cancelling!\n");
                  //throwException(szBuf);

				  disconnectSocket();

                  break;
               }
            } while (1);
        }

        if(err < 0)
        {
        	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Before END sock = %d, err = %d, errno = %d\n",__FILE__,__FUNCTION__,sock,err,WSAGetLastError());
            //throwException(szBuf);
            disconnectSocket();
        }

        SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Valid recovery for connection sock = %d, err = %d, WSAGetLastError() = %d\n",sock,err,WSAGetLastError());
	}
	else
	{
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Connected to host [%s] on port = %d sock = %d err = %d", ip.getString().c_str(),port,err);
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
		setsockopt(bcfd, SOL_SOCKET, SO_REUSEADDR, (char *)&val, sizeof(val));

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
						string fromIP = inet_ntoa(bcSender.sin_addr);
						SystemFlags::OutputDebug(SystemFlags::debugNetwork,"broadcast message received: [%s] from: [%s]\n", buff,fromIP.c_str() );

						//vector<string> tokens;
						//Tokenize(buff,tokens,":");
						//for(int idx = 1; idx < tokens.size(); idx++) {
						//	foundServers.push_back(tokens[idx]);
						//}
						if(std::find(foundServers.begin(),foundServers.end(),fromIP) == foundServers.end()) {
							foundServers.push_back(fromIP);
						}

						// For now break as soon as we find a server
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

// =====================================================
//	class ServerSocket
// =====================================================
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
	broadCastThread = new BroadCastSocketThread();
	broadCastThread->start();
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
    //char subnetmask[100];       // Subnet mask to broadcast to
    struct in_addr myaddr;      // My host address in net format
    struct hostent* myhostent;
    char * ptr;                 // some transient vars
    int len,i;

    /* get my host name */
    gethostname(myhostname,100);
    myhostent = gethostbyname(myhostname);

    // get only the first host IP address
    std::vector<std::string> ipList = Socket::getLocalIPAddressList();

    /*
    strcpy(subnetmask, ipList[0].c_str());
    ptr = &subnetmask[0];
    len = strlen(ptr);

    // substitute the address with class C subnet mask x.x.x.255
    for(i=len;i>0;i--) {
		if(ptr[i] == '.') {
			strcpy(&ptr[i+1],"255");
			break;
		}
    }
    */

    // Convert the broadcast address from dot notation to a broadcast address.
    //if( (tbcaddr = inet_addr( subnetmask )) == INADDR_NONE )
    //{
    //	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Badly formatted BC address: %d\n", WSAGetLastError());
    	//exit(-1);
    //}
    //else
    {
		port = htons( Socket::getBroadCastPort() );

		// Create the broadcast socket
		memset( &bcLocal, 0, sizeof( struct sockaddr_in));
		bcLocal.sin_family = AF_INET;
		bcLocal.sin_addr.s_addr = htonl( INADDR_BROADCAST );
		bcLocal.sin_port = port;  // We are letting the OS fill in the port number for the local machine.
		bcfd  = socket( AF_INET, SOCK_DGRAM, 0 );

		// If there is an error, report it and  terminate.
		if( bcfd <= 0  ) {
			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Unable to allocate broadcast socket.: %d\n", WSAGetLastError());
			//exit(-1);
		}
		// Mark the socket for broadcast.
		else if( setsockopt( bcfd, SOL_SOCKET, SO_BROADCAST, (const char *) &one, sizeof( int ) ) < 0 ) {
			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Could not set socket to broadcast.: %d\n", WSAGetLastError());
			//exit(-1);
		}
		// Bind the address to the broadcast socket.
		else {

			//int val = 1;
			//setsockopt(bcfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

			//if(::bind(bcfd, (struct sockaddr *) &bcLocal, sizeof(struct sockaddr_in)) < 0) {
			//	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Could not bind address to BC socket.: %d\n", WSAGetLastError());
				//exit(-1);
			//}
			//else
			{
				// Record the broadcast address of the receiver.
				bcaddr.sin_family = AF_INET;
				bcaddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);//tbcaddr;
				bcaddr.sin_port = port;

				setRunningStatus(true);
				SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Broadcast thread is running\n");

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
							elapsed = time(NULL);
							// Broadcast the packet to the subnet
							if( sendto( bcfd, buff, sizeof(buff) + 1, 0 , (struct sockaddr *)&bcaddr, sizeof(struct sockaddr_in) ) != sizeof(buff) + 1 )
							{
								SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Sendto error: %d\n", WSAGetLastError());
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
		}
    }

    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

}}//end namespace
