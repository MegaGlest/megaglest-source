/**
 * Feathery FTP-Server <https://sourceforge.net/projects/feathery>
 * Copyright (C) 2005-2010 Andreas Martin (andreas.martin@linuxmail.org)
 *
 * ftpIfc.h - ftp-server interface-header
 *
 * All exported functions and constants are defined in this header. This is
 * the only header-file that a application which uses feathery shall include.
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

#ifndef FTPIFC_H_
#define FTPIFC_H_


#define FTP_ACC_RD	1
#define FTP_ACC_WR	2
#define FTP_ACC_LS	4
#define FTP_ACC_DIR	8
#define FTP_ACC_FULL (FTP_ACC_RD | FTP_ACC_WR | FTP_ACC_LS | FTP_ACC_DIR)

#include "ftpTypes.h"

#ifdef	__cplusplus
extern "C" {
#endif

void ftpInit(ftpFindExternalFTPServerIpType cb1, ftpAddUPNPPortForwardType cb2, ftpRemoveUPNPPortForwardType cb3, ftpIsValidClientType cb4);
int ftpCreateAccount(const char* name, const char* passw, const char* root, int accRights);
int ftpStart(int portNumber);
int ftpShutdown(void);
int ftpExecute(void);
int ftpState(void);

#ifdef	__cplusplus
}
#endif

#endif /* FTPIFC_H_ */
