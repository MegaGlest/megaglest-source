/**
 * Feathery FTP-Server <https://sourceforge.net/projects/feathery>
 * Copyright (C) 2005-2010 Andreas Martin (andreas.martin@linuxmail.org)
 *
 * ftpConfig.h - Compile time server configuration
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

#ifndef FTPCONFIG_H_
#define FTPCONFIG_H_

/**
 * @brief max. possible simultaneous FTP client connections
 */
#define MAX_CONNECTIONS 20

/**
 * @brief max. possible user accounts
 */
#define MAX_USERS		10

/**
 * @brief max. length of a user account name
 */
#define MAXLEN_USERNAME	40

/**
 * @brief max. length of a user account password
 */
#define MAXLEN_PASSWORD	40

/**
 * @brief session timeout in seconds
 */
#define SESSION_TIMEOUT 600

/**
 * @brief maximum length of a complete directory path
 */
#define MAX_PATH_LEN	1024

/**
 * @brief Size of the scratch buffer
 *
 * The scratch buffer is used for
 *  send / receive of files and directory listings
 */
#define LEN_SCRATCHBUF	1024

/**
 * @brief Size of the receive buffer for ftp commands
 *
 * Buffer must be big enough to hold a complete ftp command.
 * command (4) + space (1) + path (MAX_PATH_LEN) + quotes (2) + CRLF (2) + end of string (1)
 */
#define LEN_RXBUF		(MAX_PATH_LEN + 10)

/**
 * @brief activates ftp extentions according to RFC 3659
 */
#define RFC3659			1

/**
 * @brief set to 1 to activate debug messages on stdout
 */
//#define DBG_LOG			1

/**
 * @brief set to 1 if target-plattform supports ANSI-C file-IO functions
 */
#define ANSI_FILE_IO	1

#endif /* FTPCONFIG_H_ */
