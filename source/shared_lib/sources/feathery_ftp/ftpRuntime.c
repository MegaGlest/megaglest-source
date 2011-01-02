/**
 * Feathery FTP-Server <https://sourceforge.net/projects/feathery>
 * Copyright (C) 2005-2010 Andreas Martin (andreas.martin@linuxmail.org)
 *
 * ftpRuntime.c - ftp-server runtime environment
 *
 * This is the central module of feathery. It defines all runtime functions
 * of feathery. That is startup, state, main-thread an shutdown.
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

#include "ftpTypes.h"
#include "ftpConfig.h"
#include "ftp.h"
#include "ftpMessages.h"


/**
 * @brief server-sockets that listens for incoming connections
 */
LOCAL socket_t server;

void ftpInit(ftpFindExternalFTPServerIpType cb1, ftpAddUPNPPortForwardType cb2, ftpRemoveUPNPPortForwardType cb3) {
	ftpFindExternalFTPServerIp	= cb1;
	ftpAddUPNPPortForward		= cb2;
	ftpRemoveUPNPPortForward	= cb3;
}

/**
 *  @brief Initializes and starts the server
 *
 *
 *  @return -  0: server started successfully
 *          - -1: could not create server socket
 */
int ftpStart(int portNumber)
{
	server = -1;														// set server socket to invalid value

if(VERBOSE_MODE_ENABLED) printf("Feathery FTP-Server\n");

	ftpArchInit();

if(VERBOSE_MODE_ENABLED) printf("Creating server socket");

	server = ftpCreateServerSocket(portNumber);									// create main listener socket
	if(server < 0)
	{
if(VERBOSE_MODE_ENABLED) printf("\t\terror\n");

		return -1;
	}

if(VERBOSE_MODE_ENABLED) printf("\t\tok\n");
if(VERBOSE_MODE_ENABLED) printf("Server successfully started\n");

	ftpTrackSocket(server);												// add socket to "watchlist"
	return 0;
}


/**
 *  @brief Checks if the server has something to do
 *
 *  In order of avoid blocking of ftpExecute this function can be used to
 *  determine if the server has something to do. You can call this function in
 *  a loop and everytime it returns nonezero its time to call ftpExecute().
 *  @todo implement me
 *
 *  @return 0 noting to do; else server has received some data
 */
int ftpState(void)
{
	return 0;
}


/**
 *  @brief Main server-thread
 *
 *  Manages all client connections and active transmitions. In addtition
 *  all received control-strings are dispatched to the command-module.
 *  Note, the function blocks if there is nothing to do.
 */
void ftpExecute(void)
{
	int n;
	int socksRdy;
	ftpSession_S *pSession;
	int sessionId;
	int activeJobs;


	activeJobs = ftpGetActiveTransCnt();								// are there any active transmitions?
	for(n = 0; (activeJobs > 0) && (n < MAX_CONNECTIONS); n++)
	{
		pSession = ftpGetSession(n);
		if(pSession->activeTrans.op)									// has this session an active transmition?
		{
			ftpExecTransmission(n);										// do the job
			activeJobs--;
		}
	}

	//if(ftpGetActiveTransCnt())											// don't block if there's still something to do
	socksRdy = ftpSelect(TRUE);
	//else
	//	socksRdy = ftpSelect(FALSE);
	if(socksRdy > 0)
	{
		if(ftpTestSocket(server))										// server listner-socket signaled?
		{
			socket_t clientSocket;
			ip_t remoteIP;
			port_t remotePort;

			socksRdy--;
			clientSocket = ftpAcceptServerConnection(server, &remoteIP, &remotePort);
			if(clientSocket >= 0)
			{
				sessionId = ftpOpenSession(clientSocket, remoteIP, remotePort);
				if(sessionId >= 0)
				{
					ftpTrackSocket(clientSocket);
					ftpSendMsg(MSG_NORMAL, sessionId, 220, ftpMsg037);
				}
				else
				{
if(VERBOSE_MODE_ENABLED) printf("Connection refused; Session limit reached.\n");

					ftpCloseSocket(clientSocket);
				}
			}
		}

		for(n = 0; (socksRdy > 0) && (n < MAX_CONNECTIONS); n++)
		{
			pSession = ftpGetSession(n);
            if(pSession->open)
            {
                socket_t ctrlSocket = pSession->ctrlSocket;

				if(ftpTestSocket(ctrlSocket))
				{
					int len;
					socksRdy--;
					len = ftpReceive(ctrlSocket,
									 &pSession->rxBuf[pSession->rxBufWriteIdx],
									 LEN_RXBUF - pSession->rxBufWriteIdx);
					if(len <= 0)											// has client shutdown the connection?
					{
						ftpUntrackSocket(ctrlSocket);
						ftpCloseSession(n);
					}
					else
					{
						pSession->rxBufWriteIdx += len;
						ftpParseCmd(n);
					}
				}
				/// @bug Session-Timeout-Management doesn't work
            	if((ftpGetUnixTime() - pSession->timeLastCmd) > SESSION_TIMEOUT)
            	{
            		ftpSendMsg(MSG_NORMAL, n, 421, ftpMsg036);
					ftpUntrackSocket(ctrlSocket);
					ftpCloseSession(n);
            	}
			}
		}
	}
}


/**
 *  @brief Shut down the server
 *
 *  Cuts all tcp-connections and frees all used resources.

 *  @return 0
 */
int ftpShutdown(void)
{
	int n;
	ftpUntrackSocket(server);
	ftpCloseSocket(server);

	for(n = 0; n < MAX_CONNECTIONS; n++)
	{
		if(ftpGetSession(n)->open)
		{
			ftpUntrackSocket(ftpGetSession(n)->ctrlSocket);
		}
		ftpCloseSession(n);
	}

	ftpArchCleanup();

	return 0;
}
