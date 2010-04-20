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
//#include <sstream>

#if defined(HAVE_SYS_IOCTL_H)
  #define BSD_COMP /* needed for FIONREAD on Solaris2 */
  #include <sys/ioctl.h>
#endif
  #if defined(HAVE_SYS_FILIO_H) /* needed for FIONREAD on Solaris 2.5 */
  #include <sys/filio.h>
#endif


#include "conversion.h"
#include "util.h"
#include "platform_util.h"
#include <algorithm>

#ifdef WIN32

  #include <windows.h>
  #include <winsock.h>
  #include <iphlpapi.h>
  #include <strstream>

#else

  #include <unistd.h>
  #include <stdlib.h>
  #include <sys/socket.h>
  #include <netdb.h>
  #include <netinet/in.h>
  #include <net/if.h>
  //#include <sys/ioctl.h>

#endif

#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "leak_dumper.h"

using namespace std;
using namespace Shared::Util;

namespace Shared{ namespace Platform{

#ifdef WIN32

    #define socklen_t 	int
	#define MAXHOSTNAME 254
	#define PLATFORM_SOCKET_TRY_AGAIN WSAEWOULDBLOCK
    #define PLATFORM_SOCKET_INPROGRESS WSAEINPROGRESS
	#define PLATFORM_SOCKET_INTERRUPTED WSAEWOULDBLOCK
	typedef SSIZE_T ssize_t;

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
									   int nErrorID = 0 )
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

#else

	typedef unsigned int UINT_PTR, *PUINT_PTR;
	typedef UINT_PTR        SOCKET;
	#define INVALID_SOCKET  (SOCKET)(~0)

	#define PLATFORM_SOCKET_TRY_AGAIN EAGAIN
	#define PLATFORM_SOCKET_INPROGRESS EINPROGRESS
	#define PLATFORM_SOCKET_INTERRUPTED EINTR

#endif

int Socket::broadcast_portno = 61357;
BroadCastClientSocketThread *ClientSocket::broadCastClientThread = NULL;

int getLastSocketError() {
#ifndef WIN32
	return errno;
#else
	return WSAGetLastError();
#endif
}

const char * getLastSocketErrorText(int *errNumber=NULL) {
	int errId = (errNumber != NULL ? *errNumber : getLastSocketError());
#ifndef WIN32
	return strerror(errId);
#else
	return WSAGetLastErrorMessage("",errId);
#endif
}

string getLastSocketErrorFormattedText(int *errNumber=NULL) {
	int errId = (errNumber != NULL ? *errNumber : getLastSocketError());
	string msg = "(Error: " + intToStr(errId) + " - [" + string(getLastSocketErrorText(&errId)) +"])";
	return msg;
}

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

#if defined(__FreeBSD__) || defined(BSD) || defined(__APPLE__) || defined(__linux__)
# define USE_GETIFADDRS 1
# include <ifaddrs.h>
static uint32 SockAddrToUint32(struct sockaddr * a)
{
   return ((a)&&(a->sa_family == AF_INET)) ? ntohl(((struct sockaddr_in *)a)->sin_addr.s_addr) : 0;
}
#endif

// convert a numeric IP address into its string representation
static void Inet_NtoA(uint32 addr, char * ipbuf)
{
   sprintf(ipbuf, "%d.%d.%d.%d", (addr>>24)&0xFF, (addr>>16)&0xFF, (addr>>8)&0xFF, (addr>>0)&0xFF);
}

// convert a string represenation of an IP address into its numeric equivalent
static uint32 Inet_AtoN(const char * buf)
{
   // net_server inexplicably doesn't have this function; so I'll just fake it
   uint32 ret = 0;
   int shift = 24;  // fill out the MSB first
   bool startQuad = true;
   while((shift >= 0)&&(*buf))
   {
      if (startQuad)
      {
         unsigned char quad = (unsigned char) atoi(buf);
         ret |= (((uint32)quad) << shift);
         shift -= 8;
      }
      startQuad = (*buf == '.');
      buf++;
   }
   return ret;
}

/*
static void PrintNetworkInterfaceInfos()
{
#if defined(USE_GETIFADDRS)
   // BSD-style implementation
   struct ifaddrs * ifap;
   if (getifaddrs(&ifap) == 0)
   {
      struct ifaddrs * p = ifap;
      while(p)
      {
         uint32 ifaAddr  = SockAddrToUint32(p->ifa_addr);
         uint32 maskAddr = SockAddrToUint32(p->ifa_netmask);
         uint32 dstAddr  = SockAddrToUint32(p->ifa_dstaddr);
         if (ifaAddr > 0)
         {
            char ifaAddrStr[32];  Inet_NtoA(ifaAddr,  ifaAddrStr);
            char maskAddrStr[32]; Inet_NtoA(maskAddr, maskAddrStr);
            char dstAddrStr[32];  Inet_NtoA(dstAddr,  dstAddrStr);
            printf("  Found interface:  name=[%s] desc=[%s] address=[%s] netmask=[%s] broadcastAddr=[%s]\n", p->ifa_name, "unavailable", ifaAddrStr, maskAddrStr, dstAddrStr);
         }
         p = p->ifa_next;
      }
      freeifaddrs(ifap);
   }
#elif defined(WIN32)
   // Windows XP style implementation

   // Adapted from example code at http://msdn2.microsoft.com/en-us/library/aa365917.aspx
   // Now get Windows' IPv4 addresses table.  Once again, we gotta call GetIpAddrTable()
   // multiple times in order to deal with potential race conditions properly.
   MIB_IPADDRTABLE * ipTable = NULL;
   {
      ULONG bufLen = 0;
      for (int i=0; i<5; i++)
      {
         DWORD ipRet = GetIpAddrTable(ipTable, &bufLen, false);
         if (ipRet == ERROR_INSUFFICIENT_BUFFER)
         {
            free(ipTable);  // in case we had previously allocated it
            ipTable = (MIB_IPADDRTABLE *) malloc(bufLen);
         }
         else if (ipRet == NO_ERROR) break;
         else
         {
            free(ipTable);
            ipTable = NULL;
            break;
         }
     }
   }

   if (ipTable)
   {
      // Try to get the Adapters-info table, so we can given useful names to the IP
      // addresses we are returning.  Gotta call GetAdaptersInfo() up to 5 times to handle
      // the potential race condition between the size-query call and the get-data call.
      // I love a well-designed API :^P
      IP_ADAPTER_INFO * pAdapterInfo = NULL;
      {
         ULONG bufLen = 0;
         for (int i=0; i<5; i++)
         {
            DWORD apRet = GetAdaptersInfo(pAdapterInfo, &bufLen);
            if (apRet == ERROR_BUFFER_OVERFLOW)
            {
               free(pAdapterInfo);  // in case we had previously allocated it
               pAdapterInfo = (IP_ADAPTER_INFO *) malloc(bufLen);
            }
            else if (apRet == ERROR_SUCCESS) break;
            else
            {
               free(pAdapterInfo);
               pAdapterInfo = NULL;
               break;
            }
         }
      }

      for (DWORD i=0; i<ipTable->dwNumEntries; i++)
      {
         const MIB_IPADDRROW & row = ipTable->table[i];

         // Now lookup the appropriate adaptor-name in the pAdaptorInfos, if we can find it
         const char * name = NULL;
         const char * desc = NULL;
         if (pAdapterInfo)
         {
            IP_ADAPTER_INFO * next = pAdapterInfo;
            while((next)&&(name==NULL))
            {
               IP_ADDR_STRING * ipAddr = &next->IpAddressList;
               while(ipAddr)
               {
                  if (Inet_AtoN(ipAddr->IpAddress.String) == ntohl(row.dwAddr))
                  {
                     name = next->AdapterName;
                     desc = next->Description;
                     break;
                  }
                  ipAddr = ipAddr->Next;
               }
               next = next->Next;
            }
         }
         char buf[128];
         if (name == NULL)
         {
            sprintf(buf, "unnamed-%i", i);
            name = buf;
         }

         uint32 ipAddr  = ntohl(row.dwAddr);
         uint32 netmask = ntohl(row.dwMask);
         uint32 baddr   = ipAddr & netmask;
         if (row.dwBCastAddr) baddr |= ~netmask;

         char ifaAddrStr[32];  Inet_NtoA(ipAddr,  ifaAddrStr);
         char maskAddrStr[32]; Inet_NtoA(netmask, maskAddrStr);
         char dstAddrStr[32];  Inet_NtoA(baddr,   dstAddrStr);
         printf("  Found interface:  name=[%s] desc=[%s] address=[%s] netmask=[%s] broadcastAddr=[%s]\n", name, desc?desc:"unavailable", ifaAddrStr, maskAddrStr, dstAddrStr);
      }

      free(pAdapterInfo);
      free(ipTable);
   }
#else
   // Dunno what we're running on here!
#  error "Don't know how to implement PrintNetworkInterfaceInfos() on this OS!"
#endif
}
*/

string getNetworkInterfaceBroadcastAddress(string ipAddress)
{
	string broadCastAddress = "";

#if defined(USE_GETIFADDRS)
   // BSD-style implementation
   struct ifaddrs * ifap;
   if (getifaddrs(&ifap) == 0)
   {
      struct ifaddrs * p = ifap;
      while(p)
      {
         uint32 ifaAddr  = SockAddrToUint32(p->ifa_addr);
         uint32 maskAddr = SockAddrToUint32(p->ifa_netmask);
         uint32 dstAddr  = SockAddrToUint32(p->ifa_dstaddr);
         if (ifaAddr > 0)
         {
            char ifaAddrStr[32];  Inet_NtoA(ifaAddr,  ifaAddrStr);
            char maskAddrStr[32]; Inet_NtoA(maskAddr, maskAddrStr);
            char dstAddrStr[32];  Inet_NtoA(dstAddr,  dstAddrStr);
            //printf("  Found interface:  name=[%s] desc=[%s] address=[%s] netmask=[%s] broadcastAddr=[%s]\n", p->ifa_name, "unavailable", ifaAddrStr, maskAddrStr, dstAddrStr);
			 if(strcmp(ifaAddrStr,ipAddress.c_str()) == 0) {
				broadCastAddress = dstAddrStr;
			 }
         }
         p = p->ifa_next;
      }
      freeifaddrs(ifap);
   }
#elif defined(WIN32)
   // Windows XP style implementation

   // Adapted from example code at http://msdn2.microsoft.com/en-us/library/aa365917.aspx
   // Now get Windows' IPv4 addresses table.  Once again, we gotta call GetIpAddrTable()
   // multiple times in order to deal with potential race conditions properly.
   MIB_IPADDRTABLE * ipTable = NULL;
   {
      ULONG bufLen = 0;
      for (int i=0; i<5; i++)
      {
         DWORD ipRet = GetIpAddrTable(ipTable, &bufLen, false);
         if (ipRet == ERROR_INSUFFICIENT_BUFFER)
         {
            free(ipTable);  // in case we had previously allocated it
            ipTable = (MIB_IPADDRTABLE *) malloc(bufLen);
         }
         else if (ipRet == NO_ERROR) break;
         else
         {
            free(ipTable);
            ipTable = NULL;
            break;
         }
     }
   }

   if (ipTable)
   {
      // Try to get the Adapters-info table, so we can given useful names to the IP
      // addresses we are returning.  Gotta call GetAdaptersInfo() up to 5 times to handle
      // the potential race condition between the size-query call and the get-data call.
      // I love a well-designed API :^P
      IP_ADAPTER_INFO * pAdapterInfo = NULL;
      {
         ULONG bufLen = 0;
         for (int i=0; i<5; i++)
         {
            DWORD apRet = GetAdaptersInfo(pAdapterInfo, &bufLen);
            if (apRet == ERROR_BUFFER_OVERFLOW)
            {
               free(pAdapterInfo);  // in case we had previously allocated it
               pAdapterInfo = (IP_ADAPTER_INFO *) malloc(bufLen);
            }
            else if (apRet == ERROR_SUCCESS) break;
            else
            {
               free(pAdapterInfo);
               pAdapterInfo = NULL;
               break;
            }
         }
      }

      for (DWORD i=0; i<ipTable->dwNumEntries; i++)
      {
         const MIB_IPADDRROW & row = ipTable->table[i];

         // Now lookup the appropriate adaptor-name in the pAdaptorInfos, if we can find it
         const char * name = NULL;
         const char * desc = NULL;
         if (pAdapterInfo)
         {
            IP_ADAPTER_INFO * next = pAdapterInfo;
            while((next)&&(name==NULL))
            {
               IP_ADDR_STRING * ipAddr = &next->IpAddressList;
               while(ipAddr)
               {
                  if (Inet_AtoN(ipAddr->IpAddress.String) == ntohl(row.dwAddr))
                  {
                     name = next->AdapterName;
                     desc = next->Description;
                     break;
                  }
                  ipAddr = ipAddr->Next;
               }
               next = next->Next;
            }
         }
         char buf[128];
         if (name == NULL)
         {
            //sprintf(buf, "unnamed-%i", i);
            name = buf;
         }

         uint32 ipAddr  = ntohl(row.dwAddr);
         uint32 netmask = ntohl(row.dwMask);
         uint32 baddr   = ipAddr & netmask;
         if (row.dwBCastAddr) baddr |= ~netmask;

         char ifaAddrStr[32];  Inet_NtoA(ipAddr,  ifaAddrStr);
         char maskAddrStr[32]; Inet_NtoA(netmask, maskAddrStr);
         char dstAddrStr[32];  Inet_NtoA(baddr,   dstAddrStr);
         //printf("  Found interface:  name=[%s] desc=[%s] address=[%s] netmask=[%s] broadcastAddr=[%s]\n", name, desc?desc:"unavailable", ifaAddrStr, maskAddrStr, dstAddrStr);
		 if(strcmp(ifaAddrStr,ipAddress.c_str()) == 0) {
			broadCastAddress = dstAddrStr;
		 }
      }

      free(pAdapterInfo);
      free(ipTable);
   }
#else
   // Dunno what we're running on here!
#  error "Don't know how to implement PrintNetworkInterfaceInfos() on this OS!"
#endif

	return broadCastAddress;
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

#ifndef WIN32

	// Now check all linux network devices
	for(int idx = 0; idx < 5; ++idx) {
		PLATFORM_SOCKET fd = socket(AF_INET, SOCK_DGRAM, 0);

		/* I want to get an IPv4 IP address */
		struct ifreq ifr;
		ifr.ifr_addr.sa_family = AF_INET;

		/* I want IP address attached to "eth0" */
		char szBuf[10]="";
		sprintf(szBuf,"eth%d",idx);
		strncpy(ifr.ifr_name, szBuf, IFNAMSIZ-1);
		ioctl(fd, SIOCGIFADDR, &ifr);
		close(fd);

		sprintf(myhostaddr, "%s",inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] myhostaddr = [%s]\n",__FILE__,__FUNCTION__,__LINE__,myhostaddr);

		if(strlen(myhostaddr) > 0 && strncmp(myhostaddr,"127.",4) != 0) {
			if(std::find(ipList.begin(),ipList.end(),myhostaddr) == ipList.end()) {
				ipList.push_back(myhostaddr);
			}
		}
	}

#endif

	return ipList;
}

bool Socket::isSocketValid() const {
	return Socket::isSocketValid(&sock);
}

bool Socket::isSocketValid(const PLATFORM_SOCKET *validateSocket) {
#ifdef WIN32
	if(validateSocket == NULL) {
		return false;
	}
	else {
		return (*validateSocket != INVALID_SOCKET);
	}
#else
	if(validateSocket == NULL) {
		return false;
	}
	else {
		return (*validateSocket > 0);
	}
#endif
}

Socket::Socket(PLATFORM_SOCKET sock){
	this->sock= sock;
}

Socket::Socket()
{
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(isSocketValid() == false)
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

    if(isSocketValid() == true)
    {
        SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] calling shutdown and close for socket = %d...\n",__FILE__,__FUNCTION__,sock);
        ::shutdown(sock,2);
#ifndef WIN32
        ::close(sock);
        sock = INVALID_SOCKET;
#else
        ::closesocket(sock);
        sock = -1;
#endif
    }

    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] END closing socket = %d...\n",__FILE__,__FUNCTION__,sock);
}

// Int lookup is socket fd while bool result is whether or not that socket was signalled for reading
bool Socket::hasDataToRead(std::map<PLATFORM_SOCKET,bool> &socketTriggeredList)
{
    bool bResult = false;

    if(socketTriggeredList.size() > 0)
    {
        /* Watch stdin (fd 0) to see when it has input. */
        fd_set rfds;
        FD_ZERO(&rfds);

        PLATFORM_SOCKET imaxsocket = 0;
        for(std::map<PLATFORM_SOCKET,bool>::iterator itermap = socketTriggeredList.begin();
            itermap != socketTriggeredList.end(); itermap++)
        {
        	PLATFORM_SOCKET socket = itermap->first;
            if(Socket::isSocketValid(&socket) == true)
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
                SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] ERROR SELECTING SOCKET DATA retval = %d error = %s\n",__FILE__,__FUNCTION__,retval,getLastSocketErrorFormattedText().c_str());
            }
            else if(retval)
            {
                bResult = true;

                SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] select detected data imaxsocket = %d...\n",__FILE__,__FUNCTION__,imaxsocket);

                for(std::map<PLATFORM_SOCKET,bool>::iterator itermap = socketTriggeredList.begin();
                    itermap != socketTriggeredList.end(); itermap++)
                {
                	PLATFORM_SOCKET socket = itermap->first;
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

bool Socket::hasDataToRead(PLATFORM_SOCKET socket)
{
    bool bResult = false;

    if(Socket::isSocketValid(&socket) == true)
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
    if(isSocketValid() == true)
    {
    	int loopCount = 1;
    	for(time_t elapsed = time(NULL); difftime(time(NULL),elapsed) < 1;) {
			/* ioctl isn't posix, but the following seems to work on all modern
			 * unixes */
	#ifndef WIN32
			int err = ioctl(sock, FIONREAD, &size);
	#else
			int err= ioctlsocket(sock, FIONREAD, &size);
	#endif
			if(err < 0 && getLastSocketError() != PLATFORM_SOCKET_TRY_AGAIN)
			{
				SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] ERROR PEEKING SOCKET DATA, err = %d %s\n",__FILE__,__FUNCTION__,err,getLastSocketErrorFormattedText().c_str());
				break;
				//throwException(szBuf);
			}
			else if(err == 0)
			{
				//if(Socket::enableNetworkDebugInfo) printf("In [%s] ioctl returned = %d, size = %ld\n",__FUNCTION__,err,size);
			}

			if(size > 0) {
				break;
			}
			else if(hasDataToRead() == true) {
				SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] WARNING PEEKING SOCKET DATA, (hasDataToRead() == true) err = %d, sock = %d, size = %u, loopCount = %d\n",__FILE__,__FUNCTION__,__LINE__,err,sock,size,loopCount);
			}
			else {
				SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] WARNING PEEKING SOCKET DATA, err = %d, sock = %d, size = %u, loopCount = %d\n",__FILE__,__FUNCTION__,__LINE__,err,sock,size,loopCount);
				break;
			}

			loopCount++;
    	}
    }

	return static_cast<int>(size);
}

int Socket::send(const void *data, int dataSize) {
	ssize_t bytesSent= 0;
	if(isSocketValid() == true)
	{
        bytesSent = ::send(sock, reinterpret_cast<const char*>(data), dataSize, 0);
	}
	if(bytesSent < 0 && getLastSocketError() != PLATFORM_SOCKET_TRY_AGAIN)
	{
        SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] ERROR WRITING SOCKET DATA, err = %d error = %s\n",__FILE__,__FUNCTION__,bytesSent,getLastSocketErrorFormattedText().c_str());
		//throwException(szBuf);
	}
	else if(bytesSent < 0 && getLastSocketError() == PLATFORM_SOCKET_TRY_AGAIN)
	{
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] #1 EAGAIN during send, trying again...\n",__FILE__,__FUNCTION__);

	    time_t tStartTimer = time(NULL);
	    while((bytesSent < 0 && getLastSocketError() == PLATFORM_SOCKET_TRY_AGAIN) && (difftime(time(NULL),tStartTimer) <= 5))
	    {
	        if(Socket::isWritable(true) == true)
	        {
                bytesSent = ::send(sock, reinterpret_cast<const char*>(data), dataSize, 0);

                SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] #2 EAGAIN during send, trying again returned: %d\n",__FILE__,__FUNCTION__,bytesSent);
	        }
	    }
	}
	if(bytesSent <= 0)
	{
	    int iErr = getLastSocketError();
	    disconnectSocket();

        SystemFlags::OutputDebug(SystemFlags::debugNetwork,"[%s::%s] DISCONNECTED SOCKET error while sending socket data, bytesSent = %d, error = %s\n",__FILE__,__FUNCTION__,bytesSent,getLastSocketErrorFormattedText(&iErr).c_str());
	    //throwException(szBuf);
	}

    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] sock = %d, bytesSent = %d\n",__FILE__,__FUNCTION__,sock,bytesSent);

	return static_cast<int>(bytesSent);
}

int Socket::receive(void *data, int dataSize)
{
	ssize_t bytesReceived = 0;

	if(isSocketValid() == true)
	{
	    bytesReceived = recv(sock, reinterpret_cast<char*>(data), dataSize, 0);
	}
	if(bytesReceived < 0 && getLastSocketError() != PLATFORM_SOCKET_TRY_AGAIN)
	{
        SystemFlags::OutputDebug(SystemFlags::debugNetwork,"[%s::%s] ERROR READING SOCKET DATA error while sending socket data, bytesSent = %d, error = %s\n",__FILE__,__FUNCTION__,bytesReceived,getLastSocketErrorFormattedText().c_str());
		//throwException(szBuf);
	}
	else if(bytesReceived < 0 && getLastSocketError() == PLATFORM_SOCKET_TRY_AGAIN)
	{
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] #1 EAGAIN during receive, trying again...\n",__FILE__,__FUNCTION__);

	    time_t tStartTimer = time(NULL);
	    while((bytesReceived < 0 && getLastSocketError() == PLATFORM_SOCKET_TRY_AGAIN) && (difftime(time(NULL),tStartTimer) <= 5))
	    {
	        if(Socket::isReadable() == true)
	        {
                bytesReceived = recv(sock, reinterpret_cast<char*>(data), dataSize, 0);

                SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] #2 EAGAIN during receive, trying again returned: %d\n",__FILE__,__FUNCTION__,bytesReceived);
	        }
	    }
	}

	if(bytesReceived <= 0)
	{
	    int iErr = getLastSocketError();
	    disconnectSocket();

        SystemFlags::OutputDebug(SystemFlags::debugNetwork,"[%s::%s] DISCONNECTED SOCKET error while receiving socket data, bytesReceived = %d, error = %s\n",__FILE__,__FUNCTION__,bytesReceived,getLastSocketErrorFormattedText(&iErr).c_str());
	    //throwException(szBuf);
	}
	return static_cast<int>(bytesReceived);
}

int Socket::peek(void *data, int dataSize){
	ssize_t err = 0;
	if(isSocketValid() == true)
	{
	    err = recv(sock, reinterpret_cast<char*>(data), dataSize, MSG_PEEK);
	}
	if(err < 0 && getLastSocketError() != PLATFORM_SOCKET_TRY_AGAIN)
	{
	    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"[%s::%s] ERROR PEEKING SOCKET DATA error while sending socket data, bytesSent = %d, error = %s\n",__FILE__,__FUNCTION__,err,getLastSocketErrorFormattedText().c_str());
		//throwException(szBuf);

		disconnectSocket();
	}
	else if(err < 0 && getLastSocketError() == PLATFORM_SOCKET_TRY_AGAIN)
	{
		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] #1 EAGAIN during peek, trying again...\n",__FILE__,__FUNCTION__);

	    time_t tStartTimer = time(NULL);
	    while((err < 0 && getLastSocketError() == PLATFORM_SOCKET_TRY_AGAIN) && (difftime(time(NULL),tStartTimer) <= 5))
	    {
	        if(Socket::isReadable() == true)
	        {
                err = recv(sock, reinterpret_cast<char*>(data), dataSize, MSG_PEEK);

                SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] #2 EAGAIN during peek, trying again returned: %d\n",__FILE__,__FUNCTION__,err);
	        }
	    }
	}

	if(err <= 0)
	{
	    int iErr = getLastSocketError();
	    disconnectSocket();

        SystemFlags::OutputDebug(SystemFlags::debugNetwork,"[%s::%s] DISCONNECTED SOCKET error while peeking socket data, err = %d, error = %s\n",__FILE__,__FUNCTION__,err,getLastSocketErrorFormattedText(&iErr).c_str());
	    //throwException(szBuf);
	}

	return static_cast<int>(err);
}

void Socket::setBlock(bool block){
	setBlock(block,this->sock);
}

void Socket::setBlock(bool block, PLATFORM_SOCKET socket){
#ifndef WIN32
	int err= fcntl(socket, F_SETFL, block ? 0 : O_NONBLOCK);
#else
	u_long iMode= !block;
	int err= ioctlsocket(socket, FIONBIO, &iMode);
#endif
	if(err < 0){
		throwException("Error setting I/O mode for socket");
	}
}

bool Socket::isReadable()
{
    if(isSocketValid() == false) return false;
#ifndef WIN32
	struct timeval tv;
#else
	TIMEVAL tv;
#endif
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
            SystemFlags::OutputDebug(SystemFlags::debugNetwork,"[%s::%s] error while selecting socket data, err = %d, error = %s\n",__FILE__,__FUNCTION__,i,getLastSocketErrorFormattedText().c_str());
        }
	}
	//return (i == 1 && FD_ISSET(sock, &set));
	return (i == 1);
}

bool Socket::isWritable(bool waitOnDelayedResponse)
{
    if(isSocketValid() == false) return false;

#ifndef WIN32
	struct timeval tv;
#else
	TIMEVAL tv;
#endif
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

                SystemFlags::OutputDebug(SystemFlags::debugNetwork,"[%s::%s] error while selecting socket data, err = %d, error = %s\n",__FILE__,__FUNCTION__,i,getLastSocketErrorFormattedText().c_str());
            }
            waitOnDelayedResponse = false;

            //throwException("Error selecting socket");
        }
        else if(i == 0)
        {
            if(difftime(time(NULL),lastDebugEvent) >= 1)
            {
                lastDebugEvent = time(NULL);
                SystemFlags::OutputDebug(SystemFlags::debugNetwork,"[%s::%s] TIMEOUT while selecting socket data, err = %d, error = %s\n",__FILE__,__FUNCTION__,i,getLastSocketErrorFormattedText().c_str());
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

void Socket::throwException(string str){
	string msg = str + " " + getLastSocketErrorFormattedText();
	throw runtime_error(msg);
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

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Connecting to host [%s] on port = %d\n", ip.getString().c_str(),port);

	int err= ::connect(sock, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
	if(err < 0)
	{
	    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] #2 Error connecting socket for IP: %s for Port: %d err = %d error = %s\n",__FILE__,__FUNCTION__,ip.getString().c_str(),port,err,getLastSocketErrorFormattedText().c_str());

        if (getLastSocketError() == PLATFORM_SOCKET_INPROGRESS ||
        	getLastSocketError() == PLATFORM_SOCKET_TRY_AGAIN)
        {
            fd_set myset;
            struct timeval tv;
            int valopt;
            socklen_t lon;

            SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] PLATFORM_SOCKET_INPROGRESS in connect() - selecting\n",__FILE__,__FUNCTION__);

            do
            {
               tv.tv_sec = 10;
               tv.tv_usec = 0;

               FD_ZERO(&myset);
               FD_SET(sock, &myset);

               err = select(sock+1, NULL, &myset, NULL, &tv);

               if (err < 0 && getLastSocketError() != PLATFORM_SOCKET_INTERRUPTED)
               {
            	   SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Error connecting %s\n",__FILE__,__FUNCTION__,getLastSocketErrorFormattedText().c_str());
                  //throwException(szBuf);
                  break;
               }
               else if (err > 0)
               {
                  // Socket selected for write
                  lon = sizeof(int);
#ifndef WIN32
                  if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0)
#else
                  if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *)(&valopt), &lon) < 0)
#endif
                  {
                	  SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Error in getsockopt() %s\n",__FILE__,__FUNCTION__,getLastSocketErrorFormattedText().c_str());
                     //throwException(szBuf);
                     break;
                  }
                  // Check the value returned...
                  if (valopt)
                  {
                	  SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Error in delayed connection() %d - [%s]\n",__FILE__,__FUNCTION__,valopt, strerror(valopt));
                     //throwException(szBuf);
                     break;
                  }

                  errno = 0;
                  SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Apparent recovery for connection sock = %d, err = %d\n",__FILE__,__FUNCTION__,sock,err);

                  break;
               }
               else
               {
            	   SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Timeout in select() - Cancelling!\n",__FILE__,__FUNCTION__);
                  //throwException(szBuf);

                  disconnectSocket();
                  break;
               }
            } while (1);
        }

        if(err < 0)
        {
        	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Before END sock = %d, err = %d, error = %s\n",__FILE__,__FUNCTION__,sock,err,getLastSocketErrorFormattedText().c_str());
            //throwException(szBuf);
            disconnectSocket();
        }
        else
        {
        	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Valid recovery for connection sock = %d, err = %d, error = %s\n",__FILE__,__FUNCTION__,sock,err,getLastSocketErrorFormattedText().c_str());
        }
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
    PLATFORM_SOCKET bcfd;                // The file descriptor used for the broadcast.
    bool one = true;            // Parameter for "setscokopt".
    char buff[10024];            // Buffers the data to be broadcasted.
    socklen_t alen;
    int nb;                     // The number of bytes read.

    port = htons( Socket::getBroadCastPort() );

    // Prepare to receive the broadcast.
    bcfd = socket(AF_INET, SOCK_DGRAM, 0);
    if( bcfd <= 0 )	{
    	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"socket failed: %s\n", getLastSocketErrorFormattedText().c_str());
		//exit(-1);
	}
    else {
		// Create the address we are receiving on.
		memset( (char*)&bcaddr, 0, sizeof(bcaddr));
		bcaddr.sin_family = AF_INET;
		bcaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		bcaddr.sin_port = port;

		int val = 1;
#ifndef WIN32
		setsockopt(bcfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
#else
		setsockopt(bcfd, SOL_SOCKET, SO_REUSEADDR, (char *)&val, sizeof(val));
#endif
		if(bind( bcfd,  (struct sockaddr *)&bcaddr, sizeof(bcaddr) ) < 0 )	{
			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"bind failed: %s\n", getLastSocketErrorFormattedText().c_str());
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
						SystemFlags::OutputDebug(SystemFlags::debugNetwork,"recvfrom failed: %s\n", getLastSocketErrorFormattedText().c_str());
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

#ifndef WIN32
        ::close(bcfd);
        bcfd = INVALID_SOCKET;
#else
        ::closesocket(bcfd);
        bcfd = -1;
#endif

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
			//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
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

bool ServerSocket::isBroadCastThreadRunning() {
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	bool isThreadRunning = (broadCastThread != NULL && broadCastThread->getRunningStatus() == true);

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] isThreadRunning = %d\n",__FILE__,__FUNCTION__,__LINE__,isThreadRunning);

	return isThreadRunning;
}

void ServerSocket::bind(int port)
{
	//sockaddr structure
	sockaddr_in addr;
	addr.sin_family= AF_INET;
	addr.sin_addr.s_addr= INADDR_ANY;
	addr.sin_port= htons(port);

	int val = 1;

#ifndef WIN32
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
#else
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&val, sizeof(val));
#endif

	int err= ::bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
	if(err < 0)
	{
	    char szBuf[1024]="";
	    sprintf(szBuf, "In [%s::%s] Error binding socket sock = %d, err = %d, error = %s\n",__FILE__,__FUNCTION__,sock,err,getLastSocketErrorFormattedText().c_str());
		throwException(szBuf);
	}
}

void ServerSocket::listen(int connectionQueueSize)
{
	int err= ::listen(sock, connectionQueueSize);
	if(err < 0)
	{
	    char szBuf[1024]="";
	    sprintf(szBuf, "In [%s::%s] Error listening socket sock = %d, err = %d, error = %s\n",__FILE__,__FUNCTION__,sock,err,getLastSocketErrorFormattedText().c_str());
		throwException(szBuf);
	}

	if(connectionQueueSize > 0) {
		if(isBroadCastThreadRunning() == false) {
			startBroadCastThread();
		}
	}
	else {
		stopBroadCastThread();
	}

}

Socket *ServerSocket::accept()
{
	PLATFORM_SOCKET newSock= ::accept(sock, NULL, NULL);
	if(isSocketValid(&newSock) == false)
	{
	    char szBuf[1024]="";
	    sprintf(szBuf, "In [%s::%s] Error accepting socket connection sock = %d, err = %d, error = %s\n",__FILE__,__FUNCTION__,sock,newSock,getLastSocketErrorFormattedText().c_str());

		if(getLastSocketError() == PLATFORM_SOCKET_TRY_AGAIN)
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

	const int MAX_NIC_COUNT = 10;
    short port;                 // The port for the broadcast.
    struct sockaddr_in bcLocal[MAX_NIC_COUNT]; // local socket address for the broadcast.
    PLATFORM_SOCKET bcfd[MAX_NIC_COUNT];                // The socket used for the broadcast.
    bool one = true;            // Parameter for "setscokopt".
    int pn;                     // The number of the packet broadcasted.
    char buff[1024];            // Buffers the data to be broadcasted.
    char myhostname[100];       // hostname of local machine
    char subnetmask[MAX_NIC_COUNT][100];       // Subnet mask to broadcast to
    struct hostent* myhostent;
    //char * ptr;                 // some transient vars
    //int len,i;

    /* get my host name */
    gethostname(myhostname,100);
    myhostent = gethostbyname(myhostname);

    // get all host IP addresses
    std::vector<std::string> ipList = Socket::getLocalIPAddressList();

	for(unsigned int idx = 0; idx < ipList.size() && idx < MAX_NIC_COUNT; idx++) {
		string broadCastAddress = getNetworkInterfaceBroadcastAddress(ipList[idx]);
		strcpy(subnetmask[idx], broadCastAddress.c_str());
	}

	port = htons( Socket::getBroadCastPort() );

	for(unsigned int idx = 0; idx < ipList.size() && idx < MAX_NIC_COUNT; idx++) {
		// Create the broadcast socket
		memset( &bcLocal[idx], 0, sizeof( struct sockaddr_in));
		bcLocal[idx].sin_family			= AF_INET;
		bcLocal[idx].sin_addr.s_addr	= inet_addr(subnetmask[idx]); //htonl( INADDR_BROADCAST );
		bcLocal[idx].sin_port			= port;  // We are letting the OS fill in the port number for the local machine.
		bcfd[idx]						= socket( AF_INET, SOCK_DGRAM, 0 );
		if( bcfd[idx] <= 0  ) {
			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Unable to allocate broadcast socket [%s]: %s\n", subnetmask[idx], getLastSocketErrorFormattedText().c_str());
			//exit(-1);
		}
		// Mark the socket for broadcast.
		else if( setsockopt( bcfd[idx], SOL_SOCKET, SO_BROADCAST, (const char *) &one, sizeof( int ) ) < 0 ) {
			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Could not set socket to broadcast [%s]: %s\n", subnetmask[idx], getLastSocketErrorFormattedText().c_str());
			//exit(-1);
		}

		SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] setting up broadcast on address [%s]\n",__FILE__,__FUNCTION__,__LINE__,subnetmask[idx]);
	}

	setRunningStatus(true);
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Broadcast thread is running\n");

	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	time_t elapsed = 0;
	for( pn = 1; ; pn++ )
	{
		for(unsigned int idx = 0; idx < ipList.size() && idx < MAX_NIC_COUNT; idx++) {
			if( Socket::isSocketValid(&bcfd[idx]) == true ) {
				try {
					// Send this machine's host name and address in hostname:n.n.n.n format
					sprintf(buff,"%s",myhostname);
					for(unsigned int idx1 = 0; idx1 < ipList.size(); idx1++) {
						sprintf(buff,"%s:%s",buff,ipList[idx1].c_str());
					}

					if(difftime(time(NULL),elapsed) >= 1) {
						elapsed = time(NULL);
						// Broadcast the packet to the subnet
						//if( sendto( bcfd, buff, sizeof(buff) + 1, 0 , (struct sockaddr *)&bcaddr, sizeof(struct sockaddr_in) ) != sizeof(buff) + 1 )
						if( sendto( bcfd[idx], buff, sizeof(buff) + 1, 0 , (struct sockaddr *)&bcLocal[idx], sizeof(struct sockaddr_in) ) != sizeof(buff) + 1 )
						{
							SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Sendto error: %s\n", getLastSocketErrorFormattedText().c_str());
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

					//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
				}
				catch(const exception &ex) {
					SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
					setRunningStatus(false);
				}
				catch(...) {
					SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] unknown error\n",__FILE__,__FUNCTION__,__LINE__);
					setRunningStatus(false);
				}
			}

			if(getQuitStatus() == true) {
				SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
				break;
			}
		}
		if(getQuitStatus() == true) {
			SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			break;
		}
	}

	for(unsigned int idx = 0; idx < ipList.size() && idx < MAX_NIC_COUNT; idx++) {
		if( Socket::isSocketValid(&bcfd[idx]) == true ) {
#ifndef WIN32
        ::close(bcfd[idx]);
        bcfd[idx] = INVALID_SOCKET;
#else
        ::closesocket(bcfd[idx]);
        bcfd[idx] = -1;
#endif
		}
	}

	setRunningStatus(false);
	SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Broadcast thread is exiting\n");

    SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}


}}//end namespace

