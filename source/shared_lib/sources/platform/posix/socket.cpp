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

#if defined(HAVE_SYS_IOCTL_H) || defined(__linux__)
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
  #include <winsock2.h>
  #include <winsock.h>
  #include <iphlpapi.h>
  #include <strstream>

#define MSG_NOSIGNAL 0
#define MSG_DONTWAIT 0

#else

  #include <unistd.h>
  #include <stdlib.h>
  #include <sys/socket.h>
  #include <netdb.h>
  #include <netinet/in.h>
  #include <net/if.h>
  #include <netinet/tcp.h>
#endif


#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "miniwget.h"
#include "miniupnpc.h"
#include "upnpcommands.h"

#include "leak_dumper.h"

using namespace std;
using namespace Shared::Util;

namespace Shared{ namespace Platform{

bool Socket::disableNagle = false;
int Socket::DEFAULT_SOCKET_SENDBUF_SIZE = -1;
int Socket::DEFAULT_SOCKET_RECVBUF_SIZE = -1;

int Socket::broadcast_portno    = 61357;
int ServerSocket::ftpServerPort = 61358;
int ServerSocket::maxPlayerCount = -1;
int ServerSocket::externalPort  = Socket::broadcast_portno;
BroadCastClientSocketThread *ClientSocket::broadCastClientThread = NULL;
SDL_Thread *ServerSocket::upnpdiscoverThread = NULL;
Mutex ServerSocket::mutexUpnpdiscoverThread;
//
// UPnP - Start
//
static struct UPNPUrls urls;
static struct IGDdatas data;
// local ip address
static char lanaddr[16]         = "";
bool UPNP_Tools::isUPNP         = true;
bool UPNP_Tools::enabledUPNP    = false;
Mutex UPNP_Tools::mutexUPNP;
// UPnP - End

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
		static char acErrorBuffer[8096];
		std::ostrstream outs(acErrorBuffer, 8095);
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
			outs << "unknown socket error";
		}
		outs << " (" << Target.nID << ")";

		// Finish error message off and return it.
		outs << std::ends;
		acErrorBuffer[8095] = '\0';
		return acErrorBuffer;
	}

	// keeps in scope for duration of the application
	SocketManager Socket::wsaManager;

	SocketManager::SocketManager() {
		WSADATA wsaData;
		WORD wVersionRequested = MAKEWORD(2, 0);
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("SocketManager calling WSAStartup...\n");
		WSAStartup(wVersionRequested, &wsaData);
		//dont throw exceptions here, this is a static initializacion
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Winsock initialized.\n");
	}

	SocketManager::~SocketManager() {
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("SocketManager calling WSACleanup...\n");
		WSACleanup();
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Winsock cleanup complete.\n");
	}

#else

	typedef unsigned int UINT_PTR, *PUINT_PTR;
	typedef UINT_PTR        SOCKET;
	#define INVALID_SOCKET  (SOCKET)(~0)

	#define PLATFORM_SOCKET_TRY_AGAIN EAGAIN
	#define PLATFORM_SOCKET_INPROGRESS EINPROGRESS
	#define PLATFORM_SOCKET_INTERRUPTED EINTR

#endif

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
	size_t offset= 0;
	int byteIndex= 0;

	for(byteIndex= 0; byteIndex<4; ++byteIndex){
		size_t dotPos= ipString.find_first_of('.', offset);

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

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(BSD) || defined(__APPLE__) || defined(__linux__)
# define USE_GETIFADDRS 1
# include <ifaddrs.h>
#endif

static uint32 SockAddrToUint32(struct in_addr * a) {
	uint32 result = 0;

	//printf("a [%p]\n",a);
	if(a != NULL) {
		result = ntohl(a->s_addr);
	}

	return result;
}

static uint32 SockAddrToUint32(struct sockaddr * a) {
	uint32 result = 0;

	//printf("a [%p] family = %d\n",a,(a ? a->sa_family : -1));
	if(a != NULL && (a->sa_family == AF_INET || a->sa_family == AF_UNSPEC)) {
		//result = SockAddrToUint32((((struct sockaddr_in *)a)->sin_addr.s_addr);
		result = SockAddrToUint32(&((struct sockaddr_in *)a)->sin_addr);
	}

	return result;
}

// convert a numeric IP address into its string representation
void Ip::Inet_NtoA(uint32 addr, char * ipbuf)
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
            char ifaAddrStr[32];  Ip::Inet_NtoA(ifaAddr,  ifaAddrStr);
            char maskAddrStr[32]; Ip::Inet_NtoA(maskAddr, maskAddrStr);
            char dstAddrStr[32];  Ip::Inet_NtoA(dstAddr,  dstAddrStr);
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
            snprintf(buf, 128,"unnamed-%i", i);
            name = buf;
         }

         uint32 ipAddr  = ntohl(row.dwAddr);
         uint32 netmask = ntohl(row.dwMask);
         uint32 baddr   = ipAddr & netmask;
         if (row.dwBCastAddr) baddr |= ~netmask;

         char ifaAddrStr[32];  Ip::Inet_NtoA(ipAddr,  ifaAddrStr);
         char maskAddrStr[32]; Ip::Inet_NtoA(netmask, maskAddrStr);
         char dstAddrStr[32];  Ip::Inet_NtoA(baddr,   dstAddrStr);
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
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
   struct ifaddrs * ifap;
   if (getifaddrs(&ifap) == 0)
   {
	   if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
      struct ifaddrs * p = ifap;
      while(p)
      {
         uint32 ifaAddr  = SockAddrToUint32(p->ifa_addr);
         uint32 maskAddr = SockAddrToUint32(p->ifa_netmask);
         uint32 dstAddr  = SockAddrToUint32(p->ifa_dstaddr);
         if (ifaAddr > 0)
         {
        	 if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

            char ifaAddrStr[32];  Ip::Inet_NtoA(ifaAddr,  ifaAddrStr);
            char maskAddrStr[32]; Ip::Inet_NtoA(maskAddr, maskAddrStr);
            char dstAddrStr[32];  Ip::Inet_NtoA(dstAddr,  dstAddrStr);
            //printf("  Found interface:  name=[%s] desc=[%s] address=[%s] netmask=[%s] broadcastAddr=[%s]\n", p->ifa_name, "unavailable", ifaAddrStr, maskAddrStr, dstAddrStr);
			if(strcmp(ifaAddrStr,ipAddress.c_str()) == 0) {
				broadCastAddress = dstAddrStr;
			}

			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] ifaAddrStr [%s], maskAddrStr [%s], dstAddrStr[%s], ipAddress [%s], broadCastAddress [%s]\n",__FILE__,__FUNCTION__,__LINE__,ifaAddrStr,maskAddrStr,dstAddrStr,ipAddress.c_str(),broadCastAddress.c_str());
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
         if (name == NULL)
         {
            name = "";
         }

         uint32 ipAddr  = ntohl(row.dwAddr);
         uint32 netmask = ntohl(row.dwMask);
         uint32 baddr   = ipAddr & netmask;
         if (row.dwBCastAddr) baddr |= ~netmask;

         char ifaAddrStr[32];  Ip::Inet_NtoA(ipAddr,  ifaAddrStr);
         char maskAddrStr[32]; Ip::Inet_NtoA(netmask, maskAddrStr);
         char dstAddrStr[32];  Ip::Inet_NtoA(baddr,   dstAddrStr);
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

uint32 Socket::getConnectedIPAddress(string IP) {
	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));

    addr.sin_family= AF_INET;
    if(IP == "") {
        IP = connectedIpAddress;
    }
	addr.sin_addr.s_addr= inet_addr(IP.c_str());
	//addr.sin_port= htons(port);

	return SockAddrToUint32((struct sockaddr *)&addr);
}

std::vector<std::string> Socket::getLocalIPAddressList() {
	std::vector<std::string> ipList;

	/* get my host name */
	char myhostname[101]="";
	gethostname(myhostname,100);
	char myhostaddr[101] = "";

	struct hostent* myhostent = gethostbyname(myhostname);
	if(myhostent) {
		// get all host IP addresses (Except for loopback)
		char myhostaddr[101] = "";
		//int ipIdx = 0;
		//while (myhostent->h_addr_list[ipIdx] != 0) {
		for(int ipIdx = 0; myhostent->h_addr_list[ipIdx] != NULL; ++ipIdx) {
			Ip::Inet_NtoA(SockAddrToUint32((struct in_addr *)myhostent->h_addr_list[ipIdx]), myhostaddr);

		   //printf("ipIdx = %d [%s]\n",ipIdx,myhostaddr);
		   if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] myhostaddr = [%s]\n",__FILE__,__FUNCTION__,__LINE__,myhostaddr);

		   if(strlen(myhostaddr) > 0 &&
			  strncmp(myhostaddr,"127.",4) != 0 &&
			  strncmp(myhostaddr,"0.",2) != 0) {
			   ipList.push_back(myhostaddr);
		   }
		   //ipIdx++;
		}
	}

#ifndef WIN32

	// Now check all linux network devices
	std::vector<string> intfTypes;
	intfTypes.push_back("lo");
	intfTypes.push_back("eth");
	intfTypes.push_back("wlan");
	intfTypes.push_back("vlan");
	intfTypes.push_back("vboxnet");
	intfTypes.push_back("br-lan");
	intfTypes.push_back("br-gest");

	for(int intfIdx = 0; intfIdx < intfTypes.size(); intfIdx++) {
		string intfName = intfTypes[intfIdx];
		for(int idx = 0; idx < 10; ++idx) {
			PLATFORM_SOCKET fd = socket(AF_INET, SOCK_DGRAM, 0);
			//PLATFORM_SOCKET fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

			/* I want to get an IPv4 IP address */
			struct ifreq ifr;
			struct ifreq ifrA;
			ifr.ifr_addr.sa_family = AF_INET;
			ifrA.ifr_addr.sa_family = AF_INET;

			/* I want IP address attached to "eth0" */
			char szBuf[100]="";
			snprintf(szBuf,100,"%s%d",intfName.c_str(),idx);
			int maxIfNameLength = std::min((int)strlen(szBuf),IFNAMSIZ-1);

			strncpy(ifr.ifr_name, szBuf, maxIfNameLength);
			ifr.ifr_name[maxIfNameLength] = '\0';
			strncpy(ifrA.ifr_name, szBuf, maxIfNameLength);
			ifrA.ifr_name[maxIfNameLength] = '\0';

			int result_ifaddrr = ioctl(fd, SIOCGIFADDR, &ifr);
			ioctl(fd, SIOCGIFFLAGS, &ifrA);
			close(fd);

			if(result_ifaddrr >= 0) {
				struct sockaddr_in *pSockAddr = (struct sockaddr_in *)&ifr.ifr_addr;
				if(pSockAddr != NULL) {

					Ip::Inet_NtoA(SockAddrToUint32(&pSockAddr->sin_addr), myhostaddr);
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] szBuf [%s], myhostaddr = [%s], ifr.ifr_flags = %d, ifrA.ifr_flags = %d, ifr.ifr_name [%s]\n",__FILE__,__FUNCTION__,__LINE__,szBuf,myhostaddr,ifr.ifr_flags,ifrA.ifr_flags,ifr.ifr_name);

					// Now only include interfaces that are both UP and running
					if( (ifrA.ifr_flags & IFF_UP) 		== IFF_UP &&
						(ifrA.ifr_flags & IFF_RUNNING) 	== IFF_RUNNING) {
						if( strlen(myhostaddr) > 0 &&
							strncmp(myhostaddr,"127.",4) != 0  &&
							strncmp(myhostaddr,"0.",2) != 0) {
							if(std::find(ipList.begin(),ipList.end(),myhostaddr) == ipList.end()) {
								ipList.push_back(myhostaddr);
							}
						}
					}
				}
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
	if(validateSocket == NULL || (*validateSocket) == 0) {
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

Socket::Socket(PLATFORM_SOCKET sock) {
	dataSynchAccessorRead = new Mutex();
	dataSynchAccessorWrite = new Mutex();
	inSocketDestructorSynchAccessor = new Mutex();

	MutexSafeWrapper safeMutexSocketDestructorFlag(inSocketDestructorSynchAccessor,CODE_AT_LINE);
	inSocketDestructorSynchAccessor->setOwnerId(CODE_AT_LINE);
	this->inSocketDestructor = false;
	lastThreadedPing = 0;
	lastDebugEvent = 0;
	//safeMutexSocketDestructorFlag.ReleaseLock();

	//this->pingThread = NULL;
	//pingThreadAccessor.setOwnerId(CODE_AT_LINE);
	dataSynchAccessorRead->setOwnerId(CODE_AT_LINE);
	dataSynchAccessorWrite->setOwnerId(CODE_AT_LINE);

	this->sock= sock;
	this->isSocketBlocking = true;
	this->connectedIpAddress = "";
}

Socket::Socket() {
	dataSynchAccessorRead = new Mutex();
	dataSynchAccessorWrite = new Mutex();
	inSocketDestructorSynchAccessor = new Mutex();

	MutexSafeWrapper safeMutexSocketDestructorFlag(inSocketDestructorSynchAccessor,CODE_AT_LINE);
	inSocketDestructorSynchAccessor->setOwnerId(CODE_AT_LINE);
	this->inSocketDestructor = false;
	//safeMutexSocketDestructorFlag.ReleaseLock();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//this->pingThread = NULL;

	this->connectedIpAddress = "";

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(isSocketValid() == false) {
		throwException("Error creating socket");
	}

	this->isSocketBlocking = true;

#ifdef __APPLE__
    int set = 1;
    setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
#endif

    /* Disable the Nagle (TCP No Delay) algorithm */
    if(Socket::disableNagle == true) {
		int flag = 1;
		int ret = setsockopt( sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag) );
		if (ret == -1) {
		  printf("Couldn't setsockopt(TCP_NODELAY)\n");
		}
    }

	if(Socket::DEFAULT_SOCKET_SENDBUF_SIZE >= 0) {
		int bufsize 	 = 0;
		socklen_t optlen = sizeof(bufsize);

		int ret = getsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *)&bufsize, &optlen);
		printf("Original setsockopt(SO_SNDBUF) = [%d] new will be [%d] ret = %d\n",bufsize,Socket::DEFAULT_SOCKET_SENDBUF_SIZE,ret);

		ret = setsockopt( sock, SOL_SOCKET, SO_SNDBUF, (char *) &Socket::DEFAULT_SOCKET_SENDBUF_SIZE, sizeof( int ) );
		if (ret == -1) {
		  printf("Couldn't setsockopt(SO_SNDBUF) [%d]\n",Socket::DEFAULT_SOCKET_SENDBUF_SIZE);
		}
	}

	if(Socket::DEFAULT_SOCKET_RECVBUF_SIZE >= 0) {
		int bufsize 	 = 0;
		socklen_t optlen = sizeof(bufsize);

		int ret = getsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *)&bufsize, &optlen);
		printf("Original setsockopt(SO_RCVBUF) = [%d] new will be [%d] ret = %d\n",bufsize,Socket::DEFAULT_SOCKET_RECVBUF_SIZE,ret);

		ret = setsockopt( sock, SOL_SOCKET, SO_RCVBUF, (char *) &Socket::DEFAULT_SOCKET_RECVBUF_SIZE, sizeof( int ) );
		if (ret == -1) {
		  printf("Couldn't setsockopt(SO_RCVBUF) [%d]\n",Socket::DEFAULT_SOCKET_RECVBUF_SIZE);
		}
	}

}

float Socket::getThreadedPingMS(std::string host) {
	float result = -1;
/*
	if(pingThread == NULL) {
		lastThreadedPing = 0;
		pingThread = new SimpleTaskThread(this,0,50);
		pingThread->setUniqueID(__FILE__ + "_" + __FUNCTION);
		pingThread->start();
	}

	if(pingCache.find(host) == pingCache.end()) {
		MutexSafeWrapper safeMutex(&pingThreadAccessor);
		safeMutex.ReleaseLock();
		result = getAveragePingMS(host, 1);
		pingCache[host]=result;
	}
	else {
		MutexSafeWrapper safeMutex(&pingThreadAccessor);
		result = pingCache[host];
		safeMutex.ReleaseLock();
	}
*/
	return result;
}

/*
void Socket::simpleTask(BaseThread *callingThread)  {
	// update ping times every x seconds
	const int pingFrequencySeconds = 2;
	if(difftime(time(NULL),lastThreadedPing) < pingFrequencySeconds) {
		return;
	}
	lastThreadedPing = time(NULL);

	//printf("Pinging hosts...\n");

	for(std::map<string,double>::iterator iterMap = pingCache.begin();
		iterMap != pingCache.end(); iterMap++) {
		MutexSafeWrapper safeMutex(&pingThreadAccessor,CODE_AT_LINE);
		iterMap->second = getAveragePingMS(iterMap->first, 1);
		safeMutex.ReleaseLock();
	}
}
*/

Socket::~Socket() {
	MutexSafeWrapper safeMutexSocketDestructorFlag(inSocketDestructorSynchAccessor,CODE_AT_LINE);
	if(this->inSocketDestructor == true) {
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] this->inSocketDestructor == true\n",__FILE__,__FUNCTION__,__LINE__);
		return;
	}
	inSocketDestructorSynchAccessor->setOwnerId(CODE_AT_LINE);
	this->inSocketDestructor = true;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] START closing socket = %d...\n",__FILE__,__FUNCTION__,sock);

	//safeMutexSocketDestructorFlag.ReleaseLock();

    disconnectSocket();

    // Allow other callers with a lock on the mutexes to let them go
	for(time_t elapsed = time(NULL);
		(dataSynchAccessorRead->getRefCount() > 0 ||
		 dataSynchAccessorWrite->getRefCount() > 0) &&
		 difftime((long int)time(NULL),elapsed) <= 2;) {
		printf("Waiting in socket destructor\n");
		//sleep(0);
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] END closing socket = %d...\n",__FILE__,__FUNCTION__,sock);

	//delete pingThread;
	//pingThread = NULL;
	safeMutexSocketDestructorFlag.ReleaseLock();

	delete dataSynchAccessorRead;
	dataSynchAccessorRead = NULL;
	delete dataSynchAccessorWrite;
	dataSynchAccessorWrite = NULL;
	delete inSocketDestructorSynchAccessor;
	inSocketDestructorSynchAccessor = NULL;
}

void Socket::disconnectSocket() {
	//printf("Socket disconnecting\n");

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] START closing socket = %d...\n",__FILE__,__FUNCTION__,sock);

    if(isSocketValid() == true) {
    	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] calling shutdown and close for socket = %d...\n",__FILE__,__FUNCTION__,sock);

        MutexSafeWrapper safeMutex(dataSynchAccessorRead,CODE_AT_LINE);
        MutexSafeWrapper safeMutex1(dataSynchAccessorWrite,CODE_AT_LINE);

        if(isSocketValid() == true) {
        ::shutdown(sock,2);
#ifndef WIN32
        ::close(sock);
        sock = INVALID_SOCKET;
#else
        ::closesocket(sock);
        sock = INVALID_SOCKET;
#endif
        }
        safeMutex.ReleaseLock();
        safeMutex1.ReleaseLock();
    }

    if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] END closing socket = %d...\n",__FILE__,__FUNCTION__,sock);
}

// Int lookup is socket fd while bool result is whether or not that socket was signalled for reading
bool Socket::hasDataToRead(std::map<PLATFORM_SOCKET,bool> &socketTriggeredList)
{
    bool bResult = false;

    if(socketTriggeredList.empty() == false) {
        /* Watch stdin (fd 0) to see when it has input. */
        fd_set rfds;
        FD_ZERO(&rfds);

		string socketDebugList = "";
        PLATFORM_SOCKET imaxsocket = 0;
        for(std::map<PLATFORM_SOCKET,bool>::iterator itermap = socketTriggeredList.begin();
            itermap != socketTriggeredList.end(); ++itermap) {
        	PLATFORM_SOCKET socket = itermap->first;
            if(Socket::isSocketValid(&socket) == true) {
                FD_SET(socket, &rfds);
                imaxsocket = max(socket,imaxsocket);

				if(socketDebugList != "") {
					socketDebugList += ",";
				}
				socketDebugList += intToStr(socket);
            }
        }

        if(imaxsocket > 0) {
            /* Wait up to 0 seconds. */
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 0;


            int retval = 0;
            {
            	//MutexSafeWrapper safeMutex(&dataSynchAccessor);
            	retval = select((int)imaxsocket + 1, &rfds, NULL, NULL, &tv);
            }
            if(retval < 0) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d, ERROR SELECTING SOCKET DATA retval = %d error = %s, socketDebugList [%s]\n",__FILE__,__FUNCTION__,__LINE__,retval,getLastSocketErrorFormattedText().c_str(),socketDebugList.c_str());
            }
            else if(retval) {
                bResult = true;

                for(std::map<PLATFORM_SOCKET,bool>::iterator itermap = socketTriggeredList.begin();
                    itermap != socketTriggeredList.end(); ++itermap) {
                	PLATFORM_SOCKET socket = itermap->first;
                    if (FD_ISSET(socket, &rfds)) {
                        itermap->second = true;
                    }
                    else {
                        itermap->second = false;
                    }
                }
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

        int retval = 0;
        {
        	//MutexSafeWrapper safeMutex(&dataSynchAccessor);
        	retval = select((int)socket + 1, &rfds, NULL, NULL, &tv);
        }
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

bool Socket::hasDataToReadWithWait(int waitMilliseconds)
{
    return Socket::hasDataToReadWithWait(sock,waitMilliseconds) ;
}

bool Socket::hasDataToReadWithWait(PLATFORM_SOCKET socket,int waitMilliseconds)
{
    bool bResult = false;

    Chrono chono;
    chono.start();
    if(Socket::isSocketValid(&socket) == true)
    {
        fd_set rfds;
        struct timeval tv;

        /* Watch stdin (fd 0) to see when it has input. */
        FD_ZERO(&rfds);
        FD_SET(socket, &rfds);

        /* Wait up to 0 seconds. */
        tv.tv_sec = 0;
        tv.tv_usec = 1000 * waitMilliseconds;

        int retval = 0;
        {
        	//MutexSafeWrapper safeMutex(&dataSynchAccessor);
        	retval = select((int)socket + 1, &rfds, NULL, NULL, &tv);
        }
        if(retval)
        {
            if (FD_ISSET(socket, &rfds))
            {
                bResult = true;
            }
        }
    }

    //printf("hasdata waited [%d] milliseconds [%d], bResult = %d\n",chono.getMillis(),waitMilliseconds,bResult);
    return bResult;
}

int Socket::getDataToRead(bool wantImmediateReply) {
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
    	//int loopCount = 1;
    	for(time_t elapsed = time(NULL); difftime((long int)time(NULL),elapsed) < 1;) {
			/* ioctl isn't posix, but the following seems to work on all modern
			 * unixes */
	#ifndef WIN32
			int err = ioctl(sock, FIONREAD, &size);
	#else
			int err= ioctlsocket(sock, FIONREAD, &size);
	#endif
			int lastSocketError = getLastSocketError();
			if(err < 0 && lastSocketError != PLATFORM_SOCKET_TRY_AGAIN)
			{
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] ERROR PEEKING SOCKET DATA, err = %d %s\n",__FILE__,__FUNCTION__,__LINE__,err,getLastSocketErrorFormattedText().c_str());
				break;
			}
			else if(err == 0)
			{
				if(isConnected() == false) {
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] ERROR PEEKING SOCKET DATA, err = %d %s\n",__FILE__,__FUNCTION__,__LINE__,err,getLastSocketErrorFormattedText().c_str());
					break;
				}
			}

			if(size > 0) {
				break;
			}
			else if(hasDataToRead() == true) {
				//if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] WARNING PEEKING SOCKET DATA, (hasDataToRead() == true) err = %d, sock = %d, size = " MG_SIZE_T_SPECIFIER ", loopCount = %d\n",__FILE__,__FUNCTION__,__LINE__,err,sock,size,loopCount);
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] WARNING PEEKING SOCKET DATA, (hasDataToRead() == true) err = %d, sock = %d, size = " MG_SIZE_T_SPECIFIER "\n",__FILE__,__FUNCTION__,__LINE__,err,sock,size);
			}
			else {
				//if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] WARNING PEEKING SOCKET DATA, err = %d, sock = %d, size = " MG_SIZE_T_SPECIFIER ", loopCount = %d\n",__FILE__,__FUNCTION__,__LINE__,err,sock,size,loopCount);
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] WARNING PEEKING SOCKET DATA, err = %d, sock = %d, size = " MG_SIZE_T_SPECIFIER "\n",__FILE__,__FUNCTION__,__LINE__,err,sock,size);
				break;
			}

			if(wantImmediateReply == true) {
				break;
			}

			//loopCount++;
    	}
    }

	return static_cast<int>(size);
}

int Socket::send(const void *data, int dataSize) {
	const int MAX_SEND_WAIT_SECONDS = 3;

	int bytesSent= 0;
	if(isSocketValid() == true)	{
		errno = 0;

//    	MutexSafeWrapper safeMutexSocketDestructorFlag(&inSocketDestructorSynchAccessor,CODE_AT_LINE);
//    	if(this->inSocketDestructor == true) {
//    		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] this->inSocketDestructor == true\n",__FILE__,__FUNCTION__,__LINE__);
//    		return -1;
//    	}
//    	inSocketDestructorSynchAccessor->setOwnerId(CODE_AT_LINE);
//    	safeMutexSocketDestructorFlag.ReleaseLock();

		MutexSafeWrapper safeMutex(dataSynchAccessorWrite,CODE_AT_LINE);

		if(isSocketValid() == true)	{
#ifdef __APPLE__
        bytesSent = ::send(sock, (const char *)data, dataSize, SO_NOSIGPIPE);
#else
        bytesSent = ::send(sock, (const char *)data, dataSize, MSG_NOSIGNAL | MSG_DONTWAIT);
#endif
		}
        safeMutex.ReleaseLock();
	}

	// TEST errors
	//bytesSent = -1;
	// END TEST

	int lastSocketError = getLastSocketError();
	if(bytesSent < 0 && lastSocketError != PLATFORM_SOCKET_TRY_AGAIN) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] ERROR WRITING SOCKET DATA, err = %d error = %s dataSize = %d\n",__FILE__,__FUNCTION__,__LINE__,bytesSent,getLastSocketErrorFormattedText(&lastSocketError).c_str(),dataSize);
	}
	else if(bytesSent < 0 && lastSocketError == PLATFORM_SOCKET_TRY_AGAIN && isConnected() == true) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] #1 EAGAIN during send, trying again... dataSize = %d\n",__FILE__,__FUNCTION__,__LINE__,dataSize);

		int attemptCount = 0;
	    time_t tStartTimer = time(NULL);
	    while((bytesSent < 0 && lastSocketError == PLATFORM_SOCKET_TRY_AGAIN) &&
	    		(difftime((long int)time(NULL),tStartTimer) <= MAX_SEND_WAIT_SECONDS)) {
	    	attemptCount++;
	    	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] attemptCount = %d\n",__FILE__,__FUNCTION__,__LINE__,attemptCount);

	    	MutexSafeWrapper safeMutex(dataSynchAccessorWrite,CODE_AT_LINE);
            if(isConnected() == true) {
            	struct timeval timeVal;
            	timeVal.tv_sec = 1;
            	timeVal.tv_usec = 0;
            	bool canWrite = isWritable(&timeVal);

            	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] attemptCount = %d, sock = %d, dataSize = %d, data = %p, canWrite = %d\n",__FILE__,__FUNCTION__,__LINE__,attemptCount,sock,dataSize,data,canWrite);

//	        	MutexSafeWrapper safeMutexSocketDestructorFlag(&inSocketDestructorSynchAccessor,CODE_AT_LINE);
//	        	if(this->inSocketDestructor == true) {
//	        		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] this->inSocketDestructor == true\n",__FILE__,__FUNCTION__,__LINE__);
//	        		return -1;
//	        	}
//	        	inSocketDestructorSynchAccessor->setOwnerId(CODE_AT_LINE);
//	        	safeMutexSocketDestructorFlag.ReleaseLock();


#ifdef __APPLE__
                bytesSent = ::send(sock, (const char *)data, dataSize, SO_NOSIGPIPE);
#else
                bytesSent = ::send(sock, (const char *)data, dataSize, MSG_NOSIGNAL | MSG_DONTWAIT);
#endif
				lastSocketError = getLastSocketError();
                if(bytesSent < 0 && lastSocketError != PLATFORM_SOCKET_TRY_AGAIN) {
                    break;
                }

                //safeMutex.ReleaseLock();

                if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] #2 EAGAIN during send, trying again returned: %d\n",__FILE__,__FUNCTION__,__LINE__,bytesSent);
	        }
	        else {
                int iErr = getLastSocketError();

                safeMutex.ReleaseLock();
                disconnectSocket();

                if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"[%s::%s] DISCONNECTED SOCKET error while sending socket data, bytesSent = %d, error = %s\n",__FILE__,__FUNCTION__,bytesSent,getLastSocketErrorFormattedText(&iErr).c_str());
                break;

	        }
            if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] attemptCount = %d\n",__FILE__,__FUNCTION__,__LINE__,attemptCount);
	    }
	}

	if(isConnected() == true && bytesSent > 0 && bytesSent < dataSize) {
		lastSocketError = getLastSocketError();
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] need to send more data, trying again getLastSocketError() = %d, bytesSent = %d, dataSize = %d\n",__FILE__,__FUNCTION__,__LINE__,lastSocketError,bytesSent,dataSize);

		int totalBytesSent = bytesSent;
		int attemptCount = 0;

		
	    time_t tStartTimer = time(NULL);
	    while(((bytesSent > 0 && totalBytesSent < dataSize) ||
	    		(bytesSent < 0 && lastSocketError == PLATFORM_SOCKET_TRY_AGAIN)) &&
	    		(difftime((long int)time(NULL),tStartTimer) <= MAX_SEND_WAIT_SECONDS)) {
	    	attemptCount++;
	    	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] attemptCount = %d, totalBytesSent = %d\n",__FILE__,__FUNCTION__,__LINE__,attemptCount,totalBytesSent);

	    	MutexSafeWrapper safeMutex(dataSynchAccessorWrite,CODE_AT_LINE);
            if(isConnected() == true) {
            	struct timeval timeVal;
            	timeVal.tv_sec = 1;
            	timeVal.tv_usec = 0;
            	bool canWrite = isWritable(&timeVal);

            	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] attemptCount = %d, sock = %d, dataSize = %d, data = %p, canWrite = %d\n",__FILE__,__FUNCTION__,__LINE__,attemptCount,sock,dataSize,data,canWrite);

//	        	MutexSafeWrapper safeMutexSocketDestructorFlag(&inSocketDestructorSynchAccessor,CODE_AT_LINE);
//	        	if(this->inSocketDestructor == true) {
//	        		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] this->inSocketDestructor == true\n",__FILE__,__FUNCTION__,__LINE__);
//	        		return -1;
//	        	}
//	        	inSocketDestructorSynchAccessor->setOwnerId(CODE_AT_LINE);
//	        	safeMutexSocketDestructorFlag.ReleaseLock();


	        	const char *sendBuf = (const char *)data;
#ifdef __APPLE__
			    bytesSent = ::send(sock, &sendBuf[totalBytesSent], dataSize - totalBytesSent, SO_NOSIGPIPE);
#else
			    bytesSent = ::send(sock, &sendBuf[totalBytesSent], dataSize - totalBytesSent, MSG_NOSIGNAL | MSG_DONTWAIT);
#endif
				lastSocketError = getLastSocketError();
                if(bytesSent > 0) {
                	totalBytesSent += bytesSent;
                }

                if(bytesSent < 0 && lastSocketError != PLATFORM_SOCKET_TRY_AGAIN) {
                    break;
                }

			    if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] retry send returned: %d\n",__FILE__,__FUNCTION__,__LINE__,bytesSent);
	        }
	        else {
                int iErr = getLastSocketError();

                safeMutex.ReleaseLock();
                disconnectSocket();

                if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"[%s::%s Line: %d] DISCONNECTED SOCKET error while sending socket data, bytesSent = %d, error = %s\n",__FILE__,__FUNCTION__,__LINE__,bytesSent,getLastSocketErrorFormattedText(&iErr).c_str());
	            break;
	        }

            if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] attemptCount = %d\n",__FILE__,__FUNCTION__,__LINE__,attemptCount);
	    }

	    if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] bytesSent = %d, totalBytesSent = %d\n",__FILE__,__FUNCTION__,__LINE__,bytesSent,totalBytesSent);

	    if(bytesSent > 0) {
	    	bytesSent = totalBytesSent;
	    }
	}

	if(bytesSent <= 0 || isConnected() == false) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] ERROR WRITING SOCKET DATA, err = %d error = %s\n",__FILE__,__FUNCTION__,__LINE__,bytesSent,getLastSocketErrorFormattedText().c_str());

	    int iErr = getLastSocketError();
	    disconnectSocket();

	    if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"[%s::%s Line: %d] DISCONNECTED SOCKET error while sending socket data, bytesSent = %d, error = %s\n",__FILE__,__FUNCTION__,__LINE__,bytesSent,getLastSocketErrorFormattedText(&iErr).c_str());
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] sock = %d, bytesSent = %d\n",__FILE__,__FUNCTION__,__LINE__,sock,bytesSent);

	return static_cast<int>(bytesSent);
}

int Socket::receive(void *data, int dataSize, bool tryReceiveUntilDataSizeMet) {
	const int MAX_RECV_WAIT_SECONDS = 3;

	ssize_t bytesReceived = 0;

	if(isSocketValid() == true)	{
//    	MutexSafeWrapper safeMutexSocketDestructorFlag(&inSocketDestructorSynchAccessor,CODE_AT_LINE);
//    	if(this->inSocketDestructor == true) {
//    		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] this->inSocketDestructor == true\n",__FILE__,__FUNCTION__,__LINE__);
//    		return -1;
//    	}
//    	inSocketDestructorSynchAccessor->setOwnerId(CODE_AT_LINE);
//    	safeMutexSocketDestructorFlag.ReleaseLock();

		MutexSafeWrapper safeMutex(dataSynchAccessorRead,CODE_AT_LINE);
		if(isSocketValid() == true)	{
			bytesReceived = recv(sock, reinterpret_cast<char*>(data), dataSize, 0);
		}
	    safeMutex.ReleaseLock();
	}
	int lastSocketError = getLastSocketError();
	if(bytesReceived < 0 && lastSocketError != PLATFORM_SOCKET_TRY_AGAIN) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"[%s::%s Line: %d] ERROR READING SOCKET DATA error while sending socket data, bytesSent = %d, error = %s\n",__FILE__,__FUNCTION__,__LINE__,bytesReceived,getLastSocketErrorFormattedText(&lastSocketError).c_str());
	}
	else if(bytesReceived < 0 && lastSocketError == PLATFORM_SOCKET_TRY_AGAIN)	{
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] #1 EAGAIN during receive, trying again...\n",__FILE__,__FUNCTION__,__LINE__);

	    time_t tStartTimer = time(NULL);
	    while((bytesReceived < 0 && lastSocketError == PLATFORM_SOCKET_TRY_AGAIN) &&
	    		(difftime((long int)time(NULL),tStartTimer) <= MAX_RECV_WAIT_SECONDS)) {
	        if(isConnected() == false) {
                int iErr = getLastSocketError();
                disconnectSocket();

                if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"[%s::%s Line: %d] DISCONNECTED SOCKET error while receiving socket data, bytesSent = %d, error = %s\n",__FILE__,__FUNCTION__,__LINE__,bytesReceived,getLastSocketErrorFormattedText(&iErr).c_str());
	            break;
	        }
	        else if(Socket::isReadable(true) == true) {
//	        	MutexSafeWrapper safeMutexSocketDestructorFlag(&inSocketDestructorSynchAccessor,CODE_AT_LINE);
//	        	if(this->inSocketDestructor == true) {
//	        		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] this->inSocketDestructor == true\n",__FILE__,__FUNCTION__,__LINE__);
//	        		return -1;
//	        	}
//	        	inSocketDestructorSynchAccessor->setOwnerId(CODE_AT_LINE);
//	        	safeMutexSocketDestructorFlag.ReleaseLock();

	        	MutexSafeWrapper safeMutex(dataSynchAccessorRead,CODE_AT_LINE);
                bytesReceived = recv(sock, reinterpret_cast<char*>(data), dataSize, 0);
				lastSocketError = getLastSocketError();
                safeMutex.ReleaseLock();

                if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] #2 EAGAIN during receive, trying again returned: %d\n",__FILE__,__FUNCTION__,__LINE__,bytesReceived);
	        }
	    }
	}

	if(bytesReceived <= 0) {
	    int iErr = getLastSocketError();
	    disconnectSocket();

	    if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"[%s::%s Line: %d] DISCONNECTED SOCKET error while receiving socket data, bytesReceived = %d, error = %s, dataSize = %d, tryReceiveUntilDataSizeMet = %d\n",__FILE__,__FUNCTION__,__LINE__,bytesReceived,getLastSocketErrorFormattedText(&iErr).c_str(),dataSize,tryReceiveUntilDataSizeMet);
	}
	else if(tryReceiveUntilDataSizeMet == true && bytesReceived < dataSize) {
		int newBufferSize = (dataSize - bytesReceived);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] WARNING, attempting to receive MORE data, bytesReceived = %d, dataSize = %d, newBufferSize = %d\n",__FILE__,__FUNCTION__,__LINE__,bytesReceived,dataSize,newBufferSize);
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\nIn [%s::%s Line: %d] WARNING, attempting to receive MORE data, bytesReceived = %d, dataSize = %d, newBufferSize = %d\n",__FILE__,__FUNCTION__,__LINE__,(int)bytesReceived,dataSize,newBufferSize);

		char *dataAsCharPointer = reinterpret_cast<char *>(data);
		int additionalBytes = receive(&dataAsCharPointer[bytesReceived], newBufferSize, tryReceiveUntilDataSizeMet);

		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] WARNING, additionalBytes = %d\n",__FILE__,__FUNCTION__,__LINE__,additionalBytes);
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\nIn [%s::%s Line: %d] WARNING, additionalBytes = %d\n",__FILE__,__FUNCTION__,__LINE__,additionalBytes);

		if(additionalBytes > 0) {
			bytesReceived += additionalBytes;
		}
		else {
			//throw megaglest_runtime_error("additionalBytes == " + intToStr(additionalBytes));
			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] additionalBytes == %d\n",__FILE__,__FUNCTION__,__LINE__,additionalBytes);
			if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\nIn [%s::%s Line: %d] additionalBytes == %d\n",__FILE__,__FUNCTION__,__LINE__,additionalBytes);
			if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] additionalBytes == %d\n",__FILE__,__FUNCTION__,__LINE__,additionalBytes);

		    int iErr = getLastSocketError();
		    disconnectSocket();

		    if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"[%s::%s Line: %d] DISCONNECTED SOCKET error while receiving socket data, bytesReceived = %d, error = %s, dataSize = %d, tryReceiveUntilDataSizeMet = %d\n",__FILE__,__FUNCTION__,__LINE__,bytesReceived,getLastSocketErrorFormattedText(&iErr).c_str(),dataSize,tryReceiveUntilDataSizeMet);
		}
	}
	return static_cast<int>(bytesReceived);
}

SafeSocketBlockToggleWrapper::SafeSocketBlockToggleWrapper(Socket *socket, bool toggle) {
	this->socket = socket;
	if(this->socket != NULL) {
		this->originallyBlocked = socket->getBlock();
		this->newBlocked = toggle;

		if(this->originallyBlocked != this->newBlocked) {
			socket->setBlock(this->newBlocked);
		}
	}
}

void SafeSocketBlockToggleWrapper::Restore() {
	if(this->socket != NULL) {
		if(this->originallyBlocked != this->newBlocked) {
			socket->setBlock(this->originallyBlocked);
			this->newBlocked = this->originallyBlocked;
		}
	}
}

SafeSocketBlockToggleWrapper::~SafeSocketBlockToggleWrapper() {
	Restore();
}

int Socket::peek(void *data, int dataSize,bool mustGetData,int *pLastSocketError) {
	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) chrono.start();

    const int MAX_PEEK_WAIT_SECONDS = 3;

    int lastSocketError = 0;
	int err = 0;
	if(isSocketValid() == true) {
		//if(chrono.getMillis() > 1) printf("In [%s::%s Line: %d] action running for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());

//    	MutexSafeWrapper safeMutexSocketDestructorFlag(&inSocketDestructorSynchAccessor,CODE_AT_LINE);
//    	if(this->inSocketDestructor == true) {
//    		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] this->inSocketDestructor == true\n",__FILE__,__FUNCTION__,__LINE__);
//    		return -1;
//    	}
//    	inSocketDestructorSynchAccessor->setOwnerId(CODE_AT_LINE);
//    	safeMutexSocketDestructorFlag.ReleaseLock();

		//MutexSafeWrapper safeMutex(&dataSynchAccessor,CODE_AT_LINE + "_" + intToStr(sock) + "_" + intToStr(dataSize));
		MutexSafeWrapper safeMutex(dataSynchAccessorRead,CODE_AT_LINE);

		//if(chrono.getMillis() > 1) printf("In [%s::%s Line: %d] action running for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());
		if(isSocketValid() == true)	{
//			Chrono recvTimer(true);
			SafeSocketBlockToggleWrapper safeUnblock(this, false);
			errno = 0;
			err = recv(sock, reinterpret_cast<char*>(data), dataSize, MSG_PEEK);
			lastSocketError = getLastSocketError();
			if(pLastSocketError != NULL) {
				*pLastSocketError = lastSocketError;
			}
			safeUnblock.Restore();

//			if(recvTimer.getMillis() > 1000 || (err <= 0 && lastSocketError != 0 && lastSocketError != PLATFORM_SOCKET_TRY_AGAIN)) {
//				printf("#1 PEEK err = %d lastSocketError = %d ms: %lld\n",err,lastSocketError,(long long int)recvTimer.getMillis());
//			}
		}
	    safeMutex.ReleaseLock();

	    //printf("Peek #1 err = %d\n",err);

	    if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) if(chrono.getMillis() > 1) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] action running for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());
	}
	else {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] SOCKET appears to be invalid [%d]\n",__FILE__,__FUNCTION__,__LINE__,sock);
	}
	//if(chrono.getMillis() > 1) printf("In [%s::%s Line: %d] action running for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());

	if(err < 0 && lastSocketError != PLATFORM_SOCKET_TRY_AGAIN) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"[%s::%s Line: %d] ERROR PEEKING SOCKET DATA error while sending socket data, err = %d, error = %s\n",__FILE__,__FUNCTION__,__LINE__,err,getLastSocketErrorFormattedText().c_str());
		disconnectSocket();
	}
	else if(err < 0 && lastSocketError == PLATFORM_SOCKET_TRY_AGAIN && mustGetData == true) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] #1 ERROR EAGAIN during peek, trying again...\n",__FILE__,__FUNCTION__,__LINE__);

		//printf("Peek #2 err = %d\n",err);

	    time_t tStartTimer = time(NULL);
	    while((err < 0 && lastSocketError == PLATFORM_SOCKET_TRY_AGAIN) &&
	    		isSocketValid() == true &&
	    		(difftime((long int)time(NULL),tStartTimer) <= MAX_PEEK_WAIT_SECONDS)) {
/*
	        if(isConnected() == false) {
                int iErr = getLastSocketError();
                disconnectSocket();
	            break;
	        }
*/
	        if(Socket::isReadable(true) == true) {

//	        	MutexSafeWrapper safeMutexSocketDestructorFlag(&inSocketDestructorSynchAccessor,CODE_AT_LINE);
//	        	if(this->inSocketDestructor == true) {
//	        		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] this->inSocketDestructor == true\n",__FILE__,__FUNCTION__,__LINE__);
//	        		return -1;
//	        	}
//	        	inSocketDestructorSynchAccessor->setOwnerId(CODE_AT_LINE);
//	        	safeMutexSocketDestructorFlag.ReleaseLock();

	        	//MutexSafeWrapper safeMutex(&dataSynchAccessor,CODE_AT_LINE + "_" + intToStr(sock) + "_" + intToStr(dataSize));
	        	MutexSafeWrapper safeMutex(dataSynchAccessorRead,CODE_AT_LINE);

//	        	Chrono recvTimer(true);
	        	SafeSocketBlockToggleWrapper safeUnblock(this, false);
	        	errno = 0;
                err = recv(sock, reinterpret_cast<char*>(data), dataSize, MSG_PEEK);
				lastSocketError = getLastSocketError();
				if(pLastSocketError != NULL) {
					*pLastSocketError = lastSocketError;
				}
				safeUnblock.Restore();

//                if(recvTimer.getMillis() > 1000 || (err <= 0 && lastSocketError != 0 && lastSocketError != PLATFORM_SOCKET_TRY_AGAIN)) {
//    				printf("#2 PEEK err = %d lastSocketError = %d ms: %lld\n",err,lastSocketError,(long long int)recvTimer.getMillis());
//    			}

				//printf("Socket peek delayed checking for sock = %d err = %d\n",sock,err);

                safeMutex.ReleaseLock();

                if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) if(chrono.getMillis() > 1) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] action running for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());
                if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] #2 EAGAIN during peek, trying again returned: %d\n",__FILE__,__FUNCTION__,__LINE__,err);
	        }
	        else {
	        	//printf("Socket peek delayed [NOT READABLE] checking for sock = %d err = %d\n",sock,err);
	        }
	    }

	    if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) if(chrono.getMillis() > 1) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] action running for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());
	}
	else {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] #2 SOCKET appears to be invalid [%d] lastSocketError [%d] err [%d] mustGetData [%d]\n",__FILE__,__FUNCTION__,__LINE__,sock,lastSocketError,err,mustGetData);
	}

	//if(chrono.getMillis() > 1) printf("In [%s::%s Line: %d] action running for msecs: %lld\n",__FILE__,__FUNCTION__,__LINE__,(long long int)chrono.getMillis());

	if(err < 0 || (err == 0 && dataSize != 0) ||
		((err == 0 || err == -1) && dataSize == 0 && lastSocketError != 0 && lastSocketError != PLATFORM_SOCKET_TRY_AGAIN)) {
		//printf("** #1 Socket peek error for sock = %d err = %d lastSocketError = %d\n",sock,err,lastSocketError);

		int iErr = lastSocketError;
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"[%s::%s Line: %d] DISCONNECTING SOCKET for socket [%d], err = %d, error = %s\n",__FILE__,__FUNCTION__,__LINE__,socket,err,getLastSocketErrorFormattedText(&iErr).c_str());
		//printf("Peek #3 err = %d\n",err);
		//lastSocketError = getLastSocketError();
		if(mustGetData == true || lastSocketError != PLATFORM_SOCKET_TRY_AGAIN) {
			//printf("** #2 Socket peek error for sock = %d err = %d lastSocketError = %d mustGetData = %d\n",sock,err,lastSocketError,mustGetData);

			int iErr = lastSocketError;
			disconnectSocket();

			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"[%s::%s Line: %d] DISCONNECTED SOCKET error while peeking socket data for socket [%d], err = %d, error = %s\n",__FILE__,__FUNCTION__,__LINE__,socket,err,getLastSocketErrorFormattedText(&iErr).c_str());
		}
	}

	return static_cast<int>(err);
}

bool Socket::getBlock() {
	bool blocking = true;

	// don't waste time if the socket is invalid
	if(isSocketValid(&sock) == false) {
		return blocking;
	}

//#ifndef WIN32
//	int currentFlags = fcntl(sock, F_GETFL);
//	blocking = !((currentFlags & O_NONBLOCK) == O_NONBLOCK);
//#else
	blocking = this->isSocketBlocking;
//#endif
	return blocking;
}

void Socket::setBlock(bool block){
	setBlock(block,this->sock);
	this->isSocketBlocking = block;
}

void Socket::setBlock(bool block, PLATFORM_SOCKET socket) {
	// don't waste time if the socket is invalid
	if(isSocketValid(&socket) == false) {
		return;
	}

#ifndef WIN32
	int currentFlags = fcntl(socket, F_GETFL);
	if(currentFlags < 0) {
		currentFlags = 0;
	}
	if(block == true) {
		currentFlags &= (~O_NONBLOCK);
	}
	else {
		currentFlags |= O_NONBLOCK;
	}
	int err= fcntl(socket, F_SETFL, currentFlags);
#else
	u_long iMode= !block;
	int err= ioctlsocket(socket, FIONBIO, &iMode);
#endif
	if(err < 0) {
		throwException("Error setting I/O mode for socket");
	}
}

bool Socket::isReadable(bool lockMutex) {
    if(isSocketValid() == false) return false;

    struct timeval tv;
	tv.tv_sec= 0;
	tv.tv_usec= 0;

	fd_set set;
	FD_ZERO(&set);

	MutexSafeWrapper safeMutex(NULL,CODE_AT_LINE);
	if(lockMutex == true) {
		safeMutex.setMutex(dataSynchAccessorRead,CODE_AT_LINE);
	}

	FD_SET(sock, &set);

	int i = 0;
	{
    	//MutexSafeWrapper safeMutexSocketDestructorFlag(&inSocketDestructorSynchAccessor,CODE_AT_LINE);
    	//if(this->inSocketDestructor == true) {
    	//	SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] this->inSocketDestructor == true\n",__FILE__,__FUNCTION__,__LINE__);
    	//	return false;
    	//}
    	//inSocketDestructorSynchAccessor->setOwnerId(CODE_AT_LINE);
    	//safeMutexSocketDestructorFlag.ReleaseLock();

		//MutexSafeWrapper safeMutex(&dataSynchAccessorRead,CODE_AT_LINE);
		i= select((int)sock + 1, &set, NULL, NULL, &tv);
	}
	if(i < 0) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"[%s::%s] error while selecting socket data, err = %d, error = %s\n",__FILE__,__FUNCTION__,i,getLastSocketErrorFormattedText().c_str());
	}

	bool result = (i == 1);
	//if(result == false) {
	//	SystemFlags::OutputDebug(SystemFlags::debugError,"SOCKET DISCONNECTED In [%s::%s Line: %d] i = %d sock = %d\n",__FILE__,__FUNCTION__,__LINE__,i,sock);
	//}

	return result;
}

bool Socket::isWritable(struct timeval *timeVal, bool lockMutex) {
    if(isSocketValid() == false) return false;

	struct timeval tv;
	if(timeVal != NULL) {
		tv = *timeVal;
	}
	else {
		tv.tv_sec= 0;
		//tv.tv_usec= 1;
		tv.tv_usec= 0;
	}

	fd_set set;
	FD_ZERO(&set);

	MutexSafeWrapper safeMutex(NULL,CODE_AT_LINE);
	if(lockMutex == true) {
		safeMutex.setMutex(dataSynchAccessorWrite,CODE_AT_LINE);
	}
	FD_SET(sock, &set);

	int i = 0;
	{
    	//MutexSafeWrapper safeMutexSocketDestructorFlag(&inSocketDestructorSynchAccessor,CODE_AT_LINE);
    	//if(this->inSocketDestructor == true) {
    	//	SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] this->inSocketDestructor == true\n",__FILE__,__FUNCTION__,__LINE__);
    	//	return false;
    	//}
    	//inSocketDestructorSynchAccessor->setOwnerId(CODE_AT_LINE);
    	//safeMutexSocketDestructorFlag.ReleaseLock();

		//MutexSafeWrapper safeMutex(&dataSynchAccessorWrite,CODE_AT_LINE);
		i = select((int)sock + 1, NULL, &set, NULL, &tv);
	}
	bool result = false;
	if(i < 0 ) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"[%s::%s Line: %d] error while selecting socket data, err = %d, error = %s\n",__FILE__,__FUNCTION__,__LINE__,i,getLastSocketErrorFormattedText().c_str());
		SystemFlags::OutputDebug(SystemFlags::debugError,"SOCKET DISCONNECTED In [%s::%s Line: %d] error while selecting socket data, err = %d, error = %s\n",__FILE__,__FUNCTION__,__LINE__,i,getLastSocketErrorFormattedText().c_str());
	}
	else if(i == 0) {
		//SystemFlags::OutputDebug(SystemFlags::debugNetwork,"[%s::%s Line: %d] TIMEOUT while selecting socket data, err = %d, error = %s\n",__FILE__,__FUNCTION__,__LINE__,i,getLastSocketErrorFormattedText().c_str());
		// Assume we are still connected, write buffer could be very busy
		result = true;
		SystemFlags::OutputDebug(SystemFlags::debugError,"SOCKET WRITE TIMEOUT In [%s::%s Line: %d] i = %d sock = %d\n",__FILE__,__FUNCTION__,__LINE__,i,sock);
	}
	else {
		result = true;
	}

	//return (i == 1 && FD_ISSET(sock, &set));
	return result;
}

bool Socket::isConnected() {
	if(isSocketValid() == false) return false;

//	MutexSafeWrapper safeMutexSocketDestructorFlag(&inSocketDestructorSynchAccessor,CODE_AT_LINE);
//	if(this->inSocketDestructor == true) {
//		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] this->inSocketDestructor == true\n",__FILE__,__FUNCTION__,__LINE__);
//		return false;
//	}
//	inSocketDestructorSynchAccessor->setOwnerId(CODE_AT_LINE);

	//if the socket is not writable then it is not conencted
	if(isWritable(NULL,true) == false) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"[%s::%s Line: %d] ERROR isWritable failed.\n",__FILE__,__FUNCTION__,__LINE__);
		return false;
	}
	//if the socket is readable it is connected if we can read a byte from it
	if(isReadable(true)) {
		char tmp=0;
		int peekDataBytes=1;
		int lastSocketError=0;
		//int err = peek(&tmp, 1, false, &lastSocketError);
		int err = peek(&tmp, peekDataBytes, false, &lastSocketError);
		//if(err <= 0 && err != PLATFORM_SOCKET_TRY_AGAIN) {
		if(err <= 0 && lastSocketError != 0 && lastSocketError != PLATFORM_SOCKET_TRY_AGAIN) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"[%s::%s Line: %d] ERROR Peek failed, err = %d for socket: %d, error = %s, lastSocketError = %d\n",__FILE__,__FUNCTION__,__LINE__,err,sock,getLastSocketErrorFormattedText().c_str(),lastSocketError);
			if(SystemFlags::VERBOSE_MODE_ENABLED) SystemFlags::OutputDebug(SystemFlags::debugError,"SOCKET DISCONNECTED In [%s::%s Line: %d] ERROR Peek failed, err = %d for socket: %d, error = %s, lastSocketError = %d\n",__FILE__,__FUNCTION__,__LINE__,err,sock,getLastSocketErrorFormattedText().c_str(),lastSocketError);
			return false;
		}
	}

	//otherwise the socket is connected
	return true;
}

string Socket::getHostName()  {
	static string host = "";
	if(host == "") {
		const int strSize= 257;
		char hostname[strSize]="";
		int result = gethostname(hostname, strSize);
		if(result == 0) {
			host = (hostname[0] != '\0' ? hostname : "");
		}
		else {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] result = %d, error = %s\n",__FILE__,__FUNCTION__,__LINE__,result,getLastSocketErrorText());
		}
	}
	return host;
}

string Socket::getIp() {
	hostent* info= gethostbyname(getHostName().c_str());
	unsigned char* address;

	if(info==NULL){
		throw megaglest_runtime_error("Error getting host by name");
	}

	address= reinterpret_cast<unsigned char*>(info->h_addr_list[0]);

	if(address==NULL){
		throw megaglest_runtime_error("Error getting host ip");
	}

	return
		intToStr(address[0]) + "." +
		intToStr(address[1]) + "." +
		intToStr(address[2]) + "." +
		intToStr(address[3]);
}

void Socket::throwException(string str){
	string msg = str + " " + getLastSocketErrorFormattedText();
	throw megaglest_runtime_error(msg);
}

// ===============================================
//	class ClientSocket
// ===============================================

ClientSocket::ClientSocket() : Socket() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	stopBroadCastClientThread();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

ClientSocket::~ClientSocket() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	stopBroadCastClientThread();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void ClientSocket::stopBroadCastClientThread() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	if(broadCastClientThread != NULL) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		broadCastClientThread->shutdownAndWait();
		delete broadCastClientThread;
		broadCastClientThread = NULL;
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void ClientSocket::startBroadCastClientThread(DiscoveredServersInterface *cb) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	ClientSocket::stopBroadCastClientThread();

	static string mutexOwnerId = string(extractFileFromDirectoryPath(__FILE__).c_str()) + string("_") + intToStr(__LINE__);
	broadCastClientThread = new BroadCastClientSocketThread(cb);
	broadCastClientThread->setUniqueID(mutexOwnerId);
	broadCastClientThread->start();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
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

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Connecting to host [%s] on port = %d\n", ip.getString().c_str(),port);
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf ("Connecting to host [%s] on port = %d\n", ip.getString().c_str(),port);

    connectedIpAddress = "";
	int err= ::connect(sock, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
	if(err < 0)	{
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] #2 Error connecting socket for IP: %s for Port: %d err = %d error = %s\n",__FILE__,__FUNCTION__,__LINE__,ip.getString().c_str(),port,err,getLastSocketErrorFormattedText().c_str());
	    if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] #2 Error connecting socket for IP: %s for Port: %d err = %d error = %s\n",__FILE__,__FUNCTION__,__LINE__,ip.getString().c_str(),port,err,getLastSocketErrorFormattedText().c_str());

		int lastSocketError = getLastSocketError();
        if (lastSocketError == PLATFORM_SOCKET_INPROGRESS ||
        	lastSocketError == PLATFORM_SOCKET_TRY_AGAIN) {
            fd_set myset;
            struct timeval tv;
            int valopt=0;
            socklen_t lon=0;

            if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] PLATFORM_SOCKET_INPROGRESS in connect() - selecting\n",__FILE__,__FUNCTION__,__LINE__);
            if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] PLATFORM_SOCKET_INPROGRESS in connect() - selecting\n",__FILE__,__FUNCTION__,__LINE__);

            do {
               tv.tv_sec = 10;
               tv.tv_usec = 0;

               FD_ZERO(&myset);
               FD_SET(sock, &myset);

               {
            	   MutexSafeWrapper safeMutex(dataSynchAccessorRead,CODE_AT_LINE);
            	   err = select((int)sock + 1, NULL, &myset, NULL, &tv);
				   lastSocketError = getLastSocketError();
            	   //safeMutex.ReleaseLock();
               }

               if (err < 0 && lastSocketError != PLATFORM_SOCKET_INTERRUPTED) {
            	   if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] Error connecting %s\n",__FILE__,__FUNCTION__,__LINE__,getLastSocketErrorFormattedText().c_str());
            	   if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] Error connecting %s\n",__FILE__,__FUNCTION__,__LINE__,getLastSocketErrorFormattedText().c_str());

                  break;
               }
               else if(err > 0) {
               //else if(FD_ISSET(sock, &myset)) {
                  // Socket selected for write
                  lon = sizeof(valopt);
#ifndef WIN32
                  if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0)
#else
                  if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *)(&valopt), &lon) < 0)
#endif
                  {
                	  if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] Error in getsockopt() %s\n",__FILE__,__FUNCTION__,__LINE__,getLastSocketErrorFormattedText().c_str());
                	  if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] Error in getsockopt() %s\n",__FILE__,__FUNCTION__,__LINE__,getLastSocketErrorFormattedText().c_str());
                     break;
                  }
                  // Check the value returned...
                  if(valopt) {
                	  if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] Error in delayed connection() %d - [%s]\n",__FILE__,__FUNCTION__,__LINE__,valopt, strerror(valopt));
                	  if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] Error in delayed connection() %d - [%s]\n",__FILE__,__FUNCTION__,__LINE__,valopt, strerror(valopt));
                     break;
                  }

                  errno = 0;
                  if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] Apparent recovery for connection sock = %d, err = %d\n",__FILE__,__FUNCTION__,__LINE__,sock,err);
                  if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] Apparent recovery for connection sock = %d, err = %d\n",__FILE__,__FUNCTION__,__LINE__,sock,err);

                  break;
               }
               else {
            	   if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] Timeout in select() - Cancelling!\n",__FILE__,__FUNCTION__,__LINE__);
            	   if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] Timeout in select() - Cancelling!\n",__FILE__,__FUNCTION__,__LINE__);

                  disconnectSocket();
                  break;
               }
            } while (1);
        }

        if(err < 0) {
        	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] Before END sock = %d, err = %d, error = %s\n",__FILE__,__FUNCTION__,__LINE__,sock,err,getLastSocketErrorFormattedText().c_str());
        	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] Before END sock = %d, err = %d, error = %s\n",__FILE__,__FUNCTION__,__LINE__,sock,err,getLastSocketErrorFormattedText().c_str());
            disconnectSocket();
        }
        else {
        	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] Valid recovery for connection sock = %d, err = %d, error = %s\n",__FILE__,__FUNCTION__,__LINE__,sock,err,getLastSocketErrorFormattedText().c_str());
        	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("In [%s::%s Line: %d] Valid recovery for connection sock = %d, err = %d, error = %s\n",__FILE__,__FUNCTION__,__LINE__,sock,err,getLastSocketErrorFormattedText().c_str());
        	connectedIpAddress = ip.getString();
        }
	}
	else {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Connected to host [%s] on port = %d sock = %d err = %d", ip.getString().c_str(),port,sock,err);
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("Connected to host [%s] on port = %d sock = %d err = %d", ip.getString().c_str(),port,sock,err);
		connectedIpAddress = ip.getString();
	}
}

//=======================================================================
// Function :		discovery response thread
// Description:		Runs in its own thread to listen for broadcasts from
//					other servers
//
BroadCastClientSocketThread::BroadCastClientSocketThread(DiscoveredServersInterface *cb) : BaseThread() {

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	discoveredServersCB = cb;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

//=======================================================================
// Function :		broadcast thread
// Description:		Runs in its own thread to send out a broadcast to the local network
//					the current broadcast message is <myhostname:my.ip.address.dotted>
//
void BroadCastClientSocketThread::execute() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	RunningStatusSafeWrapper runningStatus(this);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Broadcast Client thread is running\n");

	std::vector<string> foundServers;

	short port;                 // The port for the broadcast.
    struct sockaddr_in bcSender; // local socket address for the broadcast.
    struct sockaddr_in bcaddr;  // The broadcast address for the receiver.
    PLATFORM_SOCKET bcfd;                // The file descriptor used for the broadcast.
    //bool one = true;            // Parameter for "setscokopt".
    char buff[10024]="";            // Buffers the data to be broadcasted.
    socklen_t alen=0;

    port = htons( Socket::getBroadCastPort() );

    // Prepare to receive the broadcast.
    bcfd = socket(AF_INET, SOCK_DGRAM, 0);
    if( bcfd <= 0 )	{
    	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"socket failed: %s\n", getLastSocketErrorFormattedText().c_str());
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
		if(::bind( bcfd,  (struct sockaddr *)&bcaddr, sizeof(bcaddr) ) < 0 )	{
			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"bind failed: %s\n", getLastSocketErrorFormattedText().c_str());
		}
		else {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			Socket::setBlock(false, bcfd);

			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			try	{
				// Keep getting packets forever.
				for( time_t elapsed = time(NULL); difftime((long int)time(NULL),elapsed) <= 5; ) {
					alen = sizeof(struct sockaddr);
					int nb=0;// The number of bytes read.
					bool gotData = (nb = recvfrom(bcfd, buff, 10024, 0, (struct sockaddr *) &bcSender, &alen)) > 0;

					//printf("Broadcasting client nb = %d buff [%s] gotData = %d\n",nb,buff,gotData);

					if(gotData == false) {
						if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"recvfrom failed: %s\n", getLastSocketErrorFormattedText().c_str());
					}
					else {
						//string fromIP = inet_ntoa(bcSender.sin_addr);
						char szHostFrom[100]="";
						Ip::Inet_NtoA(SockAddrToUint32(&bcSender.sin_addr), szHostFrom);
						//printf("Client szHostFrom [%s]\n",szHostFrom);

						string fromIP = szHostFrom;
						if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"broadcast message received: [%s] from: [%s]\n", buff,fromIP.c_str() );

						if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Client got broadcast from server: [%s] data [%s]\n",fromIP.c_str(),buff);

						int serverGamePort = Socket::getBroadCastPort();
						vector<string> paramPartPortsTokens;
						Tokenize(buff,paramPartPortsTokens,":");
						if(paramPartPortsTokens.size() >= 3 && paramPartPortsTokens[2].length() > 0) {
							serverGamePort = strToInt(paramPartPortsTokens[2]);
						}

						if(std::find(foundServers.begin(),foundServers.end(),fromIP) == foundServers.end()) {
							foundServers.push_back(fromIP + ":" + intToStr(serverGamePort));
						}

						// For now break as soon as we find a server
						break;
					}

					if(getQuitStatus() == true) {
						if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
						break;
					}
					sleep( 100 ); // send out broadcast every 1 seconds
					if(getQuitStatus() == true) {
						if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
						break;
					}
				}
			}
			catch(const exception &ex) {
				SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
			}
			catch(...) {
				SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] UNKNOWN Error\n",__FILE__,__FUNCTION__,__LINE__);
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] unknown error\n",__FILE__,__FUNCTION__,__LINE__);
			}
		}

#ifndef WIN32
        ::close(bcfd);
        bcfd = INVALID_SOCKET;
#else
        ::closesocket(bcfd);
        bcfd = INVALID_SOCKET;
#endif

    }

    if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

    // Here we callback into the implementer class
    if(getQuitStatus() == false && discoveredServersCB != NULL) {
    	discoveredServersCB->DiscoveredServers(foundServers);
    }

    if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Broadcast Client thread is exiting\n");
    if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

// ===============================================
//	class ServerSocket
// ===============================================

ServerSocket::ServerSocket(bool basicMode) : Socket() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] basicMode = %d\n",__FILE__,__FUNCTION__,__LINE__,basicMode);

	this->basicMode = basicMode;
	this->bindSpecificAddress = "";
	//printf("SERVER SOCKET CONSTRUCTOR\n");
	//MutexSafeWrapper safeMutexUPNP(&ServerSocket::mutexUpnpdiscoverThread,CODE_AT_LINE);
	//ServerSocket::upnpdiscoverThread = NULL;
	//safeMutexUPNP.ReleaseLock();

	boundPort = 0;
	portBound = false;
	broadCastThread = NULL;
	if(this->basicMode == false) {
		UPNP_Tools::enabledUPNP = false;
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

ServerSocket::~ServerSocket() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//printf("In [%s::%s] Line: %d UPNP_Tools::enabledUPNP = %d\n",__FILE__,__FUNCTION__,__LINE__,UPNP_Tools::enabledUPNP);

	stopBroadCastThread();

	if(this->basicMode == false) {
		//printf("In [%s::%s] Line: %d safeMutexUPNP\n",__FILE__,__FUNCTION__,__LINE__);
		//printf("SERVER SOCKET DESTRUCTOR\n");
		MutexSafeWrapper safeMutexUPNP(&ServerSocket::mutexUpnpdiscoverThread,CODE_AT_LINE);
		if(ServerSocket::upnpdiscoverThread != NULL) {
			SDL_WaitThread(ServerSocket::upnpdiscoverThread, NULL);
			ServerSocket::upnpdiscoverThread = NULL;
		}
		safeMutexUPNP.ReleaseLock();
		//printf("In [%s::%s] Line: %d safeMutexUPNP\n",__FILE__,__FUNCTION__,__LINE__);


		//printf("In [%s::%s] Line: %d UPNP_Tools::enabledUPNP = %d\n",__FILE__,__FUNCTION__,__LINE__,UPNP_Tools::enabledUPNP);
		if (UPNP_Tools::enabledUPNP) {
			//UPNP_Tools::NETremRedirects(ServerSocket::externalPort);
			UPNP_Tools::NETremRedirects(this->getExternalPort());
			//UPNP_Tools::enabledUPNP = false;
		}

		MutexSafeWrapper safeMutexUPNP1(&UPNP_Tools::mutexUPNP,CODE_AT_LINE);
		if(urls.controlURL && urls.ipcondescURL && urls.controlURL_CIF) {
			FreeUPNPUrls(&urls);
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		safeMutexUPNP1.ReleaseLock();
	}
}

void ServerSocket::stopBroadCastThread() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	//printf("Stopping broadcast thread [%p]\n",broadCastThread);

	if(broadCastThread != NULL) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

		//printf("Stopping broadcast thread [%p] - A\n",broadCastThread);

		if(broadCastThread->canShutdown(false) == true) {
			sleep(0);
		}
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		//printf("Stopping broadcast thread [%p] - B\n",broadCastThread);

		if(broadCastThread->shutdownAndWait() == true) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			delete broadCastThread;

			//printf("Stopping broadcast thread [%p] - C\n",broadCastThread);
		}
		broadCastThread = NULL;
		//printf("Stopping broadcast thread [%p] - D\n",broadCastThread);

		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void ServerSocket::startBroadCastThread() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	stopBroadCastThread();

	broadCastThread = new BroadCastSocketThread(this->boundPort);

	//printf("Start broadcast thread [%p]\n",broadCastThread);

	static string mutexOwnerId = string(extractFileFromDirectoryPath(__FILE__).c_str()) + string("_") + intToStr(__LINE__);
	broadCastThread->setUniqueID(mutexOwnerId);
	broadCastThread->start();

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
}

void ServerSocket::resumeBroadcast() {
	if(broadCastThread != NULL) {
		broadCastThread->setPauseBroadcast(false);
	}
}
void ServerSocket::pauseBroadcast() {
	if(broadCastThread != NULL) {
		broadCastThread->setPauseBroadcast(true);
	}
}

bool ServerSocket::isBroadCastThreadRunning() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	bool isThreadRunning = (broadCastThread != NULL && broadCastThread->getRunningStatus() == true);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] isThreadRunning = %d\n",__FILE__,__FUNCTION__,__LINE__,isThreadRunning);

	return isThreadRunning;
}

void ServerSocket::bind(int port) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d port = %d, portBound = %d START\n",__FILE__,__FUNCTION__,__LINE__,port,portBound);

	boundPort = port;

	if(isSocketValid() == false) {
		sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if(isSocketValid() == false) {
			throwException("Error creating socket");
		}
		setBlock(false);
	}

	//sockaddr structure
	sockaddr_in addr;
	addr.sin_family= AF_INET;
	if(this->bindSpecificAddress != "") {
		addr.sin_addr.s_addr= inet_addr(this->bindSpecificAddress.c_str());
	}
	else {
		addr.sin_addr.s_addr= INADDR_ANY;
	}
	addr.sin_port= htons(port);

	int val = 1;

#ifndef WIN32
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
#else
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&val, sizeof(val));
#endif

	int err= ::bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
	if(err < 0) {
	    char szBuf[8096]="";
	    snprintf(szBuf, 8096,"In [%s::%s] Error binding socket sock = %d, address [%s] port = %d err = %d, error = %s\n",__FILE__,__FUNCTION__,sock,this->bindSpecificAddress.c_str(),port,err,getLastSocketErrorFormattedText().c_str());
	    if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"%s",szBuf);

	    snprintf(szBuf, 8096,"Error binding socket sock = %d, address [%s] port = %d err = %d, error = %s\n",sock,this->bindSpecificAddress.c_str(),port,err,getLastSocketErrorFormattedText().c_str());
	    throw megaglest_runtime_error(szBuf);
	}
	portBound = true;

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d port = %d, portBound = %d END\n",__FILE__,__FUNCTION__,__LINE__,port,portBound);
}

void ServerSocket::disconnectSocket() {
	Socket::disconnectSocket();
	portBound = false;
}

void ServerSocket::listen(int connectionQueueSize) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s] Line: %d connectionQueueSize = %d\n",__FILE__,__FUNCTION__,__LINE__,connectionQueueSize);

	if(connectionQueueSize > 0) {
		if(isSocketValid() == false) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

			sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if(isSocketValid() == false) {
				throwException("Error creating socket");
			}
			setBlock(false);
		}

		if(portBound == false) {
			bind(boundPort);
		}

		int err= ::listen(sock, connectionQueueSize);
		if(err < 0) {
			char szBuf[8096]="";
			snprintf(szBuf, 8096,"In [%s::%s] Error listening socket sock = %d, err = %d, error = %s\n",__FILE__,__FUNCTION__,sock,err,getLastSocketErrorFormattedText().c_str());
			throwException(szBuf);
		}
	}
	else {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
		disconnectSocket();
	}

	if(this->basicMode == false) {
		if(connectionQueueSize > 0) {
			if(isBroadCastThreadRunning() == false) {
				startBroadCastThread();
			}
			else {
				resumeBroadcast();
			}
		}
		else {
			pauseBroadcast();
		}
	}
}

Socket *ServerSocket::accept(bool errorOnFail) {
	if(isSocketValid() == false) {
		if(errorOnFail == true) {
			throwException("socket is invalid!");
		}
		else {
			return NULL;
		}
	}

	PLATFORM_SOCKET newSock=0;
	char client_host[100]="";
	const int max_attempts = 100;
	for(int attempt = 0; attempt < max_attempts; ++attempt) {
		struct sockaddr_in cli_addr;
		socklen_t clilen = sizeof(cli_addr);
		client_host[0]='\0';
		MutexSafeWrapper safeMutex(dataSynchAccessorRead,CODE_AT_LINE);
		newSock= ::accept(sock, (struct sockaddr *) &cli_addr, &clilen);
		safeMutex.ReleaseLock();

		if(isSocketValid(&newSock) == false) {
			char szBuf[8096]="";
			snprintf(szBuf, 8096,"In [%s::%s Line: %d] Error accepting socket connection sock = %d, err = %d, error = %s\n",__FILE__,__FUNCTION__,__LINE__,sock,newSock,getLastSocketErrorFormattedText().c_str());
			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] %s\n",__FILE__,__FUNCTION__,__LINE__,szBuf);

			int lastSocketError = getLastSocketError();
			if(lastSocketError == PLATFORM_SOCKET_TRY_AGAIN) {
				if(attempt+1 >= max_attempts) {
					return NULL;
				}
				else {
					sleep(0);
				}
			}
			if(errorOnFail == true) {
				throwException(szBuf);
			}
			else {
				return NULL;
			}

		}
		else {
			Ip::Inet_NtoA(SockAddrToUint32((struct sockaddr *)&cli_addr), client_host);
			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] got connection, newSock = %d client_host [%s]\n",__FILE__,__FUNCTION__,__LINE__,newSock,client_host);
		}
		if(isIPAddressBlocked((client_host[0] != '\0' ? client_host : "")) == true) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] BLOCKING connection, newSock = %d client_host [%s]\n",__FILE__,__FUNCTION__,__LINE__,newSock,client_host);

	#ifndef WIN32
			::close(newSock);
			newSock = INVALID_SOCKET;
	#else
			::closesocket(newSock);
			newSock = INVALID_SOCKET;
	#endif

			return NULL;
		}

		break;
	}
	Socket *result = new Socket(newSock);
	result->setIpAddress((client_host[0] != '\0' ? client_host : ""));
	return result;
}

void ServerSocket::NETdiscoverUPnPDevices() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] UPNP - Start\n",__FILE__,__FUNCTION__,__LINE__);

	//printf("SERVER SOCKET NETdiscoverUPnPDevices - START\n");
	//printf("In [%s::%s] Line: %d safeMutexUPNP\n",__FILE__,__FUNCTION__,__LINE__);
	MutexSafeWrapper safeMutexUPNP(&ServerSocket::mutexUpnpdiscoverThread,CODE_AT_LINE);
	if(ServerSocket::upnpdiscoverThread != NULL) {
		SDL_WaitThread(ServerSocket::upnpdiscoverThread, NULL);
		ServerSocket::upnpdiscoverThread = NULL;
	}

    // WATCH OUT! Because the thread takes void * as a parameter we MUST cast to the pointer type
    // used on the other side (inside the thread)
	//printf("STARTING UPNP Thread\n");
	ServerSocket::upnpdiscoverThread = SDL_CreateThread(&UPNP_Tools::upnp_init, dynamic_cast<UPNPInitInterface *>(this));
	safeMutexUPNP.ReleaseLock();
	//printf("In [%s::%s] Line: %d safeMutexUPNP\n",__FILE__,__FUNCTION__,__LINE__);

	//printf("SERVER SOCKET NETdiscoverUPnPDevices - END\n");
}

void ServerSocket::UPNPInitStatus(bool result) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] result = %d\n",__FILE__,__FUNCTION__,__LINE__,result);

    if(result == true) {
        //int ports[4] = { this->getExternalPort(), this->getBindPort(), this->getFTPServerPort(), this->getFTPServerPort() };
        std::vector<int> UPNPPortForwardList;

        // Glest Game Server port
        UPNPPortForwardList.push_back(this->getExternalPort());
        UPNPPortForwardList.push_back(this->getBindPort());

        // Glest mini FTP Server Listen Port
        UPNPPortForwardList.push_back(this->getFTPServerPort());
        UPNPPortForwardList.push_back(this->getFTPServerPort());

        // GLest mini FTP Server Passive file TRansfer ports (1 per game player)
        for(int clientIndex = 1; clientIndex <= ServerSocket::maxPlayerCount; ++clientIndex) {
            UPNPPortForwardList.push_back(this->getFTPServerPort() + clientIndex);
            UPNPPortForwardList.push_back(this->getFTPServerPort() + clientIndex);
        }

        UPNP_Tools::NETaddRedirects(UPNPPortForwardList,false);
    }
}

//
// UPNP Tools Start
//
void UPNP_Tools::AddUPNPPortForward(int internalPort, int externalPort) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] internalPort = %d, externalPort = %d\n",__FILE__,__FUNCTION__,__LINE__,internalPort,externalPort);

    bool addPorts = (UPNP_Tools::enabledUPNP == true);
    if(addPorts == false) {
        int result = UPNP_Tools::upnp_init(NULL);
        addPorts = (result != 0);
    }

    if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] internalPort = %d, externalPort = %d, addPorts = %d\n",__FILE__,__FUNCTION__,__LINE__,internalPort,externalPort,addPorts);

    if(addPorts == true) {
        int ports[2] = { externalPort, internalPort };
        UPNP_Tools::upnp_add_redirect(ports);
    }
}

void UPNP_Tools::RemoveUPNPPortForward(int internalPort, int externalPort) {
    UPNP_Tools::upnp_rem_redirect(externalPort);
}

//
// This code below handles Universal Plug and Play Router Discovery
//
int UPNP_Tools::upnp_init(void *param) {
	int result				= -1;
	struct UPNPDev *devlist = NULL;
	struct UPNPDev *dev     = NULL;
	char *descXML           = NULL;
	int descXMLsize         = 0;
	char buf[255]           = "";
	// Callers MUST pass in NULL or a UPNPInitInterface *
	UPNPInitInterface *callback = (UPNPInitInterface *)(param);

	MutexSafeWrapper safeMutexUPNP(&UPNP_Tools::mutexUPNP,CODE_AT_LINE);
	memset(&urls, 0, sizeof(struct UPNPUrls));
	memset(&data, 0, sizeof(struct IGDdatas));

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] isUPNP = %d callback = %p\n",__FILE__,__FUNCTION__,__LINE__,UPNP_Tools::isUPNP,callback);

	if(UPNP_Tools::isUPNP) {
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Searching for UPnP devices for automatic port forwarding...\n");

		int upnp_delay = 5000;
		const char *upnp_multicastif = NULL;
		const char *upnp_minissdpdsock = NULL;
		int upnp_sameport = 0;
		int upnp_ipv6 = 0;
		int upnp_error = 0;

#ifndef MINIUPNPC_VERSION_PRE1_6
		devlist = upnpDiscover(upnp_delay, upnp_multicastif, upnp_minissdpdsock, upnp_sameport, upnp_ipv6, &upnp_error);

		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"UPnP discover returned upnp_error = %d.\n",upnp_error);
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("UPnP discover returned upnp_error = %d.\n",upnp_error);

#else
		devlist = upnpDiscover(upnp_delay, upnp_multicastif, upnp_minissdpdsock, upnp_sameport);
#endif
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"UPnP device search finished devlist = %p.\n",devlist);
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("UPnP device search finished devlist = %p.\n",devlist);

		if (devlist) {
			dev = devlist;
			while (dev) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"UPnP discover deviceList [%s].\n",(dev->st ? dev->st : "null"));
				if(SystemFlags::VERBOSE_MODE_ENABLED) printf("UPnP discover deviceList [%s].\n",(dev->st ? dev->st : "null"));

				dev = dev->pNext;
			}

			dev = devlist;
			while (dev && dev->st) {
				if (strstr(dev->st, "InternetGatewayDevice")) {
					break;
				}
				dev = dev->pNext;
			}
			if (!dev) {
				dev = devlist; /* defaulting to first device */
			}

			if(dev != NULL) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"UPnP device found: %s %s\n", dev->descURL, dev->st);
				if(SystemFlags::VERBOSE_MODE_ENABLED) printf("UPnP device found: %s %s\n", dev->descURL, dev->st);

				//printf("UPnP device found: [%s] [%s] lanaddr [%s]\n", dev->descURL, dev->st,lanaddr);
#if !defined(MINIUPNPC_VERSION_PRE1_7) && !defined(MINIUPNPC_VERSION_PRE1_6)
				descXML = (char *)miniwget_getaddr(dev->descURL, &descXMLsize, lanaddr, (sizeof(lanaddr) / sizeof(lanaddr[0])),0);
#else
				descXML = (char *)miniwget_getaddr(dev->descURL, &descXMLsize, lanaddr, (sizeof(lanaddr) / sizeof(lanaddr[0])));
#endif
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"LAN address: %s\n", lanaddr);

				if (descXML) {
					parserootdesc (descXML, descXMLsize, &data);
					free (descXML); descXML = 0;

#if !defined(MINIUPNPC_VERSION_PRE1_7) && !defined(MINIUPNPC_VERSION_PRE1_6)
					GetUPNPUrls (&urls, &data, dev->descURL,0);
#else
					GetUPNPUrls (&urls, &data, dev->descURL);
#endif
				}
				snprintf(buf, 255,"UPnP device found: %s %s LAN address %s", dev->descURL, dev->st, lanaddr);

				freeUPNPDevlist(devlist);
				devlist = NULL;
			}

			if (!urls.controlURL || urls.controlURL[0] == '\0')	{
				snprintf(buf, 255,"controlURL not available, UPnP disabled");
				if(callback) {
					safeMutexUPNP.ReleaseLock();
				    callback->UPNPInitStatus(false);
				}
				result = 0;
			}
			else {
				char externalIP[16]     = "";
#ifndef MINIUPNPC_VERSION_PRE1_5
				UPNP_GetExternalIPAddress(urls.controlURL, data.first.servicetype, externalIP);
#else
				UPNP_GetExternalIPAddress(urls.controlURL, data.servicetype, externalIP);
#endif
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"UPnP device found at: [%s] callback [%p]\n",externalIP,callback);

				//UPNP_Tools::NETaddRedirects(ports);
				UPNP_Tools::enabledUPNP = true;
				if(callback) {
					safeMutexUPNP.ReleaseLock();
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
					callback->UPNPInitStatus(true);
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
				}
				result = 1;
			}
		}

		if(result == -1) {
			snprintf(buf, 255,"UPnP device not found.");

			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"No UPnP devices found.\n");

			if(callback) {
				safeMutexUPNP.ReleaseLock();
				callback->UPNPInitStatus(false);
			}
			result = 0;
		}
	}
	else {
		snprintf(buf, 255,"UPnP detection routine disabled by user.");
		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"UPnP detection routine disabled by user.\n");

        if(callback) {
        	safeMutexUPNP.ReleaseLock();
            callback->UPNPInitStatus(false);
        }
        result = 0;
	}

	//printf("ENDING UPNP Thread\n");

	return result;
}

bool UPNP_Tools::upnp_add_redirect(int ports[2],bool mutexLock) {
	bool result				= true;
	char externalIP[16]     = "";
	char ext_port_str[16]   = "";
	char int_port_str[16]   = "";

	//printf("SERVER SOCKET upnp_add_redirect - START\n");
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] upnp_add_redir(%d : %d)\n",__FILE__,__FUNCTION__,__LINE__,ports[0],ports[1]);

	if(mutexLock) {
		//printf("In [%s::%s] Line: %d safeMutexUPNP\n",__FILE__,__FUNCTION__,__LINE__);
		MutexSafeWrapper safeMutexUPNP(&ServerSocket::mutexUpnpdiscoverThread,CODE_AT_LINE);
		if(ServerSocket::upnpdiscoverThread != NULL) {
			SDL_WaitThread(ServerSocket::upnpdiscoverThread, NULL);
			ServerSocket::upnpdiscoverThread = NULL;
		}
		safeMutexUPNP.ReleaseLock();
		//printf("In [%s::%s] Line: %d safeMutexUPNP\n",__FILE__,__FUNCTION__,__LINE__);
	}

	MutexSafeWrapper safeMutexUPNP(&UPNP_Tools::mutexUPNP,CODE_AT_LINE);
	if (!urls.controlURL || urls.controlURL[0] == '\0')	{
		result = false;
	}
	else {
#ifndef MINIUPNPC_VERSION_PRE1_5
		UPNP_GetExternalIPAddress(urls.controlURL, data.first.servicetype, externalIP);
#else
		UPNP_GetExternalIPAddress(urls.controlURL, data.servicetype, externalIP);
#endif

		sprintf(ext_port_str, "%d", ports[0]);
		sprintf(int_port_str, "%d", ports[1]);

		//int r                   = 0;
#ifndef MINIUPNPC_VERSION_PRE1_5
	#ifndef MINIUPNPC_VERSION_PRE1_6
		int r = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,ext_port_str, int_port_str, lanaddr, "MegaGlest - www.megaglest.org", "TCP", 0, NULL);
	#else
		int r = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,ext_port_str, int_port_str, lanaddr, "MegaGlest - www.megaglest.org", "TCP", 0);
	#endif
#else
		int r = UPNP_AddPortMapping(urls.controlURL, data.servicetype,ext_port_str, int_port_str, lanaddr, "MegaGlest - www.megaglest.org", "TCP", 0);
#endif
		if (r != UPNPCOMMAND_SUCCESS) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] AddPortMapping(%s, %s, %s) failed\n",__FILE__,__FUNCTION__,__LINE__,ext_port_str, int_port_str, lanaddr);
			result = false;
		}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] AddPortMapping(%s, %s, %s) success\n",__FILE__,__FUNCTION__,__LINE__,ext_port_str, int_port_str, lanaddr);
	}

	//printf("SERVER SOCKET upnp_add_redirect - END [%d]\n",result);
	return result;
}

void UPNP_Tools::upnp_rem_redirect(int ext_port) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] upnp_rem_redir(%d)\n",__FILE__,__FUNCTION__,__LINE__,ext_port);

	//printf("SERVER SOCKET upnp_rem_redirect - START\n");

	//printf("In [%s::%s] Line: %d safeMutexUPNP\n",__FILE__,__FUNCTION__,__LINE__);
	MutexSafeWrapper safeMutexUPNP(&ServerSocket::mutexUpnpdiscoverThread,CODE_AT_LINE);
    if(ServerSocket::upnpdiscoverThread != NULL) {
        SDL_WaitThread(ServerSocket::upnpdiscoverThread, NULL);
        ServerSocket::upnpdiscoverThread = NULL;
    }
    safeMutexUPNP.ReleaseLock();
    //printf("In [%s::%s] Line: %d safeMutexUPNP\n",__FILE__,__FUNCTION__,__LINE__);

	//printf("In [%s::%s] Line: %d ext_port = %d urls.controlURL = [%s]\n",__FILE__,__FUNCTION__,__LINE__,ext_port,urls.controlURL);

    MutexSafeWrapper safeMutexUPNP1(&UPNP_Tools::mutexUPNP,CODE_AT_LINE);
	if (urls.controlURL && urls.controlURL[0] != '\0')	{
		char ext_port_str[16]="";
		sprintf(ext_port_str, "%d", ext_port);

		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\n\n#1 DEBUGGING urls.controlURL [%s]\n",urls.controlURL);

		int result = 0;
	#ifndef MINIUPNPC_VERSION_PRE1_5
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\n\n#1 DEBUGGING data.first.servicetype [%s]\n",data.first.servicetype);
		result = UPNP_DeletePortMapping(urls.controlURL, data.first.servicetype, ext_port_str, "TCP", 0);
	#else
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\n\n#1 DEBUGGING data.servicetype [%s]\n",data.servicetype);
		result = UPNP_DeletePortMapping(urls.controlURL, data.servicetype, ext_port_str, "TCP", 0);
	#endif
		if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\n\nresult = %d\n",result);
		//printf("\n\nresult = %d\n",result);
	}
	if(SystemFlags::VERBOSE_MODE_ENABLED) printf("\n\n#2 DEBUGGING urls.controlURL [%s]\n",urls.controlURL);

	//printf("SERVER SOCKET upnp_rem_redirect - END\n");
}

void UPNP_Tools::NETaddRedirects(std::vector<int> UPNPPortForwardList,bool mutexLock) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] upnp_rem_redir(%d)\n",__FILE__,__FUNCTION__,__LINE__);

    if(UPNPPortForwardList.size() % 2 != 0) {
        // We need groups of 2 ports.. one external and one internal for opening ports on UPNP router
        throw megaglest_runtime_error("UPNPPortForwardList.size() MUST BE divisable by 2");
    }

    for(unsigned int clientIndex = 0; clientIndex < UPNPPortForwardList.size(); clientIndex += 2) {
        int ports[2] = { UPNPPortForwardList[clientIndex], UPNPPortForwardList[clientIndex+1] };
        upnp_add_redirect(ports,mutexLock);
    }
}

void UPNP_Tools::NETremRedirects(int ext_port) {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] upnp_rem_redir(%d)\n",__FILE__,__FUNCTION__,__LINE__);
    upnp_rem_redirect(ext_port);
}
//
// UPNP Tools END
//

//=======================================================================
// Function :		broadcast_thread
// in		:		none
// return	:		none
// Description:		To be forked in its own thread to send out a broadcast to the local subnet
//					the current broadcast message is <myhostname:my.ip.address.dotted>
//
BroadCastSocketThread::BroadCastSocketThread(int boundPort) : BaseThread() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	mutexPauseBroadcast = new Mutex();
	setPauseBroadcast(false);
	this->boundPort = boundPort;
	//printf("new broadcast thread [%p]\n",this);
}

BroadCastSocketThread::~BroadCastSocketThread() {
	//printf("delete broadcast thread [%p]\n",this);

	delete mutexPauseBroadcast;
	mutexPauseBroadcast = NULL;
}

bool BroadCastSocketThread::getPauseBroadcast() {
	MutexSafeWrapper safeMutexSocketDestructorFlag(mutexPauseBroadcast,CODE_AT_LINE);
	mutexPauseBroadcast->setOwnerId(CODE_AT_LINE);
	return pauseBroadcast;
}

void BroadCastSocketThread::setPauseBroadcast(bool value) {
	MutexSafeWrapper safeMutexSocketDestructorFlag(mutexPauseBroadcast,CODE_AT_LINE);
	mutexPauseBroadcast->setOwnerId(CODE_AT_LINE);
	pauseBroadcast = value;
}


bool BroadCastSocketThread::canShutdown(bool deleteSelfIfShutdownDelayed) {
	bool ret = (getExecutingTask() == false);
	if(ret == false && deleteSelfIfShutdownDelayed == true) {
	    setDeleteSelfOnExecutionDone(deleteSelfIfShutdownDelayed);
	    signalQuit();
	}

	return ret;
}

void BroadCastSocketThread::execute() {
	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
	//setRunningStatus(true);
	RunningStatusSafeWrapper runningStatus(this);
	ExecutingTaskSafeWrapper safeExecutingTaskMutex(this);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Broadcast thread is running\n");

	const int MAX_NIC_COUNT = 10;
    short port=0;                 // The port for the broadcast.
    struct sockaddr_in bcLocal[MAX_NIC_COUNT]; // local socket address for the broadcast.
    PLATFORM_SOCKET bcfd[MAX_NIC_COUNT];                // The socket used for the broadcast.
    int one = 1;            // Parameter for "setscokopt".
    int pn=0;                     // The number of the packet broadcasted.
    const int buffMaxSize=1024;
    char buff[buffMaxSize]="";            // Buffers the data to be broadcasted.
    char myhostname[100]="";       // hostname of local machine
    //char subnetmask[MAX_NIC_COUNT][100];       // Subnet mask to broadcast to
    //struct hostent* myhostent=NULL;

    /* get my host name */
    gethostname(myhostname,100);
    //struct hostent*myhostent = gethostbyname(myhostname);

    // get all host IP addresses
    std::vector<std::string> ipList = Socket::getLocalIPAddressList();

    // Subnet, IP Address
    std::vector<std::string> ipSubnetMaskList;
	for(unsigned int idx = 0; idx < ipList.size() && idx < MAX_NIC_COUNT; idx++) {
		string broadCastAddress = getNetworkInterfaceBroadcastAddress(ipList[idx]);
		//printf("idx = %d broadCastAddress [%s]\n",idx,broadCastAddress.c_str());

		//strcpy(subnetmask[idx], broadCastAddress.c_str());
		if(broadCastAddress != "" && std::find(ipSubnetMaskList.begin(),ipSubnetMaskList.end(),broadCastAddress) == ipSubnetMaskList.end()) {
			ipSubnetMaskList.push_back(broadCastAddress);
		}
	}

	port = htons( Socket::getBroadCastPort() );

	//for(unsigned int idx = 0; idx < ipList.size() && idx < MAX_NIC_COUNT; idx++) {
	for(unsigned int idx = 0; idx < ipSubnetMaskList.size(); idx++) {
		// Create the broadcast socket
		memset( &bcLocal[idx], 0, sizeof( struct sockaddr_in));
		bcLocal[idx].sin_family			= AF_INET;
		bcLocal[idx].sin_addr.s_addr	= inet_addr(ipSubnetMaskList[idx].c_str()); //htonl( INADDR_BROADCAST );
		bcLocal[idx].sin_port			= port;  // We are letting the OS fill in the port number for the local machine.
#ifdef WIN32
		bcfd[idx] = INVALID_SOCKET;
#else
		bcfd[idx] = INVALID_SOCKET;
#endif
		//if(strlen(subnetmask[idx]) > 0) {
		bcfd[idx] = socket( AF_INET, SOCK_DGRAM, 0 );

		if( bcfd[idx] <= 0  ) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Unable to allocate broadcast socket [%s]: %s\n", ipSubnetMaskList[idx].c_str(), getLastSocketErrorFormattedText().c_str());
			//exit(-1);
		}
		// Mark the socket for broadcast.
		else if( setsockopt( bcfd[idx], SOL_SOCKET, SO_BROADCAST, (const char *) &one, sizeof( int ) ) < 0 ) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Could not set socket to broadcast [%s]: %s\n", ipSubnetMaskList[idx].c_str(), getLastSocketErrorFormattedText().c_str());
			//exit(-1);
		}
		//}

		if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] setting up broadcast on address [%s]\n",__FILE__,__FUNCTION__,__LINE__,ipSubnetMaskList[idx].c_str());
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);

	time_t elapsed = 0;
	for( pn = 1; getQuitStatus() == false; pn++ ) {
		for(unsigned int idx = 0; getQuitStatus() == false && idx < ipSubnetMaskList.size(); idx++) {
			if( Socket::isSocketValid(&bcfd[idx]) == true ) {
				try {
					// Send this machine's host name and address in hostname:n.n.n.n format
					snprintf(buff,1024,"%s",myhostname);
					for(unsigned int idx1 = 0; idx1 < ipList.size(); idx1++) {
						strcat(buff,":");
						strcat(buff,ipList[idx1].c_str());
						strcat(buff,":");
						strcat(buff,intToStr(this->boundPort).c_str());
					}

					if(difftime((long int)time(NULL),elapsed) >= 1 && getQuitStatus() == false) {
						elapsed = time(NULL);

						bool pauseBroadCast = getPauseBroadcast();
						if(pauseBroadCast == false) {
							// Broadcast the packet to the subnet
							//if( sendto( bcfd, buff, sizeof(buff) + 1, 0 , (struct sockaddr *)&bcaddr, sizeof(struct sockaddr_in) ) != sizeof(buff) + 1 )

							if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Server is sending broadcast data [%s]\n",buff);

							ssize_t send_res = sendto( bcfd[idx], buff, buffMaxSize, 0 , (struct sockaddr *)&bcLocal[idx], sizeof(struct sockaddr_in) );
							if( send_res != buffMaxSize ) {
								if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Sendto error: %s\n", getLastSocketErrorFormattedText().c_str());
							}
							else {
								if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"Broadcasting on port [%d] the message: [%s]\n",Socket::getBroadCastPort(),buff);
							}
						}
						//printf("Broadcasting server send_res = %d buff [%s] ip [%s] getPauseBroadcast() = %d\n",send_res,buff,ipSubnetMaskList[idx].c_str(),pauseBroadCast);

						if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
					}

					if(getQuitStatus() == true) {
						if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
						break;
					}
					sleep(100); // send out broadcast every 1 seconds
				}
				catch(const exception &ex) {
					SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] error [%s]\n",__FILE__,__FUNCTION__,__LINE__,ex.what());
					this->setQuitStatus(true);
				}
				catch(...) {
					SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] UNKNOWN Error\n",__FILE__,__FUNCTION__,__LINE__);
					if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] unknown error\n",__FILE__,__FUNCTION__,__LINE__);
					this->setQuitStatus(true);
				}
			}

			if(getQuitStatus() == true) {
				if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
				break;
			}
		}
		if(getQuitStatus() == true) {
			if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d]\n",__FILE__,__FUNCTION__,__LINE__);
			break;
		}
	}

	for(unsigned int idx = 0; idx < ipSubnetMaskList.size(); idx++) {
		if( Socket::isSocketValid(&bcfd[idx]) == true ) {
#ifndef WIN32
        ::close(bcfd[idx]);
        bcfd[idx] = INVALID_SOCKET;
#else
        ::closesocket(bcfd[idx]);
        bcfd[idx] = INVALID_SOCKET;
#endif
		}
	}

	if(SystemFlags::getSystemSettingType(SystemFlags::debugNetwork).enabled) SystemFlags::OutputDebug(SystemFlags::debugNetwork,"In [%s::%s Line: %d] Broadcast thread is exiting\n",__FILE__,__FUNCTION__,__LINE__);
}

double Socket::getAveragePingMS(std::string host, int pingCount) {
	double result = -1;
	return result;

/*
	const bool debugPingOutput = false;
	char szCmd[1024]="";
#ifdef WIN32
	sprintf(szCmd,"ping -n %d %s",pingCount,host.c_str());
	if(debugPingOutput) printf("WIN32 is defined\n");
#elif defined(__GNUC__)
	sprintf(szCmd,"ping -c %d %s",pingCount,host.c_str());
	if(debugPingOutput) printf("NON WIN32 is defined\n");
#else
	#error "Your compiler needs to support popen!"
#endif
	if(szCmd[0] != '\0') {
		FILE *ping= NULL;
#ifdef WIN32
		ping= _popen(szCmd, "r");
		if(debugPingOutput) printf("WIN32 style _popen\n");
#elif defined(__GNUC__)
		ping= popen(szCmd, "r");
		if(debugPingOutput) printf("POSIX style _popen\n");
#else
	#error "Your compiler needs to support popen!"
#endif
		if (ping != NULL){
			char buf[4000]="";
			int bufferPos = 0;
			for(;feof(ping) == false;) {
				char *data = fgets(&buf[bufferPos], 256, ping);
				bufferPos = strlen(buf);
			}
#ifdef WIN32
			_pclose(ping);
#elif defined(__GNUC__)
			pclose(ping);
#else
	#error "Your compiler needs to support popen!"

#endif

			if(debugPingOutput) printf("Running cmd [%s] got [%s]\n",szCmd,buf);

			// Linux
			//softcoder@softhauslinux:~/Code/megaglest/trunk/mk/linux$ ping -c 5 soft-haus.com
			//PING soft-haus.com (65.254.250.110) 56(84) bytes of data.
			//64 bytes from 65-254-250-110.yourhostingaccount.com (65.254.250.110): icmp_seq=1 ttl=242 time=133 ms
			//64 bytes from 65-254-250-110.yourhostingaccount.com (65.254.250.110): icmp_seq=2 ttl=242 time=137 ms
			//
			// Windows XP
			//C:\Code\megaglest\trunk\data\glest_game>ping -n 5 soft-haus.com
			//
			//Pinging soft-haus.com [65.254.250.110] with 32 bytes of data:
			//
			//Reply from 65.254.250.110: bytes=32 time=125ms TTL=242
			//Reply from 65.254.250.110: bytes=32 time=129ms TTL=242

			std::string str = buf;
			std::string::size_type ms_pos = 0;
			int count = 0;
			while ( ms_pos != std::string::npos) {
				ms_pos = str.find("time=", ms_pos);

				if(debugPingOutput) printf("count = %d ms_pos = %d\n",count,ms_pos);

				if ( ms_pos != std::string::npos ) {
					++count;

					int endPos = str.find(" ms", ms_pos+5 );

					if(debugPingOutput) printf("count = %d endPos = %d\n",count,endPos);

					if(endPos == std::string::npos) {
						endPos = str.find("ms ", ms_pos+5 );

						if(debugPingOutput) printf("count = %d endPos = %d\n",count,endPos);
					}

					if(endPos != std::string::npos) {

						if(count == 1) {
							result = 0;
						}
						int startPos = ms_pos + 5;
						int posLength = endPos - startPos;
						if(debugPingOutput) printf("count = %d startPos = %d posLength = %d str = [%s]\n",count,startPos,posLength,str.substr(startPos, posLength).c_str());

						float pingMS = strToFloat(str.substr(startPos, posLength));
						result += pingMS;
					}

					ms_pos += 5; // start next search after this "time="
				}
			}

			if(result > 0 && count > 1) {
				result /= count;
			}
		}
	}
	return result;
*/
}

std::string Socket::getIpAddress() {
	return ipAddress;
}

void ServerSocket::addIPAddressToBlockedList(string value) {
	if(isIPAddressBlocked(value) == false) {
		blockIPList.push_back(value);
		if(SystemFlags::getSystemSettingType(SystemFlags::debugError).enabled) SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Blocked IP Address [%s].\n",extractFileFromDirectoryPath(__FILE__).c_str(),__FUNCTION__,__LINE__,value.c_str());
	}
}
bool ServerSocket::isIPAddressBlocked(string value) const {
	bool result = (std::find(blockIPList.begin(),blockIPList.end(),value) != blockIPList.end());
	return result;
}

void ServerSocket::removeBlockedIPAddress(string value) {
	vector<string>::iterator iterFind = std::find(blockIPList.begin(),blockIPList.end(),value);
	if(iterFind != blockIPList.end()) {
		blockIPList.erase(iterFind);
	}
}

void ServerSocket::clearBlockedIPAddress() {
	blockIPList.clear();
}

bool ServerSocket::hasBlockedIPAddresses() const {
	return(blockIPList.size() > 0);
}

}}//end namespace
