/* /////////////////////////////////////////////////////////////////////////
 * File:    glob.c
 *
 * Purpose: Definition of the glob() API functions for the Win32 platform.
 *
 * Created: 13th November 2002
 * Updated: 6th February 2010
 *
 * Home:    http://synesis.com.au/software/
 *
 * Copyright (c) 2002-2010, Matthew Wilson and Synesis Software
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer. 
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the names of Matthew Wilson and Synesis Software nor the names of
 *   any contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * ////////////////////////////////////////////////////////////////////// */

/* /////////////////////////////////////////////////////////////////////////
 * Includes
 */

#include <glob.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#define NOMINMAX
#include <windows.h>
#include "platform_util.h"

#define NUM_ELEMENTS(ar)      (sizeof(ar) / sizeof(ar[0]))

using namespace Shared::Util;
/* /////////////////////////////////////////////////////////////////////////
 * Helper functions
 */

static char const *strrpbrk(char const *string, char const *strCharSet)
{
    char const  *part   =   NULL;
    char const  *pch;

    for(pch = strCharSet; *pch; ++pch)
    {
        const char    *p  =   strrchr(string, *pch);

        if(NULL != p)
        {
            if(NULL == part)
            {
                part = p;
            }
            else
            {
                if(part < p)
                {
                    part = p;
                }
            }
        }
    }

    return part;
}

/* /////////////////////////////////////////////////////////////////////////
 * API functions
 */

/* It gives you back the matched contents of your pattern, so for Win32, the
 * directories must be included
 */

int glob(   char const  *pattern
        ,   int         flags
#if defined(__COMO__)
        , int         (*errfunc)(char const *, int)
#else /* ? compiler */
        , const int   (*errfunc)(char const *, int)
#endif /* compiler */
        ,   glob_t      *pglob)
{
    int                 result;
    char                szRelative[1 + _MAX_PATH];
    char const          *file_part;
    WIN32_FIND_DATAW    find_data;
    HANDLE              hFind;
    char                *buffer;
    char                szPattern2[1 + _MAX_PATH];
    char                szPattern3[1 + _MAX_PATH];
    char const          *effectivePattern   =   pattern;
    char const          *leafMost;
    const int           bMagic              =   (NULL != strpbrk(pattern, "?*"));
    int                 bNoMagic            =   0;
    int                 bMagic0;
    size_t              maxMatches          =   ~(size_t)(0);

    assert(NULL != pglob);

    if(flags & GLOB_NOMAGIC)
    {
        bNoMagic = !bMagic;
    }

    if(flags & GLOB_LIMIT)
    {
        maxMatches = (size_t)pglob->gl_matchc;
    }

    if(flags & GLOB_TILDE)
    {
        /* Check that begins with "~/" */
        if( '~' == pattern[0] &&
            (   '\0' == pattern[1] ||
                '/' == pattern[1] ||
                '\\' == pattern[1]))
        {
            DWORD   dw;

            (void)lstrcpyA(&szPattern2[0], "%HOMEDRIVE%%HOMEPATH%");

            dw = ExpandEnvironmentStringsA(&szPattern2[0], &szPattern3[0], NUM_ELEMENTS(szPattern3) - 1);

            if(0 != dw)
            {
                (void)lstrcpynA(&szPattern3[0] + dw - 1, &pattern[1], (int)(NUM_ELEMENTS(szPattern3) - dw));
                szPattern3[NUM_ELEMENTS(szPattern3) - 1] = '\0';

                effectivePattern = szPattern3;
            }
        }
    }

    file_part = strrpbrk(effectivePattern, "\\/");

    if(NULL != file_part)
    {
        leafMost = ++file_part;

        (void)lstrcpyA(szRelative, effectivePattern);
        szRelative[file_part - effectivePattern] = '\0';
    }
    else
    {
        szRelative[0] = '\0';
        leafMost = effectivePattern;
    }

    bMagic0 =   (leafMost == strpbrk(leafMost, "?*"));

	std::wstring wstr = utf8_decode(effectivePattern);
    hFind   =   FindFirstFileW(wstr.c_str(), &find_data);
    buffer  =   NULL;

    pglob->gl_pathc = 0;
    pglob->gl_pathv = NULL;

    if(0 == (flags & GLOB_DOOFFS))
    {
        pglob->gl_offs = 0;
    }

    if(hFind == INVALID_HANDLE_VALUE)
    {
        /* If this was a pattern search, and the
         * directory exists, then we return 0
         * matches, rather than GLOB_NOMATCH
         */
        if( bMagic &&
            NULL != file_part)
        {
            result = 0;
        }
        else
        {
            if(NULL != errfunc)
            {
                (void)errfunc(effectivePattern, (int)GetLastError());
            }

            result = GLOB_NOMATCH;
        }
    }
    else
    {
        int     cbCurr      =   0;
        size_t  cbAlloc     =   0;
        size_t  cMatches    =   0;

        result = 0;

        do
        {
            int     cch;
            size_t  new_cbAlloc;

            if( bMagic0 &&
                0 == (flags & GLOB_PERIOD))
            {
                if('.' == find_data.cFileName[0])
                {
                    continue;
                }
            }

            if(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
#ifdef GLOB_ONLYFILE
                if(flags & GLOB_ONLYFILE)
                {
                    continue;
                }
#endif /* GLOB_ONLYFILE */

                if( bMagic0 &&
                    GLOB_NODOTSDIRS == (flags & GLOB_NODOTSDIRS))
                {
                    /* Pattern must begin with '.' to match either dots directory */
                    if( 0 == lstrcmpW(L".", find_data.cFileName) ||
                        0 == lstrcmpW(L"..", find_data.cFileName))
                    {
                        continue;
                    }
                }

                if(flags & GLOB_MARK)
                {
#if 0
                    if(find_data.cFileName[0] >= 'A' && find_data.cFileName[0] <= 'M')
#endif /* 0 */
                    (void)lstrcatW(find_data.cFileName, L"/");
                }
            }
            else
            {
                if(flags & GLOB_ONLYDIR)
                {
                    /* Skip all further actions, and get the next entry */
#if 0
                    if(find_data.cFileName[0] >= 'A' && find_data.cFileName[0] <= 'M')
#endif /* 0 */
                    continue;
                }
            }

            //cch = lstrlenW(find_data.cFileName);
			string sFileName = utf8_encode(find_data.cFileName);
			cch = sFileName.length();
            if(NULL != file_part)
            {
                cch += (int)(file_part - effectivePattern);
            }

            new_cbAlloc = (size_t)cbCurr + cch + 1;
            if(new_cbAlloc > cbAlloc)
            {
                char    *new_buffer;

                new_cbAlloc *= 2;

                new_cbAlloc = (new_cbAlloc + 31) & ~(31);

                new_buffer  = (char*)realloc(buffer, new_cbAlloc);

                if(new_buffer == NULL)
                {
                    result = GLOB_NOSPACE;
                    if(buffer) {
                    	free(buffer);
                    	buffer = NULL;
                    }
                    break;
                }

                buffer = new_buffer;
                cbAlloc = new_cbAlloc;
            }

            (void)lstrcpynA(buffer + cbCurr, szRelative, 1 + (int)(file_part - effectivePattern));
            (void)lstrcatA(buffer + cbCurr, sFileName.c_str());
            cbCurr += cch + 1;

            ++cMatches;
        }
        //while(FindNextFileA(hFind, &find_data) && cMatches != maxMatches);
		while(FindNextFileW(hFind, &find_data) && cMatches != maxMatches);

        (void)FindClose(hFind);

        if(result == 0)
        {
            /* Now expand the buffer, to fit in all the pointers. */
            size_t  cbPointers  =   (1 + cMatches + pglob->gl_offs) * sizeof(char*);
            char    *new_buffer =   (char*)realloc(buffer, cbAlloc + cbPointers);

            if(new_buffer == NULL)
            {
                result = GLOB_NOSPACE;
                if(buffer) {
                	free(buffer);
                	buffer = NULL;
                }
            }
            else
            {
                char    **pp;
                char    **begin;
                char    **end;
                char    *next_str;

                buffer = new_buffer;

                (void)memmove(new_buffer + cbPointers, new_buffer, cbAlloc);

                /* Handle the offsets. */
                begin =   (char**)new_buffer;
                end   =   begin + pglob->gl_offs;

                for(; begin != end; ++begin)
                {
                    *begin = NULL;
                }

                /* Sort, or no sort. */
                pp    =   (char**)new_buffer + pglob->gl_offs;
                begin =   pp;
                end   =   begin + cMatches;

                if(flags & GLOB_NOSORT)
                {
                    /* The way we need in order to test the removal of dots in the findfile_sequence. */
                    *end = NULL;
                    for(begin = pp, next_str = buffer + cbPointers; begin != end; --end)
                    {
                        *(end - 1) = next_str;

                        /* Find the next string. */
                        next_str += 1 + lstrlenA(next_str);
                    }
                }
                else
                {
                    /* The normal way. */
                    for(begin = pp, next_str = buffer + cbPointers; begin != end; ++begin)
                    {
                        *begin = next_str;

                        /* Find the next string. */
                        next_str += 1 + lstrlenA(next_str);
                    }
                    *begin = NULL;
                }

                /* Return results to caller. */
                pglob->gl_pathc =   (int)cMatches;
                pglob->gl_matchc=   (int)cMatches;
                pglob->gl_flags =   0;
                if(bMagic)
                {
                    pglob->gl_flags |= GLOB_MAGCHAR;
                }
                pglob->gl_pathv =   (char**)new_buffer;
            }
        }

        if(0 == cMatches)
        {
            result = GLOB_NOMATCH;
        }
    }

    if(GLOB_NOMATCH == result)
    {
        if( (flags & GLOB_TILDE_CHECK) &&
            effectivePattern == szPattern3)
        {
            result = GLOB_NOMATCH;
        }
        else if(bNoMagic ||
                (flags & GLOB_NOCHECK))
        {
            const size_t    effPattLen  =   strlen(effectivePattern);
            const size_t    cbNeeded    =   ((2 + pglob->gl_offs) * sizeof(char*)) + (1 + effPattLen);
            char            **pp        =   (char**)realloc(buffer, cbNeeded);

            if(NULL == pp)
            {
                result = GLOB_NOSPACE;
                if(buffer) {
                	free(buffer);
                	buffer = NULL;
                }
            }
            else
            {
                /* Handle the offsets. */
                char**  begin   =   pp;
                char**  end     =   pp + pglob->gl_offs;
                char*   dest    =   (char*)(pp + 2 + pglob->gl_offs);

                for(; begin != end; ++begin)
                {
                    *begin = NULL;
                }

                /* Synthesise the pattern result. */
#ifdef UNIXEM_USING_SAFE_STR_FUNCTIONS
                pp[0 + pglob->gl_offs]  =   (strcpy_s(dest, effPattLen + 1, effectivePattern), dest);
#else /* ? UNIXEM_USING_SAFE_STR_FUNCTIONS */
                pp[0 + pglob->gl_offs]  =   strcpy(dest, effectivePattern);
#endif /* UNIXEM_USING_SAFE_STR_FUNCTIONS */
                pp[1 + pglob->gl_offs]  =   NULL;

                /* Return results to caller. */
                pglob->gl_pathc =   1;
                pglob->gl_matchc=   1;
                pglob->gl_flags =   0;
                if(bMagic)
                {
                    pglob->gl_flags |= GLOB_MAGCHAR;
                }
                pglob->gl_pathv =   pp;

                result = 0;
            }
        }
    }
    else if(0 == result)
    {
        if((size_t)pglob->gl_matchc == maxMatches)
        {
            result = GLOB_NOSPACE;
        }
    }

    return result;
}

void globfree(glob_t *pglob)
{
    if(pglob != NULL)
    {
        free(pglob->gl_pathv);
        pglob->gl_pathc = 0;
        pglob->gl_pathv = NULL;
    }
}

/* ///////////////////////////// end of file //////////////////////////// */
