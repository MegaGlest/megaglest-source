/**
 * Feathery FTP-Server <https://sourceforge.net/projects/feathery>
 * Copyright (C) 2005-2010 Andreas Martin (andreas.martin@linuxmail.org)
 *
 * ftpLib.c - Global helper functions
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
#include <stdarg.h>

#include "ftpTypes.h"
#include "ftpConfig.h"
#include "ftp.h"
#include "ftpIfc.h"

/**
 *  @brief Removes the trailing slash of a directory path
 *
 *  @param path   directory path
 *
 *  @return len of path
 */
int ftpRemoveTrailingSlash(char* path)
{
	int len = strlen(path);

	if(len > 1)
	{
		len--;
		if(path[len] == '/')
			path[len] = '\0';
	}
	return len;
}

/**
 *  @brief Removes double slashes in a path
 *
 *  e. g. "//a//b//c" will be converted to "a/b/c"
 *
 *  @param path   directory path
 */
void ftpRemoveDoubleSlash(char* path)
{
    char *p;

    p = path;

    while(*p != '\0')
    {
        if((p[0] == '/') && (p[1] == '/'))
            ftpStrcpy(p, &p[1]);
        else
            p++;
    }
}

/**
 *  @brief Merges multiple path strings
 *
 *  The function catenates all passed strings to a new string and assures that  
 *  the result does not exceed MAX_PATH_LEN. The last parameter has to be
 *  NULL.
 *  Not all embedded environments support variadic functions, or they are
 *  too expensive. 
 *
 *  @param dest    user name
 *
 *  @return Pointer to dest
 */
char* ftpMergePaths(char* dest, ...)
{
	const char* src;
	char* dst = dest;
	int len = 0;
	va_list args;

	va_start(args, dest);

	while((src = va_arg(args, const char*)))
	{
		while((*src != '\0') && (len < MAX_PATH_LEN-1))
		{
			*dst++ = *src++;
			len++;
		}
	}

	*dst = '\0';
	va_end(args);

	return dst;
}

/**
 *  @brief Determine the unix-time
 *
 *  The function reads the clock from the target-layer and converts it to the   
 *  unix-time. The code is taken from http://de.wikipedia.org/wiki/Unixzeit
 *
 *  @return seconds since 1. Januar 1970 00:00 
 */
uint32_t ftpGetUnixTime(void)
{
	ftpTime_S t;
	uint32_t unixTime = 0;
	int j;

	ftpGetLocalTime(&t);

	for(j=1970;j<t.year;j++)              // leap years
	{
		if(j%4==0 && (j%100!=0 || j%400==0))
			unixTime+=(366*24*60*60);
		else
			unixTime+=(365*24*60*60);
	}
	for(j=1;j<t.month;j++)                // days per month 31/30/29/28
	{
		if(j==1 || j==3 || j==5 || j==7 || j==8 || j==10 || j==12)
			unixTime+=(31*24*60*60);   		// months with 31 days
		if(j==4 || j==6 || j==9 || j==11)
			unixTime+=(30*24*60*60);   		// months with 30 days
		if( (j==2) && (t.year%4==0 && (t.year%100!=0 || t.year%400==0)) )
			unixTime+=(29*24*60*60);
		else
			unixTime+=(28*24*60*60);
	}
	unixTime+=((t.day-1)*24*60*60);
	unixTime+=(t.hour*60*60);
	unixTime+=t.minute*60;
	unixTime+=t.second;

	return unixTime;
}

/**
 *  @brief copy a string
 *
 *  The reason why feathery has its own strcpy is that in some cases src is
 *  part of dest and src > dest. Because ANSI-C explicitly declares that case
 *  as undefined our strcpy guarantees to copy from lower to higher
 *  addresses.
 *  
 *  @param dest destination string
 *  @param src Null-terminated source string
 *
 *  @return destination string 
 */
char *ftpStrcpy(char *dest, const char *src)
{
     char       *d = dest;     
     const char *s = src;

     while (*d++ = *s++);
     return dest;
}
