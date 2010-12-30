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
/*
DECLARATIVE SPECIFICATIONS

   5.1.  MINIMUM IMPLEMENTATION

      In order to make FTP workable without needless error messages, the
      following minimum implementation is required for all servers:

         TYPE - ASCII Non-print
         MODE - Stream
         STRUCTURE - File, Record
         COMMANDS - USER, QUIT, PORT,
                    TYPE, MODE, STRU,
                      for the default values
                    RETR, STOR,
                    NOOP.

      The default values for transfer parameters are:

         TYPE - ASCII Non-print
         MODE - Stream
         STRU - File

      All hosts must accept the above as the standard defaults.
      */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "ftpTypes.h"
#include "ftpConfig.h"
#include "ftp.h"
#include "ftpIfc.h"
#include "ftpMessages.h"


LOCAL uint8_t scratchBuf[LEN_SCRATCHBUF];

int ftpSendMsg(msgmode_E mode, int sessionId, int ret, const char* msg)
{
	int sentlen = 0;
	int len = strlen(msg);
	char buf[6];

	if(mode == MSG_QUOTE)
	{
		sprintf((char*)buf, "%03d \"", ret);
		sentlen += ftpSend(ftpGetSession(sessionId)->ctrlSocket, buf, 5);
		sentlen += ftpSend(ftpGetSession(sessionId)->ctrlSocket, msg, len);
		sentlen += ftpSend(ftpGetSession(sessionId)->ctrlSocket, "\"\r\n", 3);
	}
	else
	{
		sprintf((char*)buf, "%03d ", ret);
		sentlen += ftpSend(ftpGetSession(sessionId)->ctrlSocket, buf, 4);
		sentlen += ftpSend(ftpGetSession(sessionId)->ctrlSocket, msg, len);
		sentlen += ftpSend(ftpGetSession(sessionId)->ctrlSocket, "\r\n", 2);
	}

	#if DBG_LOG
	printf("%02d <-- %s%s\n", sessionId, buf, msg);
	#endif

	return	sentlen;
}

int ftpExecTransmission(int sessionId)
{
	int finished = FALSE;
	int len;
    ftpSession_S *pSession = ftpGetSession(sessionId);
	transmission_S *pTrans = &pSession->activeTrans;
	int rxLen;


    pSession->timeLastCmd = ftpGetUnixTime();
	switch(pTrans->op)
	{
	case OP_RETR:
        len = ftpReadFile(scratchBuf, 1, LEN_SCRATCHBUF, pTrans->fsHandle);
		if(len > 0)
        {
			pTrans->fileSize -= len;
            if(ftpSend(pTrans->dataSocket, scratchBuf, len))
			{
    	    	ftpSendMsg(MSG_NORMAL, sessionId, 426, ftpMsg000);
    	    	finished = TRUE;
			}
		}
		else
		{
	    	ftpSendMsg(MSG_NORMAL, sessionId, 451, ftpMsg001);
	    	finished = TRUE;
		}

	    if(pTrans->fileSize == 0)
	    {
	    	ftpSendMsg(MSG_NORMAL, sessionId, 226, ftpMsg002);
	    	finished = TRUE;
	    }
		break;
	case OP_STOR:
		rxLen = 0;
		do
		{
        	len = ftpReceive(pTrans->dataSocket, &scratchBuf[rxLen], LEN_SCRATCHBUF - rxLen);

			if(len <= 0)
				break;

			rxLen += len;
		}while(rxLen < LEN_SCRATCHBUF);

		if(rxLen > 0)
        {
	        if(ftpWriteFile(scratchBuf, 1, rxLen, pTrans->fsHandle) != rxLen)
			{
		    	ftpSendMsg(MSG_NORMAL, sessionId, 451, ftpMsg001);
    	    	finished = TRUE;
			}
		}

		if(len <= 0)
		{
	    	ftpSendMsg(MSG_NORMAL, sessionId, 226, ftpMsg003);
	    	finished = TRUE;
		}

		break;
	case OP_LIST:
		break;

	default:
		return -1;
	}

	if(finished)
	{
		ftpCloseTransmission(sessionId);
		return 1;
	}
	else
		return 0;
}

LOCAL int ftpCmdUser(int sessionId, const char* args, int len)
{
	ftpSendMsg(MSG_NORMAL, sessionId, 331, ftpMsg004);
	ftpGetSession(sessionId)->userId = ftpFindAccount(args);
	return 0;
}
LOCAL int ftpCmdPass(int sessionId, const char* args, int len)
{
	if(!ftpCheckPassword(ftpGetSession(sessionId)->userId, args))
	{
		ftpAuthSession(sessionId);
		ftpSendMsg(MSG_NORMAL, sessionId, 230, ftpMsg005);
	}
	else
	{
		ftpGetSession(sessionId)->userId = 0;
		ftpSendMsg(MSG_NORMAL, sessionId, 530, ftpMsg006);
	}
	return 0;
}
LOCAL int ftpCmdSyst(int sessionId, const char* args, int len)
{
	ftpSendMsg(MSG_NORMAL, sessionId, 215, "UNIX Type: L8");

	return 0;
}
LOCAL int ftpCmdPort(int sessionId, const char* args, int len)
{
	char clientIp[16];
	uint16_t clientPort;

	int commaCnt = 0;
	int n;
	char* p;

	for(n = 0; args[n] != '\0'; n++)
	{
		if(commaCnt <= 3)	// Ip-Adresse
		{
			if(args[n] == ',')
			{
				commaCnt++;
				if(commaCnt < 4)
					clientIp[n] = '.';
				else
					clientIp[n] = '\0';
			}
			else
				clientIp[n] = args[n];
		}
		else				// Port-Nummer
		{
			p = (char*)&args[n];
			clientPort = (uint16_t)(strtoul(p, &p, 0) << 8);
			p++;
			clientPort |= strtoul(p, &p, 0);
			break;
		}
	}
	// TODO Derzeit wird die übergebene IP nicht beachtet, sondern nur die IP des Control-Sockets verwendet
	clientIp[0] = clientIp[0];
	if(ftpGetSession(sessionId)->passiveDataSocket >= 0)
	{
		ftpCloseSocket(ftpGetSession(sessionId)->passiveDataSocket);
		ftpGetSession(sessionId)->passiveDataSocket = -1;
	}
	ftpGetSession(sessionId)->remoteDataPort = clientPort;
    ftpGetSession(sessionId)->passive = FALSE;
	ftpSendMsg(MSG_NORMAL, sessionId, 200, ftpMsg007);

	return 0;
}
LOCAL int ftpCmdNoop(int sessionId, const char* args, int len)
{
	ftpSendMsg(MSG_NORMAL, sessionId, 200, ftpMsg008);

	return 0;
}
LOCAL int ftpCmdQuit(int sessionId, const char* args, int len)
{
	ftpSendMsg(MSG_NORMAL, sessionId, 221, ftpMsg009);
	return -1;

}
LOCAL int ftpCmdAbor(int sessionId, const char* args, int len)
{
	ftpCloseTransmission(sessionId);

	ftpSendMsg(MSG_NORMAL, sessionId, 226, ftpMsg040);
	return 0;

}
#define ALL		0x80
#define LIST	1
#define NLST	2
#define MLST	4
#define MLSD	8
LOCAL int sendListing(socket_t dataSocket, int sessionId, const char* path, int format)
{
	int haveAnySuccessfulFiles = 0;
	void *dir;
    const char monName[12][4] = {
            "Jan", "Feb", "Mar", "Apr", "May", "Jun",
            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
        };

	dir = ftpOpenDir(path);
	if(dir)
	{
		const char* dirEntry;
		int len;
		int err = 0;
    	ftpTime_S currTime = {0};
		ftpPathInfo_S fileInfo;

    	ftpGetLocalTime(&currTime);
		ftpSendMsg(MSG_NORMAL, sessionId, 150, ftpMsg010);

#if DBG_LOG
		printf("about to read dir contents [%s]\n", path);
#endif

        haveAnySuccessfulFiles = 0;
		while((dirEntry = ftpReadDir(dir)) != NULL)
		{
		    const char * realPath = ftpGetRealPath(sessionId, dirEntry, FALSE);
		    int statResult = ftpStat(realPath, &fileInfo);
#if DBG_LOG
		printf("ftpGetRealPath() returned [%s] stat() = %d\n", realPath, statResult);
#endif

			if(statResult == 0)
			{
				if((format & ALL) == 0)
				{
					if(dirEntry[0] == '.')
						continue;
				}
				if(format & LIST)
				{
					switch(fileInfo.type)
					{
					default:
					case TYPE_FILE:
						scratchBuf[0] = '-';
						break;
					case TYPE_DIR:
						scratchBuf[0] = 'd';
						break;
					case TYPE_LINK:
						scratchBuf[0] = 'l';
						break;
					}

					if(currTime.year == fileInfo.mTime.year)
					{
						len = sprintf((char*)&scratchBuf[1], "rwxrwxrwx %4u %-8s %-8s %8u %s %02d %02d:%02d %s\r\n",
								fileInfo.links,
								fileInfo.user,
								fileInfo.group,
								fileInfo.size,
								monName[fileInfo.mTime.month - 1],
								fileInfo.mTime.day,
								fileInfo.mTime.hour,
								fileInfo.mTime.minute,
								dirEntry);
					}
					else
					{
						len = sprintf((char*)&scratchBuf[1], "rwxrwxrwx %4u %-8s %-8s %8u %s %02d %5d %s\r\n",
								fileInfo.links,
								fileInfo.user,
								fileInfo.group,
								fileInfo.size,
								monName[fileInfo.mTime.month - 1],
								fileInfo.mTime.day,
								fileInfo.mTime.year,
								dirEntry);
					}
					ftpSend(dataSocket, scratchBuf, len + 1);
					haveAnySuccessfulFiles = 1;
				}
				else if(format & NLST)
				{
					len = sprintf((char*)scratchBuf, "%s\r\n", dirEntry);
					ftpSend(dataSocket, scratchBuf, len);
					haveAnySuccessfulFiles = 1;
				}
				else if(format & MLSD)
				{
					if(!strcmp("..", dirEntry))
						len = sprintf((char*)scratchBuf, "Type=pdir");
					else
					{
						switch(fileInfo.type)
						{
						default:
						case TYPE_FILE:
							len = sprintf((char*)scratchBuf, "Type=file");
							break;
						case TYPE_DIR:
							len = sprintf((char*)scratchBuf, "Type=dir");
							break;
						case TYPE_LINK:
							len = sprintf((char*)scratchBuf, "Type=OS.unix=slink");
							break;
						}
					}
					ftpSend(dataSocket, scratchBuf, len);
					len = sprintf((char*)scratchBuf, ";Size=%u;Modify=%04d%02d%02d%02d%02d%02d;Perm=r; %s\r\n",
							fileInfo.size,
							fileInfo.mTime.year,
							fileInfo.mTime.month,
							fileInfo.mTime.day,
							fileInfo.mTime.hour,
							fileInfo.mTime.minute,
							fileInfo.mTime.second,
							dirEntry);
					ftpSend(dataSocket, scratchBuf, len);
					haveAnySuccessfulFiles = 1;
				}
			}
			else
			{
				err = 1;
				//break;
			}
		}

		ftpCloseDir(dir);
		if(err && haveAnySuccessfulFiles == 0)
			ftpSendMsg(MSG_NORMAL, sessionId, 451, ftpMsg039);
		else
			ftpSendMsg(MSG_NORMAL, sessionId, 226, ftpMsg013);
	}
	else
	{
#if DBG_LOG
		printf("opendir [%s] returned errno: %#x\n", path,errno);
#endif

		ftpSendMsg(MSG_NORMAL, sessionId, 451, ftpMsg038);
	}

	return 0;
}

LOCAL int ftpCmdList(int sessionId, const char* args, int len)
{
	const char* realPath;
	socket_t s;
//#### Funktioniert nicht wenn Pfad übergeben wird
/*	if(args[0] != '\0')
	{
		if(args[0] != '-')
			realPath = ftpGetRealPath(sessionId, args);
	}
	else*/
		realPath = ftpGetRealPath(sessionId, ftpGetSession(sessionId)->workingDir, TRUE);

	if(ftpGetSession(sessionId)->passive == FALSE)
    {
		s = ftpEstablishDataConnection(FALSE, &ftpGetSession(sessionId)->remoteIp, &ftpGetSession(sessionId)->remoteDataPort);
        if(s < 0)
        {
        	ftpSendMsg(MSG_NORMAL, sessionId, 425, ftpMsg011);
            return 1;
        }
    }
    else
    {
    	s = ftpAcceptDataConnection(ftpGetSession(sessionId)->passiveDataSocket);
        if(s < 0)
        {
        	ftpSendMsg(MSG_NORMAL, sessionId, 425, ftpMsg012);
            return 1;
        }
    }
	sendListing(s, sessionId, realPath, LIST);
    ftpCloseSocket(s);

	return 0;
}

LOCAL int ftpCmdNlst(int sessionId, const char* args, int len)
{
	//#### -List kann auch Argumente haben
	const char* realPath;
	socket_t s;

/*	if(pfad übergeben)
		realPath = ftpGetRealPath(sessionId, args);
	else*/
		realPath = ftpGetRealPath(sessionId, ftpGetSession(sessionId)->workingDir, TRUE);

	if(ftpGetSession(sessionId)->passive == FALSE)
    {
		s = ftpEstablishDataConnection(FALSE, &ftpGetSession(sessionId)->remoteIp, &ftpGetSession(sessionId)->remoteDataPort);
        if(s < 0)
        {
        	ftpSendMsg(MSG_NORMAL, sessionId, 425, ftpMsg011);
            return 1;
        }
    }
    else
    {
    	s = ftpAcceptDataConnection(ftpGetSession(sessionId)->passiveDataSocket);
        if(s < 0)
        {
        	ftpSendMsg(MSG_NORMAL, sessionId, 425, ftpMsg012);
            return 1;
        }
    }
	sendListing(s, sessionId, realPath, NLST);
    ftpCloseSocket(s);

	return 0;
}

LOCAL int ftpCmdRetr(int sessionId, const char* args, int len)
{
	ftpPathInfo_S fileInfo;
    const char* realPath = ftpGetRealPath(sessionId, args, TRUE);
	socket_t s;
	void *fp;
	int statResult = 0;

#if DBG_LOG
	printf("ftpCmdRetr args [%s] realPath [%s]\n", args, realPath);
#endif

    statResult = ftpStat(realPath, &fileInfo);
#if DBG_LOG
	printf("stat() = %d fileInfo.type = %d\n", statResult,fileInfo.type);
#endif

   	if(statResult || (fileInfo.type != TYPE_FILE)) // file accessible?
    {
    	ftpSendMsg(MSG_NORMAL, sessionId, 550, ftpMsg032);
        return 2;
    }

	if(ftpGetSession(sessionId)->passive == FALSE)
    {
		s = ftpEstablishDataConnection(FALSE, &ftpGetSession(sessionId)->remoteIp, &ftpGetSession(sessionId)->remoteDataPort);
        if(s < 0)
        {
        	ftpSendMsg(MSG_NORMAL, sessionId, 425, ftpMsg011);
            return 1;
        }
    }
    else
    {
    	s = ftpAcceptDataConnection(ftpGetSession(sessionId)->passiveDataSocket);
        if(s < 0)
        {
        	ftpSendMsg(MSG_NORMAL, sessionId, 425, ftpMsg012);
            return 1;
        }
    }

	ftpSendMsg(MSG_NORMAL, sessionId, 150, ftpMsg014);

	fp = ftpOpenFile(realPath, "rb");
	if(fp)
	{
		ftpOpenTransmission(sessionId, OP_RETR, fp, s, fileInfo.size);
		ftpExecTransmission(sessionId);
	}
	else
	{
		ftpSendMsg(MSG_NORMAL, sessionId, 451, ftpMsg015);
		ftpCloseSocket(s);
	}

	return 0;
}

LOCAL int ftpCmdStor(int sessionId, const char* args, int len)
{
	socket_t s;
    void* fp;
    const char* realPath = ftpGetRealPath(sessionId, args, TRUE);

	if(ftpGetSession(sessionId)->passive == FALSE)
    {
		s = ftpEstablishDataConnection(FALSE, &ftpGetSession(sessionId)->remoteIp, &ftpGetSession(sessionId)->remoteDataPort);
        if(s < 0)
        {
        	ftpSendMsg(MSG_NORMAL, sessionId, 425, ftpMsg011);
            return 1;
        }
    }
    else
    {
    	s = ftpAcceptDataConnection(ftpGetSession(sessionId)->passiveDataSocket);
        if(s < 0)
        {
        	ftpSendMsg(MSG_NORMAL, sessionId, 425, ftpMsg012);
            return 1;
        }
    }

	ftpSendMsg(MSG_NORMAL, sessionId, 150, ftpMsg016);

    fp = ftpOpenFile(realPath, "wb");
    if(fp)
    {
        ftpOpenTransmission(sessionId, OP_STOR, fp, s, 0);
		ftpExecTransmission(sessionId);
    }
    else
    {
		ftpSendMsg(MSG_NORMAL, sessionId, 451, ftpMsg015);
        ftpCloseSocket(s);
    }

	return 0;
}

LOCAL int ftpCmdDele(int sessionId, const char* args, int len)
{
    const char* realPath = ftpGetRealPath(sessionId, args, TRUE);
    if(ftpRemoveFile(realPath))
    	ftpSendMsg(MSG_NORMAL, sessionId, 450, ftpMsg018);
    else
    	ftpSendMsg(MSG_NORMAL, sessionId, 250, ftpMsg019);


	return 0;
}

LOCAL int ftpCmdMkd(int sessionId, const char* args, int len)
{
    const char* realPath = ftpGetRealPath(sessionId, args, TRUE);
    if(ftpMakeDir(realPath))
    	ftpSendMsg(MSG_NORMAL, sessionId, 550, ftpMsg020);
    else
    	ftpSendMsg(MSG_NORMAL, sessionId, 250, ftpMsg021);


	return 0;
}

LOCAL int ftpCmdRmd(int sessionId, const char* args, int len)
{
    const char* realPath = ftpGetRealPath(sessionId, args, TRUE);
    if(ftpRemoveDir(realPath))
    	ftpSendMsg(MSG_NORMAL, sessionId, 550, ftpMsg022);
    else
    	ftpSendMsg(MSG_NORMAL, sessionId, 250, ftpMsg023);


	return 0;
}

LOCAL int ftpCmdPwd(int sessionId, const char* args, int len)
{
	if(ftpGetSession(sessionId)->workingDir[0])
		ftpSendMsg(MSG_QUOTE, sessionId, 257, ftpGetSession(sessionId)->workingDir);
	else
		ftpSendMsg(MSG_QUOTE, sessionId, 257, "/");

	return 0;
}

LOCAL int ftpCmdCwd(int sessionId, const char* args, int len)
{
	if(ftpChangeDir(sessionId, args) == 0)
		ftpSendMsg(MSG_NORMAL, sessionId, 250, ftpMsg024);
	else
		ftpSendMsg(MSG_NORMAL, sessionId, 550, ftpMsg025);

	return 0;
}

LOCAL int ftpCmdType(int sessionId, const char* args, int len)
{
	switch(args[0])
	{
	case 'I':
	case 'i':
		ftpSendMsg(MSG_NORMAL, sessionId, 200, ftpMsg026);
        ftpGetSession(sessionId)->binary = TRUE;
		break;
	case 'A':
	case 'a':
		ftpSendMsg(MSG_NORMAL, sessionId, 200, ftpMsg027);
        ftpGetSession(sessionId)->binary = FALSE;
		break;
	default:
		ftpSendMsg(MSG_NORMAL, sessionId, 504, ftpMsg028);
	}

	return 0;
}

LOCAL int ftpCmdPasv(int sessionId, const char* args, int len)
{
	uint16_t port;
	uint32_t ip;
	char str[50];
	socket_t s;
	uint32_t remoteFTPServerIp;

	if(ftpGetSession(sessionId)->passiveDataSocket >= 0)
	{
		ftpCloseSocket(ftpGetSession(sessionId)->passiveDataSocket);
		ftpGetSession(sessionId)->passiveDataSocket = -1;
	}
	s = ftpEstablishDataConnection(TRUE, &ip, &port);
    if(s < 0)
    {
       	ftpSendMsg(MSG_NORMAL, sessionId, 425, ftpMsg012);
        return 1;
    }
    ftpGetSession(sessionId)->passiveDataSocket = s;

    if(ftpFindExternalFTPServerIp(ftpGetSession(sessionId)->remoteIp) > 0)
    {
        ftpGetSession(sessionId)->remoteFTPServerPassivePort = port;
        if(ftpAddUPNPPortForward) {
            ftpAddUPNPPortForward(port, port);
        }

        remoteFTPServerIp = ftpFindExternalFTPServerIp(ftpGetSession(sessionId)->remoteIp);

        sprintf(str, "%s (%d,%d,%d,%d,%d,%d)",
                ftpMsg029,
                (remoteFTPServerIp >> 24) & 0xFF,
                (remoteFTPServerIp >> 16) & 0xFF,
                (remoteFTPServerIp >> 8) & 0xFF,
                remoteFTPServerIp & 0xFF,
                (port >> 8) & 0xFF,
                port & 0xFF);
    }
    else
    {
        sprintf(str, "%s (%d,%d,%d,%d,%d,%d)",
                ftpMsg029,
                (ip >> 24) & 0xFF,
                (ip >> 16) & 0xFF,
                (ip >> 8) & 0xFF,
                ip & 0xFF,
                (port >> 8) & 0xFF,
                port & 0xFF);
    }

	ftpSendMsg(MSG_NORMAL, sessionId, 227, str);
    ftpGetSession(sessionId)->passive = TRUE;
	return 0;
}
LOCAL int ftpCmdCdup(int sessionId, const char* args, int len)
{
	return ftpCmdCwd(sessionId, "..", 2);
}
LOCAL int ftpCmdStru(int sessionId, const char* args, int len)
{
	switch(args[0])
	{
	case 'F':
	case 'f':
		ftpSendMsg(MSG_NORMAL, sessionId, 200, ftpMsg030);
		break;
	default:
		ftpSendMsg(MSG_NORMAL, sessionId, 504, ftpMsg031);
	}
	return 0;
}

#if RFC3659
LOCAL int ftpCmdSize(int sessionId, const char* args, int len)
{
    int ret;
    char str[12];
   	ftpPathInfo_S fileInfo;
    const char* realPath = ftpGetRealPath(sessionId, args, TRUE);

    ret = ftpStat(realPath, &fileInfo);
    if(ret)
    {
    	ftpSendMsg(MSG_NORMAL, sessionId, 550, ftpMsg032);
        return 2;
    }

    sprintf(str, "%d", fileInfo.size);
   	ftpSendMsg(MSG_NORMAL, sessionId, 213, str);


	return 0;
}

LOCAL int ftpCmdMdtm(int sessionId, const char* args, int len)
{
    int ret;
    char str[15];
   	ftpPathInfo_S fileInfo;
    const char* realPath = ftpGetRealPath(sessionId, args, TRUE);

    ret = ftpStat(realPath, &fileInfo);
    if(ret)
    {
    	ftpSendMsg(MSG_NORMAL, sessionId, 550, ftpMsg032);
        return 2;
    }

    sprintf(str, "%04d%02d%02d%02d%02d%02d", fileInfo.mTime.year,
											 fileInfo.mTime.month,
											 fileInfo.mTime.day,
											 fileInfo.mTime.hour,
											 fileInfo.mTime.minute,
											 fileInfo.mTime.second);
   	ftpSendMsg(MSG_NORMAL, sessionId, 213, str);


	return 0;
}

LOCAL int ftpCmdMlst(int sessionId, const char* args, int len)
{
   	ftpSendMsg(MSG_NORMAL, sessionId, 550, "MLST command not implemented");

	return 0;
}
/*LOCAL int ftpCmdMlsd(int sessionId, const char* args, int len)
{
   	ftpSendMsg(MSG_NORMAL, sessionId, 550, "MLSD command not implemented");

	return 0;
}*/
LOCAL int ftpCmdMlsd(int sessionId, const char* args, int len)
{
	const char* realPath;
	socket_t s;
//#### Funktioniert nicht wenn Pfad übergeben wird
/*	if(args[0] != '\0')
	{
		if(args[0] != '-')
			realPath = ftpGetRealPath(sessionId, args);
	}
	else*/
		realPath = ftpGetRealPath(sessionId, ftpGetSession(sessionId)->workingDir, TRUE);

	if(ftpGetSession(sessionId)->passive == FALSE)
    {
		s = ftpEstablishDataConnection(FALSE, &ftpGetSession(sessionId)->remoteIp, &ftpGetSession(sessionId)->remoteDataPort);
        if(s < 0)
        {
        	ftpSendMsg(MSG_NORMAL, sessionId, 425, ftpMsg011);
            return 1;
        }
    }
    else
    {
    	s = ftpAcceptDataConnection(ftpGetSession(sessionId)->passiveDataSocket);
        if(s < 0)
        {
        	ftpSendMsg(MSG_NORMAL, sessionId, 425, ftpMsg012);
            return 1;
        }
    }
	sendListing(s, sessionId, realPath, MLSD);
    ftpCloseSocket(s);

	return 0;
}
#endif

typedef struct
{
	char cmdToken[5];		///< command-string
	int  tokLen;			///< len of cmdToken
	int  neededRights;		///< required access rights for executing the command
	int  needLogin;			///< if TRUE, command needs successful authentication via USER and PASS
	int  reportFeat;		///< if TRUE, command is reported via FEAT
	int  duringTransfer;	///< if TRUE, command can be executed during a file transfer
	int (*handler)(int, const char*, int);	///< handler function
}ftpCommand_S;

static const ftpCommand_S cmds[] = {
		{"USER", 4, 0,  			FALSE, FALSE, FALSE, ftpCmdUser},
		{"PASS", 4,	0,  			FALSE, FALSE, FALSE, ftpCmdPass},
		{"SYST", 4,	0, 				TRUE,  FALSE, FALSE, ftpCmdSyst},
		{"PORT", 4,	0,  			TRUE,  FALSE, FALSE, ftpCmdPort},
		{"NOOP", 4,	0, 				TRUE,  FALSE, FALSE, ftpCmdNoop},
		{"QUIT", 4,	0, 				FALSE, FALSE, TRUE,  ftpCmdQuit},
		{"ABOR", 4,	0, 				FALSE, FALSE, TRUE,  ftpCmdAbor},
		{"LIST", 4,	FTP_ACC_LS,		TRUE,  FALSE, FALSE, ftpCmdList},
		{"NLST", 4,	FTP_ACC_LS, 	TRUE,  FALSE, FALSE, ftpCmdNlst},
		{"PWD",  3,	FTP_ACC_RD, 	TRUE,  FALSE, FALSE, ftpCmdPwd},
		{"XPWD", 4,	FTP_ACC_DIR, 	TRUE,  FALSE, FALSE, ftpCmdPwd},
		{"TYPE", 4,	0, 				TRUE,  FALSE, FALSE, ftpCmdType},
		{"PASV", 4,	0, 				TRUE,  FALSE, FALSE, ftpCmdPasv},
		{"CWD",  3,	FTP_ACC_DIR,  	TRUE,  FALSE, FALSE, ftpCmdCwd},
		{"CDUP", 4,	FTP_ACC_DIR, 	TRUE,  FALSE, FALSE, ftpCmdCdup},
		{"STRU", 4,	0,  			TRUE,  FALSE, FALSE, ftpCmdStru},
		{"RETR", 4,	FTP_ACC_RD,  	TRUE,  FALSE, FALSE, ftpCmdRetr},
		{"STOR", 4,	FTP_ACC_WR,  	TRUE,  FALSE, FALSE, ftpCmdStor},
		{"DELE", 4,	FTP_ACC_WR,  	TRUE,  FALSE, FALSE, ftpCmdDele},
		{"MKD" , 3,	FTP_ACC_WR,  	TRUE,  FALSE, FALSE, ftpCmdMkd},
		{"XMKD", 4,	FTP_ACC_WR,  	TRUE,  FALSE, FALSE, ftpCmdMkd},
		{"RMD" , 3,	FTP_ACC_WR,  	TRUE,  FALSE, FALSE, ftpCmdRmd},
		{"XRMD", 4,	FTP_ACC_WR,  	TRUE,  FALSE, FALSE, ftpCmdRmd},
#if RFC3659
		{"SIZE", 4,	FTP_ACC_RD,  	TRUE,  TRUE,  FALSE, ftpCmdSize},
		{"MDTM", 4,	FTP_ACC_RD,  	TRUE,  TRUE,  FALSE, ftpCmdMdtm},
		{"MLST", 4,	FTP_ACC_RD,  	TRUE,  TRUE,  FALSE, ftpCmdMlst},
		{"MLSD", 4,	FTP_ACC_LS,  	TRUE,  TRUE,  FALSE, ftpCmdMlsd}
#endif
};

int execFtpCmd(int sessionId, const char* cmd, int cmdlen)
{
	int n;
    int ret = 0;

	ftpSession_S *pSession = ftpGetSession(sessionId);


	for(n = 0; n < ARRAY_SIZE(cmds); n++)
	{
		if(!strncmp(cmds[n].cmdToken, cmd, cmds[n].tokLen))
		{
			int i = cmds[n].tokLen;

			if(cmds[n].needLogin)
			{
				if((pSession->userId == 0) || (pSession->authenticated == FALSE))
				{
					ftpSendMsg(MSG_NORMAL, sessionId, 530, ftpMsg033);
					return 0;
				}
				if(ftpCheckAccRights(pSession->userId, cmds[n].neededRights))
				{
					ftpSendMsg(MSG_NORMAL, sessionId, 550, ftpMsg034);
					return 0;
				}
			}

			if((pSession->activeTrans.op != OP_NOP)) 	// transfer in progress?
			{
				if(cmds[n].duringTransfer == FALSE)		// command during transfer allowed?
					return 0;							// no => silently discard command
			}

			while(cmd[i] != '\0')
			{
				if((cmd[i] != ' ') && (cmd[i] != '\t'))
					break;

				i++;
			}
			ret = cmds[n].handler(sessionId, &cmd[i], strlen(&cmd[i]));	// execute command
        	pSession->timeLastCmd = ftpGetUnixTime();
            return ret;
		}
	}

	ftpSendMsg(MSG_NORMAL, sessionId, 500, cmd);					// reject unknown commands
	pSession->timeLastCmd = ftpGetUnixTime();
	return ret;
}

void ftpParseCmd(int sessionId)
{
	ftpSession_S *pSession;
	int len;
	int c;
	socket_t ctrlSocket;

	pSession   = ftpGetSession(sessionId);
	len        = pSession->rxBufWriteIdx;
	ctrlSocket = pSession->ctrlSocket;

	if((pSession->rxBuf[len - 1] == '\n') &&
	   (pSession->rxBuf[len - 2] == '\r') )		// command correctly terminated?
	{
		pSession->rxBuf[len - 2] = '\0';
		pSession->rxBufWriteIdx  = 0;

		for(c = 0; c < len; c++)				// convert command token to uppercase
		{
			if(isspace(pSession->rxBuf[c]))
				break;
			pSession->rxBuf[c] = toupper(pSession->rxBuf[c]);
		}

		#if DBG_LOG
		printf("%02d --> %s\n", sessionId, pSession->rxBuf);
		#endif

		if(execFtpCmd(sessionId, pSession->rxBuf, len - 2) == -1)
		{
			ftpUntrackSocket(ctrlSocket);
			ftpCloseSession(sessionId);
		}
	}

	if(pSession->rxBufWriteIdx >= LEN_RXBUF)	// overflow of receive buffer?
	{
		pSession->rxBufWriteIdx = 0;
		ftpSendMsg(MSG_NORMAL, sessionId, 500, ftpMsg035);
		#if DBG_LOG
		printf("Receive buffer overflow. Received data discarded.\n");
		#endif
	}
}

