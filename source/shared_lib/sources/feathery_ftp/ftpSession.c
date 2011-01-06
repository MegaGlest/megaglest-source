/**
 * Feathery FTP-Server <https://sourceforge.net/projects/feathery>
 * Copyright (C) 2005-2010 Andreas Martin (andreas.martin@linuxmail.org)
 *
 * ftpSession.c - Session handling of connected clients
 *
 * A ftp session is a TCP connection between the server and a ftp client.
 * Each session has its own unique session id. The session id refers
 * (index in session[]) to a struct ftpSession_S which holds all
 * relevant data. The virtual chroot environment is also part of the
 * session management.
 *
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

#include <stdio.h>
#include <string.h>

#include "ftpTypes.h"
#include "ftpConfig.h"
#include "ftp.h"

/**
 *  @brief array that holds the data of all ftp sessions
 *
 *  The index represents the session ID.
 */
LOCAL ftpSession_S  sessions[MAX_CONNECTIONS];

/**
 *  @brief number of active file or directory transmission
 */
LOCAL int actTransCnt;

/**
 *  @brief Scratch-buffer for path manipulation
 */
LOCAL char pathScratchBuf[MAX_PATH_LEN];


/**
 *  @brief Opens a ftp session
 *
 *  Searches in 'sessions' for an empty entry and inits the session data.
 *  Call this function after accepting a tcp connection from a ftp client.
 *
 *  @param ctrlSocket   control socket which initiated the the ftp session
 *  @param remoteIp     IP address of the client
 *  @param remotePort   TCP-Port of the client
 *
 *  @return session id or -1 if session limit is reached
 */
int ftpOpenSession(socket_t ctrlSocket, ip_t remoteIp, port_t remotePort)
{
	int n;
	for(n = 0; n < MAX_CONNECTIONS; n++)
	{
		if(!sessions[n].open)
		{
			sessions[n].open               = TRUE;
			sessions[n].authenticated      = FALSE;
			sessions[n].userId             = 0;
			sessions[n].workingDir[0]      = '\0';
			sessions[n].remoteIp           = remoteIp;
			sessions[n].remotePort         = remotePort;
			sessions[n].remoteDataPort     = 0;
			sessions[n].passive            = FALSE;
			sessions[n].binary             = TRUE;
			sessions[n].timeLastCmd        = ftpGetUnixTime();
			sessions[n].ctrlSocket         = ctrlSocket;
			sessions[n].passiveDataSocket  = -1;
			sessions[n].rxBufWriteIdx      = 0;
			sessions[n].activeTrans.op       = OP_NOP;
			sessions[n].activeTrans.fsHandle = NULL;
			sessions[n].activeTrans.dataSocket = -1;
			sessions[n].activeTrans.fileSize = 0;

			return n;
		}
	}
	return -1;
}

/**
 *  @brief Marks the current ftp session as 'authenticated'
 *
 *  @param id    Session id
 *
 *  @return 0
 */
int ftpAuthSession(int id)
{
	sessions[id].authenticated = TRUE;
	strcpy(sessions[id].workingDir, "/");

	return 0;
}

/**
 *  @brief Closes a ftp session
 *
 *  Closes the TCP control connection and releases the session struct.
 *
 *  @param id    Session id
 *
 *  @return 0
 */
int ftpCloseSession(int id)
{
if(VERBOSE_MODE_ENABLED) printf("In ftpCloseSession sessionId = %d, remote IP = %u, port = %d\n",
           id, sessions[id].remoteIp, sessions[id].remoteFTPServerPassivePort);

    if(ftpFindExternalFTPServerIp != NULL && ftpFindExternalFTPServerIp(sessions[id].remoteIp) != 0)
    {
        if(ftpRemoveUPNPPortForward)
        {
if(VERBOSE_MODE_ENABLED) printf("In ftpCmdPasv sessionId = %d, removing UPNP port forward [%d]\n", id,sessions[id].remoteFTPServerPassivePort);

            //ftpRemoveUPNPPortForward(sessions[id].remoteFTPServerPassivePort, sessions[id].remoteFTPServerPassivePort);
            sessions[id].remoteFTPServerPassivePort = 0;
        }
    }
    if(sessions[id].open) {
        ftpCloseSocket(sessions[id].ctrlSocket);
        ftpCloseTransmission(id);
        if(sessions[id].passiveDataSocket > 0)
        {
            ftpCloseSocket(sessions[id].passiveDataSocket);
        }
    }
    sessions[id].remoteIp = 0;
    sessions[id].ctrlSocket = 0;
    sessions[id].passiveDataSocket = 0;
	sessions[id].open = FALSE;

if(VERBOSE_MODE_ENABLED) printf("Session %d closed\n", id);

	return 0;
}

/**
 *  @brief Returns pointer to session struct
 *
 *  @param id    Session id
 *
 *  @return pointer to session data
 */
ftpSession_S* ftpGetSession(int id)
{
	return &sessions[id];
}

/**
 *  @brief Normalizes a path string
 *
 *  The function resolves all dot and doubledot directory entries in the
 *  passed path. If the normalized path refers beyond root, root is returned.
 *
 *  @param path   string of an absolute path
 *
 *  @return 0
 */
LOCAL int normalizePath(char* path)
{
    char *in;
    char *r = NULL;

    r = path;
    in = path;

    while((in = strchr(in, '/')))
    {
        if(!strncmp(in, "/./", 3))
        {
            ftpStrcpy(in, &in[2]);
            continue;
        }
        if(!strcmp(in, "/."))
        {
            in[1] = '\0';
            continue;
        }
        if(!strncmp(in, "/../", 4))
        {
            *in = '\0';
            r = strrchr(path, '/');
            if(r == NULL)
                r = path;
            ftpStrcpy(r, &in[3]);
            in = r;
            continue;
        }
        if(!strcmp(in, "/.."))
        {
            *in = '\0';
            r = strrchr(path, '/');
            if(r)
            {
                if(r == path)
                    r[1] = '\0';
                else
                    *r = '\0';
            }
            else
            {
                in[0] = '/';
                in[1] = '\0';
            }

            continue;
        }
        in++;
    }

    return 0;
}


/**
 *  @brief Translate a ftp session path to server path
 *
 *  The translated path depends on the current working directory of the
 *  session and the root path of the session. In addition the path will
 *  be normalized.
 *  The server path is generated as followed:
 *   passed path is relative => server path = ftp user root + session working dir + path
 *   passed path is absolute => server path = ftp user root + path
 *
 *  @param id    Session id
 *  @param path  ftp session path (can be relative or absolute)
 *  @param if != 0 dot and doubledot directories will be resolved
 *
 *  @return translated absolute server path
 */
const char* ftpGetRealPath(int id, const char* path, int normalize)
{
	const char* ftpRoot;
	int len;

	ftpRoot = ftpGetRoot(sessions[id].userId, &len);

if(VERBOSE_MODE_ENABLED) printf("#1 ftpGetRealPath id = %d path [%s] ftpRoot [%s] sessions[id].workingDir [%s] normalize = %d\n", id, path, ftpRoot, sessions[id].workingDir,normalize);

    pathScratchBuf[0]='\0';
	if(path[0] == '/' || strcmp(path,sessions[id].workingDir) == 0)				// absolute path?
	{
	    ftpMergePaths(pathScratchBuf, ftpRoot, path, NULL);
	}
	else
	{
		ftpMergePaths(pathScratchBuf, ftpRoot, sessions[id].workingDir, "/", path, NULL);
		//ftpMergePaths(pathScratchBuf, ftpRoot, path, NULL);
	}

if(VERBOSE_MODE_ENABLED) printf("#2 ftpGetRealPath path [%s] ftpRoot [%s] pathScratchBuf [%s]\n", path, ftpRoot, pathScratchBuf);

	ftpRemoveDoubleSlash(pathScratchBuf);
	if(normalize) {
		normalizePath(pathScratchBuf);
	}
	ftpRemoveTrailingSlash(pathScratchBuf);

if(VERBOSE_MODE_ENABLED) printf("#2 ftpGetRealPath path [%s] ftpRoot [%s] pathScratchBuf [%s]\n", path, ftpRoot, pathScratchBuf);

	return pathScratchBuf;
}

/**
 *  @brief Change the current working directory of the session
 *
 *  @param id    Session id
 *  @param path  destination ftp path (can be relative or absolute)
 *
 *  @return 0 on success; -2 if the directory doesn't exist
 */
int ftpChangeDir(int id, const char* path)
{
	ftpPathInfo_S fileInfo;
	const char* realPath = ftpGetRealPath(id, path, TRUE);
	int len;


	ftpGetRoot(sessions[id].userId, &len);							// determine len of root-path
	if(len == 1)													// if len == 1 root-path == '/'
		len = 0;

if(VERBOSE_MODE_ENABLED) printf("ftpChangeDir path [%s] realPath [%s] sessions[id].workingDir [%s]\n", path, realPath, sessions[id].workingDir);

	if(ftpStat(realPath, &fileInfo) || (fileInfo.type != TYPE_DIR)) // directory accessible?
		return -2;

	strncpy(sessions[id].workingDir, &realPath[len], MAX_PATH_LEN); // apply path
	if(sessions[id].workingDir[0] == '\0')
		strcpy(sessions[id].workingDir, "/");

if(VERBOSE_MODE_ENABLED) printf("ftpChangeDir path [%s] realPath [%s] NEW sessions[id].workingDir [%s]\n", path, realPath, sessions[id].workingDir);

	return 0;
}

/**
 *  @todo documentation
 */
void ftpOpenTransmission(int id, operation_E op, void* fsHandle, socket_t dataSocket, uint32_t fileSize)
{
	actTransCnt++;
	sessions[id].activeTrans.op         = op;
	sessions[id].activeTrans.fsHandle   = fsHandle;
	sessions[id].activeTrans.dataSocket = dataSocket;
	sessions[id].activeTrans.fileSize   = fileSize;
}

/**
 *  @todo documentation
 */
void ftpCloseTransmission(int id)
{
	if(sessions[id].activeTrans.op != OP_NOP)						// is thera an active transmission?
	{
		ftpCloseSocket(sessions[id].activeTrans.dataSocket);
		if(sessions[id].activeTrans.op == OP_LIST)
		{
			ftpCloseDir(sessions[id].activeTrans.fsHandle);
		}
		else
		{
			ftpCloseFile(sessions[id].activeTrans.fsHandle);
		}
		sessions[id].activeTrans.op = OP_NOP;
		actTransCnt--;
	}
}

/**
 *  @todo documentation
 */
int ftpGetActiveTransCnt(void)
{
	return actTransCnt;
}
