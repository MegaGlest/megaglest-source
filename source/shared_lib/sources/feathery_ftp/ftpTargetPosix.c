/* FEATHERY FTP-Server
 * Copyright (C) 2005-2010 Andreas Martin (andreas.martin@linuxmail.org)
 * <https://sourceforge.net/projects/feathery>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef WIN32

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <grp.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <dirent.h>
#include <arpa/inet.h>

#include "ftpTypes.h"
#include "ftpConfig.h"
#include "ftp.h"

ip_t ownIp;

LOCAL fd_set watchedSockets;
LOCAL fd_set signaledSockets;
LOCAL int maxSockNr;


void ftpArchInit()
{
	ownIp = 0;
	maxSockNr = 0;
	FD_ZERO(&watchedSockets);
	FD_ZERO(&signaledSockets);
}

void ftpArchCleanup(void)
{
}

int ftpGetLocalTime(ftpTime_S *t)
{
	struct tm *pTime;
	time_t tim;

	time(&tim);
	pTime = localtime(&tim);
	if(pTime)
	{
		t->year   = (uint16_t)pTime->tm_year + 1900;
		t->month  = (uint8_t)pTime->tm_mon + 1;
		t->day    = (uint8_t)pTime->tm_mday;
		t->hour   = (uint8_t)pTime->tm_hour;
		t->minute = (uint8_t)pTime->tm_min;
		t->second = (uint8_t)pTime->tm_sec;

		return 0;
	}

	return -1;
}

void* ftpOpenDir(const char* path)
{
	return opendir(path);
}

const char* ftpReadDir(void* dirHandle)
{
	struct dirent* dirEntry;

	dirEntry = readdir(dirHandle);
	if(dirEntry)
		return dirEntry->d_name;
	else
		return NULL;
}

int ftpCloseDir(void* dirHandle)
{
	return closedir(dirHandle);
}

int ftpStat(const char* path, ftpPathInfo_S *info)
{
	struct stat fileInfo = {0};
	struct tm *pTime;
	struct passwd *pw;
	struct group *gr;

	if(!stat(path, &fileInfo))
	{
		if(S_ISREG(fileInfo.st_mode))
			info->type = TYPE_FILE;
		else if(S_ISDIR(fileInfo.st_mode))
			info->type = TYPE_DIR;
		else
			info->type = TYPE_LINK;

		pTime = localtime(&fileInfo.st_mtime);
		if(pTime)
		{
			info->mTime.year   = (uint16_t)pTime->tm_year + 1900;
			info->mTime.month  = (uint8_t)pTime->tm_mon + 1;
			info->mTime.day    = (uint8_t)pTime->tm_mday;
			info->mTime.hour   = (uint8_t)pTime->tm_hour;
			info->mTime.minute = (uint8_t)pTime->tm_min;
			info->mTime.second = (uint8_t)pTime->tm_sec;
		}
		pTime = localtime(&fileInfo.st_ctime);
		if(pTime)
		{
			info->cTime.year   = (uint16_t)pTime->tm_year + 1900;
			info->cTime.month  = (uint8_t)pTime->tm_mon + 1;
			info->cTime.day    = (uint8_t)pTime->tm_mday;
			info->cTime.hour   = (uint8_t)pTime->tm_hour;
			info->cTime.minute = (uint8_t)pTime->tm_min;
			info->cTime.second = (uint8_t)pTime->tm_sec;
		}
		pTime = localtime(&fileInfo.st_atime);
		if(pTime)
		{
			info->aTime.year   = (uint16_t)pTime->tm_year + 1900;
			info->aTime.month  = (uint8_t)pTime->tm_mon + 1;
			info->aTime.day    = (uint8_t)pTime->tm_mday;
			info->aTime.hour   = (uint8_t)pTime->tm_hour;
			info->aTime.minute = (uint8_t)pTime->tm_min;
			info->aTime.second = (uint8_t)pTime->tm_sec;
		}

		info->size  = (uint32_t)fileInfo.st_size;
		info->links = (int)fileInfo.st_nlink;

		pw = getpwuid(fileInfo.st_uid);
		if(pw)
			strncpy(info->user, pw->pw_name, sizeof(info->user));
		else
			sprintf(info->user, "%04d", fileInfo.st_uid);

		gr = getgrgid(fileInfo.st_gid);
		if(gr)
			strncpy(info->group, gr->gr_name, sizeof(info->group));
		else
			sprintf(info->group, "%04d", fileInfo.st_gid);

		return 0;
	}

	return -1;
}

int ftpMakeDir(const char* path)
{
	return mkdir(path, S_IRUSR | S_IWUSR);
}

int ftpRemoveDir(const char* path)
{
	return rmdir(path);
}

int ftpCloseSocket(socket_t s)
{
    if(VERBOSE_MODE_ENABLED) printf("\nClosing socket: %d\n",s);
	return close(s);
}

int ftpSend(socket_t s, const void *data, int len)
{
	int currLen = 0;

	do
	{
		currLen = send(s, data, len, 0);
		if(currLen >= 0)
		{
			len -= currLen;
			data = (uint8_t*)data + currLen;
		}
		else
			return -1;

	}while(len > 0);

	return 0;
}

int ftpReceive(socket_t s, void *data, int len)
{
	return recv(s, data, len, 0);
}

socket_t ftpEstablishDataConnection(int passive, ip_t *ip, port_t *port, int sessionId)
{
	socket_t dataSocket;
	struct sockaddr_in clientAddr;
	struct sockaddr_in myAddr;
	int on = 1;
	unsigned len;
	dataSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(dataSocket < 0)
		return -1;

	if(!passive)
	{
		if(setsockopt(dataSocket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)))
		{
			close(dataSocket);
			return -1;
		}
		myAddr.sin_family      = AF_INET;
		myAddr.sin_addr.s_addr = INADDR_ANY;
		myAddr.sin_port        = htons(20);
		if(bind(dataSocket, (struct sockaddr *)&myAddr, sizeof(myAddr)))
		{
			close(dataSocket);
			return -1;
		}
		clientAddr.sin_family      = AF_INET;
		clientAddr.sin_addr.s_addr = htonl(*ip);
		clientAddr.sin_port        = htons(*port);
		if(connect(dataSocket, (struct sockaddr *)&clientAddr, sizeof(clientAddr)))
		{
			close(dataSocket);
			return -1;
		}
	}
	else
	{
	    int passivePort = ftpGetPassivePort() + sessionId;
	    if(VERBOSE_MODE_ENABLED) printf("\nPASSIVE CONNECTION for sessionId = %d using port #: %d\n",sessionId,passivePort);

		myAddr.sin_family = AF_INET;
		myAddr.sin_addr.s_addr = INADDR_ANY;
		myAddr.sin_port = htons(passivePort);
		//myAddr.sin_port = htons(ftpGetPassivePort() + sessionId);

        setsockopt(dataSocket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

		if(bind(dataSocket, (struct sockaddr *)&myAddr, sizeof(myAddr)))
	    {
			if(VERBOSE_MODE_ENABLED) printf("\nPASSIVE CONNECTION for sessionId = %d using port #: %d FAILED: %d\n",sessionId,passivePort,dataSocket);

			close(dataSocket);
			return -1;
	    }

	    if(VERBOSE_MODE_ENABLED) printf("\nPASSIVE CONNECTION for sessionId = %d using port #: %d bound ok\n",sessionId,passivePort);

	    len = sizeof(myAddr);
		if(getsockname(dataSocket, (struct sockaddr *)&myAddr, &len))             // Port des Server-Sockets ermitteln
	    {
			close(dataSocket);
	        return -1;
	    }

		*port = ntohs(myAddr.sin_port);
		*ip   = ownIp;

        if(VERBOSE_MODE_ENABLED) printf("\nPASSIVE CONNECTION for sessionId = %d using port #: %d about to listen on port: %d using listener socket: %d\n",sessionId,passivePort,*port,dataSocket);

	    if(listen(dataSocket, 1))
	    {
	        if(VERBOSE_MODE_ENABLED) printf("\nPASSIVE CONNECTION for sessionId = %d using port #: %d FAILED #2: %d\n",sessionId,passivePort,dataSocket);

			close(dataSocket);
	        return -1;
	    }

   		//*port = ftpGetPassivePort();
		//*ip   = ownIp;
        //dataSocket = ftpGetServerPassivePortListenSocket();
	}
	return dataSocket;
}

socket_t ftpAcceptDataConnection(socket_t listner)
{
	struct sockaddr_in clientinfo;
    unsigned len;
    socket_t dataSocket;

    len = sizeof(clientinfo);

	dataSocket = accept(listner, (struct sockaddr *)&clientinfo, &len);
	if(dataSocket < 0)
	{
		dataSocket = -1;
	}

	close(listner); // Server-Socket wird nicht mehr gebrauch deshalb schließen

    ip_t remoteIP = ntohl(clientinfo.sin_addr.s_addr);
    if(ftpIsValidClient && ftpIsValidClient(remoteIP) == 0)
    {
if(VERBOSE_MODE_ENABLED) printf("Connection with %s is NOT a valid trusted client, dropping connection.\n", inet_ntoa(clientinfo.sin_addr));

        close(dataSocket);
        dataSocket = -1;
    }

	return dataSocket;
}

socket_t ftpCreateServerSocket(int portNumber)
{
	int	theServer;
	struct sockaddr_in serverinfo;
	unsigned len;
	int val = 1;

	theServer = socket(AF_INET, SOCK_STREAM, 0);
	if(theServer < 0)
		return -1;

	serverinfo.sin_family = AF_INET;
	serverinfo.sin_addr.s_addr = INADDR_ANY;
	serverinfo.sin_port = htons(portNumber);
	len = sizeof(serverinfo);

	setsockopt(theServer, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

	if(bind(theServer, (struct sockaddr *)&serverinfo, len))
	{
		ftpCloseSocket(theServer);
		return -2;
	}

	if(listen(theServer, 16))
	{
		ftpCloseSocket(theServer);
		return -3;
	}

	return (socket_t)theServer;
}

socket_t ftpAcceptServerConnection(socket_t server, ip_t *remoteIP, port_t *remotePort)
{
	struct sockaddr_in sockinfo;
	socket_t clientSocket;
	unsigned len;

	len = sizeof(sockinfo);

	clientSocket = accept(server, (struct sockaddr *)&sockinfo, &len);
	*remoteIP    = ntohl(sockinfo.sin_addr.s_addr);
	*remotePort  = ntohs(sockinfo.sin_port);

	if(!ownIp)	// kennen wir schon die eigene IP?
	{
		len = sizeof(sockinfo);
		if(getsockname(clientSocket, (struct sockaddr *)&sockinfo, &len))
		{
if(VERBOSE_MODE_ENABLED) printf("getsockname error\n");
		}

		ownIp = ntohl(sockinfo.sin_addr.s_addr);        // eigene IP-Adresse abspeichern (wird dir PASV benätigt)
	}

if(VERBOSE_MODE_ENABLED) printf("Connection with %s on Port %d accepted.\n", inet_ntoa(sockinfo.sin_addr), *remotePort);

    if(ftpIsValidClient && ftpIsValidClient(*remoteIP) == 0)
    {
if(VERBOSE_MODE_ENABLED) printf("Connection with %s on Port %d is NOT a valid trusted client, dropping connection.\n", inet_ntoa(sockinfo.sin_addr), *remotePort);

        close(clientSocket);
        clientSocket = -1;
    }

	return clientSocket;
}

int ftpTrackSocket(socket_t s)
{
	if(s > maxSockNr)
		maxSockNr = s;
	FD_SET(s, &watchedSockets);
	return 0;
}

int ftpUntrackSocket(socket_t s)
{
	FD_CLR(s, &watchedSockets);
	// TODO hier sollte eine Möglichkeit geschaffen werden um maxSockNr anzupassen
	return 0;
}

int ftpTestSocket(socket_t s)
{
	return FD_ISSET(s, &signaledSockets);
}

int ftpSelect(int poll)
{
	signaledSockets = watchedSockets;

	if(poll)
	{
		struct timeval t = {0};
		return select(maxSockNr+1, &signaledSockets, NULL, NULL, &t);
	}
	else
		return select(maxSockNr+1, &signaledSockets, NULL, NULL, NULL);
}

#endif
