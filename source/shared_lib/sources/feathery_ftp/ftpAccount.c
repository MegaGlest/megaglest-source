/**
 * Feathery FTP-Server <https://sourceforge.net/projects/feathery>
 * Copyright (C) 2005-2010 Andreas Martin (andreas.martin@linuxmail.org)
 *
 * ftpAccount.c - User account handling
 *
 * User account management is based on username, password and
 * access-rights. An account can be created with the function
 * ftpCreateAccount.
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

#include <string.h>

#include "ftpTypes.h"
#include "ftpConfig.h"
#include "ftp.h"


/**
 * @brief  User account data
 */
typedef struct
{
	char name[MAXLEN_USERNAME];		///< user name
	char passw[MAXLEN_PASSWORD];	///< password of the account
	char ftpRoot[MAX_PATH_LEN];		///< root path of the user account on the server
	int  ftpRootLen;				///< length of ftpRoot
	int  accRights;					///< access rights of a account

}ftpUserAccount_S;

/**
 * @brief Array which holds all registered user accounts
 */
LOCAL ftpUserAccount_S ftpUsers[MAX_USERS];

/**
 *  @brief Creates a new user account
 *
 *  The translated path depends on the current working directory of the
 *  session and the root path of the session. In addition the path will
 *  be normalized.
 *  @todo normalize root and check if normalized path really exists
 *
 *  @param name    user name
 *  @param passw   account password
 *  @param root	   root directory of the account
 *  @param acc	   access rights, can be any combination of the following flags:
 *                 - FTP_ACC_RD read access
 *                 - FTP_ACC_WR write access
 *                 - FTP_ACC_LS access to directory listing
 *                 - FTP_ACC_DIR changing of working dir allowed
 *
 *  @return 0 on success; -1 if MAX_USERS is reached
 */
int ftpCreateAccount(const char* name, const char* passw, const char* root, int acc)
{
	int n;

	n = ftpFindAccount(name);				// check if account already exists
	if(n > 0)
	{
		ftpUsers[n - 1].name[0] = '\0';		// delete account
	}

	for(n = 0; n < MAX_USERS; n++)
	{
		if(ftpUsers[n].name[0] == '\0')
		{
			strncpy(ftpUsers[n].name, name, MAXLEN_USERNAME);
			strncpy(ftpUsers[n].passw, passw, MAXLEN_PASSWORD);
			strncpy(ftpUsers[n].ftpRoot, root, MAX_PATH_LEN);
			ftpUsers[n].ftpRootLen = strlen(root);
			ftpUsers[n].accRights = acc;
			return 0;
		}
	}
	return -1;
}

/**
 *  @brief Return the account id for a user name
 *
 *  The function searches ftpUsers for the passed user name.
 *  The returned account id is the index + 1 in ftpUsers.
 *
 *  @param name    user name
 *
 *  @return 0 if user is not found; 1 to MAX_USERS+1 if user is found
 */
int ftpFindAccount(const char* name)
{
	int n;

	if(name[0] != '\0')
		for(n = 0; n < MAX_USERS; n++)
			if(!strncmp(ftpUsers[n].name, name, MAXLEN_USERNAME))
				return n + 1;

	return 0;
}

/**
 *  @brief Checks the password of a user account
 *
 *  Compares the passed password to the saved password in ftpUsers
 *
 *  @param userId   user account id
 *  @param passw    password
 *
 *  @return -  0: password is correct
 *          - -1: invalid user account id
 *          - else: incorrect password
 */
int ftpCheckPassword(int userId, const char* passw)
{
	if(!userId)
		return -1;
	else if(ftpUsers[userId - 1].passw[0] == '\0')
		return 0;
	else
		return strncmp(ftpUsers[userId - 1].passw, passw, MAXLEN_PASSWORD);
}

/**
 *  @brief Checks if the account has the needed rights
 *
 *  Compares the passed access rights to the saved rights in ftpUsers
 *
 *  @param userId     user account id
 *  @param accRights  needed access rights
 *
 *  @return -  0: the needed access rights are fulfilled
 *          - -1: invalid user account id
 */
int ftpCheckAccRights(int userId, int accRights)
{
	if(!userId)
		return -1;

	if((ftpUsers[userId - 1].accRights & accRights) == accRights)
		return 0;

	return -1;
}

/**
 *  @brief Returns the root directory of a account
 *
 *  @param userId  user account id
 *  @param len     length of the returned path
 *
 *  @return  root directory name or NULL if the user account id is invalid
 */
const char* ftpGetRoot(int userId, int* len)
{
	if(!userId)
		return NULL;
	if(len)
		*len = ftpUsers[userId - 1].ftpRootLen;

	return ftpUsers[userId - 1].ftpRoot;
}
