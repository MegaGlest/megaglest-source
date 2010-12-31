/**
 * Feathery FTP-Server <https://sourceforge.net/projects/feathery>
 * Copyright (C) 2005-2010 Andreas Martin (andreas.martin@linuxmail.org)
 *
 * ftp.h - ftp-server interface-header
 *
 * Contains all internal definitions and prototypes of feathery.
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


#ifndef FTP_H_
#define FTP_H_


/**
 * @brief specifies the type of operation that is in progress
 */
typedef enum
{
	OP_NOP  = 0,						///< no operation
	OP_RETR = 1, 						///< RETR command
	OP_STOR = 2, 						///< STOR command
	OP_LIST = 3							///< LIST command
}operation_E;

/**
 * @brief  infos about a file/directory transmittion
 * @see ftpExecTransmission
 */
typedef struct
{
	operation_E op;						///< active transmission
	void* 		fsHandle; 				///< file or directory handle of current transmission
	socket_t 	dataSocket; 			///< data socket of current transmission
	uint32_t 	fileSize;				///< size of file to transmit

}transmission_S;

/**
 * @brief  Data of a ftp session
 */
typedef struct
{
	int      open;						///< TRUE, if the session is open
	int      authenticated;				///< TRUE, if user is authenticated via USER and PASS commands
	int	     userId;					///< id of the dedicated user account, is 0 until execution of USER command
	int		 rxBufWriteIdx;				///< currect write index of receive buffer rxBuf
	char     rxBuf[LEN_RXBUF];			///< receive buffer for ftp commands
	char     workingDir[MAX_PATH_LEN];  ///< current working directory (absolute path which is relative to the root-path of the user account)
	ip_t     remoteIp;					///< IP of connected ftp client
	port_t   remoteFTPServerPassivePort; ///< Port of the FTP Server from the clients perspective related to Passive connection
	port_t   remotePort;				///< Port of connected ftp client for control connection
	port_t   remoteDataPort;			///< Port of connected ftp client for data connection
	int      passive;					///< TRUE, if passive-mode is activated via PSV command
	int      binary;					///< TRUE, if binary-mode is active
	uint32_t timeLastCmd;				///< timestamp of last ftp activity (used for timeout generation)
	socket_t ctrlSocket;				///< socket for control connection
	socket_t passiveDataSocket;			///< listener socket for data connections in passive mode
	transmission_S activeTrans;			///< infos about a currently active file/directory-transmission

}ftpSession_S;

/**
 * @brief format in which a message is to be displayed
 */
typedef enum
{
	MSG_NORMAL    = 0,					///< normal message, no translation
	MSG_QUOTE     = 1, 					///< message will be displayed between doublequotes
	MSG_MULTILINE = 2					///< message contains multiple lines
}msgmode_E;

/**
 * @brief time-structure
 */
typedef struct
{
	uint16_t year;						///< year 0000-9999
	uint8_t  month;						///< month 1 - 12
	uint8_t  day;						///< day 1 - 31
	uint8_t  hour;						///< hour 0 - 23
	uint8_t  minute;					///< minute 0 - 59
	uint8_t  second; 					///< second 0 - 59

}ftpTime_S;

/**
 * @brief type of a file
 */
typedef enum
{
	TYPE_FILE = 1, 						///< regular file
	TYPE_DIR  = 2, 						///< directory
	TYPE_LINK = 4						///< hard link

}ftpFileType_E;

/**
 * @brief infos about a file or directory similar to stat
 */
typedef struct
{
	ftpFileType_E 	type;				///< filetype
	uint16_t 		mode;				///< access rights (currently unused)
	ftpTime_S 		cTime;				///< creation time (currently unused)
	ftpTime_S 		aTime;				///< last access time  (currently unused)
	ftpTime_S 		mTime;				///< time of last modification
	uint32_t 		size;				///< size
	int     		links;				///< count of hard links
	char    		user[20];			///< owner
	char    		group[20];			///< group

}ftpPathInfo_S;

extern int ftpOpenSession(socket_t ctrlSocket, ip_t remoteIp, port_t remotePort);
extern int ftpCloseSession(int id);
extern ftpSession_S* ftpGetSession(int id);
extern int ftpAuthSession(int id);
extern const char* ftpGetRealPath(int id, const char* path, int normalize);
extern int ftpChangeDir(int id, const char* path);
extern void ftpOpenTransmission(int id, operation_E op, void* fsHandle, socket_t dataSocket, uint32_t fileSize);
extern void ftpCloseTransmission(int id);
extern int ftpGetActiveTransCnt(void);

extern int ftpFindAccount(const char* name);
extern int ftpCheckPassword(int userId, const char* passw);
extern int ftpCheckAccRights(int userId, int accRights);
extern const char* ftpGetRoot(int userId, int* len);

extern int ftpSendMsg(msgmode_E mode, int sessionId, int ret, const char* msg);
extern int ftpExecTransmission(int sessionId);
extern void ftpParseCmd(int sessionId);

extern int ftpRemoveTrailingSlash(char* path);
extern void ftpRemoveDoubleSlash(char* path);
extern char* ftpMergePaths(char* dest, ...);
extern uint32_t ftpGetUnixTime(void);
extern char *ftpStrcpy(char *dest, const char *src);


extern void ftpArchInit();
extern void ftpArchCleanup(void);
extern int ftpGetLocalTime(ftpTime_S *t);
extern void* ftpOpenDir(const char* path);
extern const char* ftpReadDir(void* dirHandle);
extern int ftpCloseDir(void* dirHandle);
extern int ftpStat(const char* path, ftpPathInfo_S *info);
#if ANSI_FILE_IO
#	define ftpOpenFile   fopen
#	define ftpCloseFile  fclose
#	define ftpReadFile   fread
#	define ftpWriteFile  fwrite
#	define ftpRemoveFile remove
#else
extern void* ftpOpenFile(const char *filename, const char *mode);
extern int ftpCloseFile(void *stream );
extern int ftpReadFile(void *buffer, size_t size, size_t count, void *stream);
extern int ftpWriteFile(const void *buffer, size_t size, size_t count, void *stream);
extern int ftpRemoveFile(const char* path);
#endif
extern int ftpMakeDir(const char* path);
extern int ftpRemoveDir(const char* path);
extern int ftpCloseSocket(socket_t s);
extern int ftpSend(socket_t s, const void *data, int len);
extern int ftpReceive(socket_t s, void *data, int len);
extern socket_t ftpEstablishDataConnection(int passive, ip_t *ip, port_t *port);
extern socket_t ftpAcceptDataConnection(socket_t listner);
extern socket_t ftpCreateServerSocket(int portNumber);
extern socket_t ftpAcceptServerConnection(socket_t server, ip_t *remoteIP, port_t *remotePort);
extern int ftpTrackSocket(socket_t s);
extern int ftpUntrackSocket(socket_t s);
extern int ftpTestSocket(socket_t s);
extern int ftpSelect(int poll);

#endif /* FTP_H_ */
