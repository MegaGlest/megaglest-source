/**
 * MojoSetup; a portable, flexible installation application.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

#if PLATFORM_WINDOWS

#include "platform.h"
#include "gui.h"

// Much of this file was lifted from PhysicsFS, http://icculus.org/physfs/ ...
//  I wrote all this code under the same open source license, and own the
//  copyright on it anyhow, so transferring it here is "safe".

// Forcibly disable UNICODE, since we manage this ourselves.
#ifdef UNICODE
#undef UNICODE
#endif

#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <shellapi.h>

// !!! FIXME: this is for stdio redirection crap.
#include <io.h>
#include <fcntl.h>

// is Win95/Win98/WinME?  (no Unicode, etc)
static boolean osIsWin9x = false;
static uint32 osMajorVer = 0;
static uint32 osMinorVer = 0;
static uint32 osBuildVer = 0;

static uint32 startupTime = 0;

// These allocation macros are much more complicated in PhysicsFS.
#define smallAlloc(x) xmalloc(x)
#define smallFree(x) free(x)

// ...so is this.
#define BAIL_IF_MACRO(cond, err, ret) if (cond) return ret;
#define BAIL_MACRO(err, ret) return ret;

#define LOWORDER_UINT64(pos) ((uint32) (pos & 0xFFFFFFFF))
#define HIGHORDER_UINT64(pos) ((uint32) ((pos >> 32) & 0xFFFFFFFF))

// Users without the platform SDK don't have this defined.  The original docs
//  for SetFilePointer() just said to compare with 0xFFFFFFFF, so this should
//  work as desired.
#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER 0xFFFFFFFF
#endif

// just in case...
#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES 0xFFFFFFFF
#endif

// Not defined before the Vista SDK.
#ifndef IO_REPARSE_TAG_SYMLINK
#define IO_REPARSE_TAG_SYMLINK 0xA000000C
#endif

// This is in shlobj.h, but we can't include it due to universal.h conflicts.
#ifndef CSIDL_PERSONAL
#define CSIDL_PERSONAL 0x0005
#endif

// This is in shlobj.h, but we can't include it due to universal.h conflicts.
#ifndef SHGFP_TYPE_CURRENT
#define SHGFP_TYPE_CURRENT 0x0000
#endif


// these utf-8 functions may move to mojosetup.c some day...

void utf8ToUcs2(const char *src, uint16 *dst, uint64 len)
{
    len -= sizeof (uint16);   // save room for null char.
    while (len >= sizeof (uint16))
    {
        uint32 cp = utf8codepoint(&src);
        if (cp == 0)
            break;
        else if (cp == UNICODE_BOGUS_CHAR_VALUE)
            cp = UNICODE_BOGUS_CHAR_CODEPOINT;

        // !!! FIXME: UTF-16 surrogates?
        if (cp > 0xFFFF)
            cp = UNICODE_BOGUS_CHAR_CODEPOINT;

        *(dst++) = cp;
        len -= sizeof (uint16);
    } // while

    *dst = 0;
} // utf8ToUcs2

static void utf8fromcodepoint(uint32 cp, char **_dst, uint64 *_len)
{
    char *dst = *_dst;
    uint64 len = *_len;

    if (len == 0)
        return;

    if (cp > 0x10FFFF)
        cp = UNICODE_BOGUS_CHAR_CODEPOINT;
    else if ((cp == 0xFFFE) || (cp == 0xFFFF))  // illegal values.
        cp = UNICODE_BOGUS_CHAR_CODEPOINT;
    else
    {
        // There are seven "UTF-16 surrogates" that are illegal in UTF-8.
        switch (cp)
        {
            case 0xD800:
            case 0xDB7F:
            case 0xDB80:
            case 0xDBFF:
            case 0xDC00:
            case 0xDF80:
            case 0xDFFF:
                cp = UNICODE_BOGUS_CHAR_CODEPOINT;
        } // switch
    } // else

    // Do the encoding...
    if (cp < 0x80)
    {
        *(dst++) = (char) cp;
        len--;
    } // if

    else if (cp < 0x800)
    {
        if (len < 2)
            len = 0;
        else
        {
            *(dst++) = (char) ((cp >> 6) | 128 | 64);
            *(dst++) = (char) (cp & 0x3F) | 128;
            len -= 2;
        } // else
    } // else if

    else if (cp < 0x10000)
    {
        if (len < 3)
            len = 0;
        else
        {
            *(dst++) = (char) ((cp >> 12) | 128 | 64 | 32);
            *(dst++) = (char) ((cp >> 6) & 0x3F) | 128;
            *(dst++) = (char) (cp & 0x3F) | 128;
            len -= 3;
        } // else
    } // else if

    else
    {
        if (len < 4)
            len = 0;
        else
        {
            *(dst++) = (char) ((cp >> 18) | 128 | 64 | 32 | 16);
            *(dst++) = (char) ((cp >> 12) & 0x3F) | 128;
            *(dst++) = (char) ((cp >> 6) & 0x3F) | 128;
            *(dst++) = (char) (cp & 0x3F) | 128;
            len -= 4;
        } // else if
    } // else

    *_dst = dst;
    *_len = len;
} // utf8fromcodepoint

#define UTF8FROMTYPE(typ, src, dst, len) \
    len--;  \
    while (len) \
    { \
        const uint32 cp = (uint32) *(src++); \
        if (cp == 0) break; \
        utf8fromcodepoint(cp, &dst, &len); \
    } \
    *dst = '\0'; \

void utf8FromUcs2(const uint16 *src, char *dst, uint64 len)
{
    UTF8FROMTYPE(uint64, src, dst, len);
} // utf8FromUcs4

#define UTF8_TO_UNICODE_STACK_MACRO(w_assignto, str) { \
    if (str == NULL) \
        w_assignto = NULL; \
    else { \
        const uint32 len = (uint32) ((strlen(str) * 4) + 1); \
        w_assignto = (WCHAR *) smallAlloc(len); \
        if (w_assignto != NULL) \
            utf8ToUcs2(str, (uint16 *) w_assignto, len); \
    } \
} \

static uint32 wStrLen(const WCHAR *wstr)
{
    uint32 len = 0;
    while (*(wstr++))
        len++;
    return len;
} // wStrLen


static char *unicodeToUtf8Heap(const WCHAR *w_str)
{
    char *retval = NULL;
    if (w_str != NULL)
    {
        void *ptr = NULL;
        const uint32 len = (wStrLen(w_str) * 4) + 1;
        retval = (char *) xmalloc(len);
        utf8FromUcs2((const uint16 *) w_str, retval, len);
        retval = xrealloc(retval, strlen(retval) + 1);  // shrink.
    } // if
    return retval;
} // unicodeToUtf8Heap


static char *codepageToUtf8Heap(const char *cpstr)
{
    char *retval = NULL;
    if (cpstr != NULL)
    {
        const int len = (int) (strlen(cpstr) + 1);
        WCHAR *wbuf = (WCHAR *) smallAlloc(len * sizeof (WCHAR));
        BAIL_IF_MACRO(wbuf == NULL, ERR_OUT_OF_MEMORY, NULL);
        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, cpstr, len, wbuf, len);
        retval = (char *) xmalloc(len * 4);
        utf8FromUcs2(wbuf, retval, len * 4);
        smallFree(wbuf);
    } // if
    return retval;
} // codepageToUtf8Heap


// pointers for APIs that may not exist on some Windows versions...
static HANDLE libKernel32 = NULL;
static HANDLE libUserEnv = NULL;
static HANDLE libAdvApi32 = NULL;
static HANDLE libShell32 = NULL;
static DWORD (WINAPI *pGetModuleFileNameW)(HMODULE, LPWCH, DWORD);
static BOOL (WINAPI *pGetUserProfileDirectoryW)(HANDLE, LPWSTR, LPDWORD);
static BOOL (WINAPI *pGetUserNameW)(LPWSTR, LPDWORD);
static DWORD (WINAPI *pGetFileAttributesW)(LPCWSTR);
static HANDLE (WINAPI *pFindFirstFileW)(LPCWSTR, LPWIN32_FIND_DATAW);
static BOOL (WINAPI *pFindNextFileW)(HANDLE, LPWIN32_FIND_DATAW);
static DWORD (WINAPI *pGetCurrentDirectoryW)(DWORD, LPWSTR);
static BOOL (WINAPI *pDeleteFileW)(LPCWSTR);
static BOOL (WINAPI *pRemoveDirectoryW)(LPCWSTR);
static BOOL (WINAPI *pCreateDirectoryW)(LPCWSTR, LPSECURITY_ATTRIBUTES);
static BOOL (WINAPI *pGetFileAttributesExA)
    (LPCSTR, GET_FILEEX_INFO_LEVELS, LPVOID);
static BOOL (WINAPI *pGetFileAttributesExW)
    (LPCWSTR, GET_FILEEX_INFO_LEVELS, LPVOID);
static DWORD (WINAPI *pFormatMessageW)
    (DWORD, LPCVOID, DWORD, DWORD, LPWSTR, DWORD, va_list *);
static HANDLE (WINAPI *pCreateFileW)
    (LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
static HRESULT (WINAPI *pSHGetFolderPathA)(HWND, int, HANDLE, DWORD, LPTSTR);
static BOOL (WINAPI *pMoveFileW)(LPCWSTR, LPCWSTR);
static void (WINAPI *pOutputDebugStringW)(LPCWSTR);


// Fallbacks for missing Unicode functions on Win95/98/ME. These are filled
//  into the function pointers if looking up the real Unicode entry points
//  in the system DLLs fails, so they're never used on WinNT/XP/Vista/etc.
// They make an earnest effort to convert to/from UTF-8 and UCS-2 to
//  the user's current codepage.

static BOOL WINAPI fallbackGetUserNameW(LPWSTR buf, LPDWORD len)
{
    const DWORD cplen = *len;
    char *cpstr = smallAlloc(cplen);
    BOOL retval = GetUserNameA(cpstr, len);
    if (buf != NULL)
        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, cpstr, cplen, buf, *len);
    smallFree(cpstr);
    return retval;
} // fallbackGetUserNameW

static DWORD WINAPI fallbackFormatMessageW(DWORD dwFlags, LPCVOID lpSource,
                                           DWORD dwMessageId, DWORD dwLangId,
                                           LPWSTR lpBuf, DWORD nSize,
                                           va_list *Arguments)
{
    char *cpbuf = (char *) smallAlloc(nSize);
    DWORD retval = FormatMessageA(dwFlags, lpSource, dwMessageId, dwLangId,
                                  cpbuf, nSize, Arguments);
    if (retval > 0)
        MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,cpbuf,retval,lpBuf,nSize);
    smallFree(cpbuf);
    return retval;
} // fallbackFormatMessageW

static DWORD WINAPI fallbackGetModuleFileNameW(HMODULE hMod, LPWCH lpBuf,
                                               DWORD nSize)
{
    char *cpbuf = (char *) smallAlloc(nSize);
    DWORD retval = GetModuleFileNameA(hMod, cpbuf, nSize);
    if (retval > 0)
        MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,cpbuf,retval,lpBuf,nSize);
    smallFree(cpbuf);
    return retval;
} // fallbackGetModuleFileNameW

static DWORD WINAPI fallbackGetFileAttributesW(LPCWSTR fname)
{
    DWORD retval = 0;
    const int buflen = (int) (wStrLen(fname) + 1);
    char *cpstr = (char *) smallAlloc(buflen);
    WideCharToMultiByte(CP_ACP, 0, fname, buflen, cpstr, buflen, NULL, NULL);
    retval = GetFileAttributesA(cpstr);
    smallFree(cpstr);
    return retval;
} // fallbackGetFileAttributesW

static DWORD WINAPI fallbackGetCurrentDirectoryW(DWORD buflen, LPWSTR buf)
{
    DWORD retval = 0;
    char *cpbuf = NULL;
    if (buf != NULL)
        cpbuf = (char *) smallAlloc(buflen);
    retval = GetCurrentDirectoryA(buflen, cpbuf);
    if (cpbuf != NULL)
    {
        MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,cpbuf,retval,buf,buflen);
        smallFree(cpbuf);
    } // if
    return retval;
} // fallbackGetCurrentDirectoryW

static BOOL WINAPI fallbackRemoveDirectoryW(LPCWSTR dname)
{
    BOOL retval = 0;
    const int buflen = (int) (wStrLen(dname) + 1);
    char *cpstr = (char *) smallAlloc(buflen);
    WideCharToMultiByte(CP_ACP, 0, dname, buflen, cpstr, buflen, NULL, NULL);
    retval = RemoveDirectoryA(cpstr);
    smallFree(cpstr);
    return retval;
} // fallbackRemoveDirectoryW

static BOOL WINAPI fallbackCreateDirectoryW(LPCWSTR dname, 
                                            LPSECURITY_ATTRIBUTES attr)
{
    BOOL retval = 0;
    const int buflen = (int) (wStrLen(dname) + 1);
    char *cpstr = (char *) smallAlloc(buflen);
    WideCharToMultiByte(CP_ACP, 0, dname, buflen, cpstr, buflen, NULL, NULL);
    retval = CreateDirectoryA(cpstr, attr);
    smallFree(cpstr);
    return retval;
} // fallbackCreateDirectoryW

static BOOL WINAPI fallbackDeleteFileW(LPCWSTR fname)
{
    BOOL retval = 0;
    const int buflen = (int) (wStrLen(fname) + 1);
    char *cpstr = (char *) smallAlloc(buflen);
    WideCharToMultiByte(CP_ACP, 0, fname, buflen, cpstr, buflen, NULL, NULL);
    retval = DeleteFileA(cpstr);
    smallFree(cpstr);
    return retval;
} // fallbackDeleteFileW

static HANDLE WINAPI fallbackCreateFileW(LPCWSTR fname, 
                DWORD dwDesiredAccess, DWORD dwShareMode,
                LPSECURITY_ATTRIBUTES lpSecurityAttrs,
                DWORD dwCreationDisposition,
                DWORD dwFlagsAndAttrs, HANDLE hTemplFile)
{
    HANDLE retval;
    const int buflen = (int) (wStrLen(fname) + 1);
    char *cpstr = (char *) smallAlloc(buflen);
    WideCharToMultiByte(CP_ACP, 0, fname, buflen, cpstr, buflen, NULL, NULL);
    retval = CreateFileA(cpstr, dwDesiredAccess, dwShareMode, lpSecurityAttrs,
                         dwCreationDisposition, dwFlagsAndAttrs, hTemplFile);
    smallFree(cpstr);
    return retval;
} // fallbackCreateFileW

static BOOL WINAPI fallbackMoveFileW(LPCWSTR src, LPCWSTR dst)
{
    BOOL retval;
    const int srcbuflen = (int) (wStrLen(src) + 1);
    char *srccpstr = (char *) smallAlloc(srcbuflen);
    const int dstbuflen = (int) (wStrLen(dst) + 1);
    char *dstcpstr = (char *) smallAlloc(dstbuflen);
    WideCharToMultiByte(CP_ACP,0,src,srcbuflen,srccpstr,srcbuflen,NULL,NULL);
    WideCharToMultiByte(CP_ACP,0,dst,dstbuflen,dstcpstr,dstbuflen,NULL,NULL);
    retval = MoveFileA(srccpstr, dstcpstr);
    smallFree(srccpstr);
    smallFree(dstcpstr);
    return retval;
} // fallbackMoveFileW

static void WINAPI fallbackOutputDebugStringW(LPCWSTR str)
{
    const int buflen = (int) (wStrLen(str) + 1);
    char *cpstr = (char *) smallAlloc(buflen);
    WideCharToMultiByte(CP_ACP, 0, str, buflen, cpstr, buflen, NULL, NULL);
    OutputDebugStringA(cpstr);
    smallFree(cpstr);
} // fallbackOutputDebugStringW


// A blatant abuse of pointer casting...
static int symLookup(HMODULE dll, void **addr, const char *sym)
{
    return ((*addr = GetProcAddress(dll, sym)) != NULL);
} // symLookup


static boolean findApiSymbols(void)
{
    const boolean osHasUnicode = !osIsWin9x;
    HMODULE dll = NULL;

    #define LOOKUP_NOFALLBACK(x, reallyLook) { \
        if (reallyLook) \
            symLookup(dll, (void **) &p##x, #x); \
        else \
            p##x = NULL; \
    }

    #define LOOKUP(x, reallyLook) { \
        if ((!reallyLook) || (!symLookup(dll, (void **) &p##x, #x))) \
            p##x = fallback##x; \
    }

    // Apparently Win9x HAS the Unicode entry points, they just don't WORK.
    //  ...so don't look them up unless we're on NT+. (see osHasUnicode.)

    dll = libUserEnv = LoadLibraryA("userenv.dll");
    if (dll != NULL)
        LOOKUP_NOFALLBACK(GetUserProfileDirectoryW, osHasUnicode);

    // !!! FIXME: what do they call advapi32.dll on Win64?
    dll = libAdvApi32 = LoadLibraryA("advapi32.dll");
    if (dll != NULL)
        LOOKUP(GetUserNameW, osHasUnicode);

    // !!! FIXME: what do they call kernel32.dll on Win64?
    dll = libKernel32 = LoadLibraryA("kernel32.dll");
    if (dll != NULL)
    {
        LOOKUP_NOFALLBACK(GetFileAttributesExA, 1);
        LOOKUP_NOFALLBACK(GetFileAttributesExW, osHasUnicode);
        LOOKUP_NOFALLBACK(FindFirstFileW, osHasUnicode);
        LOOKUP_NOFALLBACK(FindNextFileW, osHasUnicode);
        LOOKUP(GetModuleFileNameW, osHasUnicode);
        LOOKUP(FormatMessageW, osHasUnicode);
        LOOKUP(GetFileAttributesW, osHasUnicode);
        LOOKUP(GetCurrentDirectoryW, osHasUnicode);
        LOOKUP(CreateDirectoryW, osHasUnicode);
        LOOKUP(RemoveDirectoryW, osHasUnicode);
        LOOKUP(CreateFileW, osHasUnicode);
        LOOKUP(DeleteFileW, osHasUnicode);
        LOOKUP(MoveFileW, osHasUnicode);
        LOOKUP(OutputDebugStringW, osHasUnicode);
    } // if

    // !!! FIXME: what do they call shell32.dll on Win64?
    dll = libShell32 = LoadLibraryA("shell32.dll");
    if (dll != NULL)
        LOOKUP_NOFALLBACK(SHGetFolderPathA, 1);

    #undef LOOKUP_NOFALLBACK
    #undef LOOKUP

    return true;
} // findApiSymbols



// ok, now the actual platform layer implementation...

boolean MojoPlatform_istty(void)
{
    return (GetConsoleWindow() != 0);
} // MojoPlatform_istty


boolean MojoPlatform_launchBrowser(const char *url)
{
    // msdn says:
    // "Returns a value greater than 32 if successful, or an error value that
    //  is less than or equal to 32 otherwise."
    return (((int) ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL)) > 32);
} // MojoPlatform_launchBrowser


boolean MojoPlatform_installDesktopMenuItem(const char *data)
{
    // !!! FIXME: write me.
    STUBBED("desktop menu support");
    return false;
} // MojoPlatform_installDesktopMenuItem


boolean MojoPlatform_uninstallDesktopMenuItem(const char *data)
{
    // !!! FIXME: write me.
    STUBBED("desktop menu support");
    return false;
} // MojoPlatform_uninstallDesktopMenuItem


int MojoPlatform_exec(const char *cmd)
{
    STUBBED("exec");
    return 127;
} // MojoPlatform_exec


int MojoPlatform_runScript(const char *script, boolean devnull, const char **argv)
{
    STUBBED("runScript");
    return 0;
}

boolean MojoPlatform_spawnTerminal(void)
{
    assert(!MojoPlatform_istty());

    if (AllocConsole())
    {
        int hCrt;
        FILE *hf;
        hCrt = _open_osfhandle((long) GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT);
        hf = _fdopen( hCrt, "w" );
        *stdout = *hf;
        setvbuf( stdout, NULL, _IONBF, 0 ); 

        hCrt = _open_osfhandle((long) GetStdHandle(STD_ERROR_HANDLE), _O_TEXT);
        hf = _fdopen( hCrt, "w" );
        *stderr = *hf;
        setvbuf( stderr, NULL, _IONBF, 0 ); 

        hCrt = _open_osfhandle((long) GetStdHandle(STD_INPUT_HANDLE), _O_TEXT);
        hf = _fdopen( hCrt, "r" );
        *stdin = *hf;
        setvbuf( stdin, NULL, _IONBF, 0 ); 

		return true;
	} // if

    return false;
} // MojoPlatform_spawnTerminal


uint8 *MojoPlatform_decodeImage(const uint8 *data, uint32 size,
                                uint32 *w, uint32 *h)
{
    return NULL;  // !!! FIXME: try IPicture?
} // MojoPlatform_decodeImage


char *MojoPlatform_currentWorkingDir(void)
{
    char *retval = NULL;
    WCHAR *wbuf = NULL;
    DWORD buflen = 0;

    buflen = pGetCurrentDirectoryW(buflen, NULL);
    wbuf = (WCHAR *) smallAlloc((buflen + 2) * sizeof (WCHAR));
    BAIL_IF_MACRO(wbuf == NULL, ERR_OUT_OF_MEMORY, NULL);
    pGetCurrentDirectoryW(buflen, wbuf);

    if (wbuf[buflen - 2] == '\\')
        wbuf[buflen - 1] = '\0';  // just in case...
    else
    {
        wbuf[buflen - 1] = '\\'; 
        wbuf[buflen] = '\0'; 
    } // else

    retval = unicodeToUtf8Heap(wbuf);
    smallFree(wbuf);
    return retval;
} // MojoPlatform_currentWorkingDir


char *MojoPlatform_readlink(const char *linkname)
{
    STUBBED("should use symlinks (reparse points) on Vista");
    return NULL;
} // MojoPlatform_readlink


// !!! FIXME: this code sort of sucks.
char *MojoPlatform_realpath(const char *path)
{
    // !!! FIXME: this should return NULL if (path) doesn't exist?
    // !!! FIXME: Need to handle symlinks in Vista...
    // !!! FIXME: try GetFullPathName() instead?

    // this function should be UTF-8 clean.
    char *retval = NULL;
    char *p = NULL;

    if ((path == NULL) || (*path == '\0'))
        return NULL;

    retval = (char *) xmalloc(MAX_PATH);

    // If in \\server\path format, it's already an absolute path.
    //  We'll need to check for "." and ".." dirs, though, just in case.
    if ((path[0] == '\\') && (path[1] == '\\'))
        strcpy(retval, path);

    else
    {
        char *currentDir = MojoPlatform_currentWorkingDir();
        if (currentDir == NULL)
        {
            free(retval);
            return NULL;
        } // if

        if (path[1] == ':')   // drive letter specified?
        {
            // Apparently, "D:mypath" is the same as "D:\\mypath" if
            //  D: is not the current drive. However, if D: is the
            //  current drive, then "D:mypath" is a relative path. Ugh.

            if (path[2] == '\\')  // maybe an absolute path?
                strcpy(retval, path);
            else  // definitely an absolute path.
            {
                if (path[0] == currentDir[0]) // current drive; relative.
                {
                    strcpy(retval, currentDir);
                    strcat(retval, path + 2);
                } // if

                else  // not current drive; absolute.
                {
                    retval[0] = path[0];
                    retval[1] = ':';
                    retval[2] = '\\';
                    strcpy(retval + 3, path + 2);
                } // else
            } // else
        } // if

        else  // no drive letter specified.
        {
            if (path[0] == '\\')  // absolute path.
            {
                retval[0] = currentDir[0];
                retval[1] = ':';
                strcpy(retval + 2, path);
            } // if
            else
            {
                strcpy(retval, currentDir);
                strcat(retval, path);
            } // else
        } // else

        free(currentDir);
    } // else

    // (whew.) Ok, now take out "." and ".." path entries...

    p = retval;
    while ( (p = strstr(p, "\\.")) != NULL)
    {
        // it's a "." entry that doesn't end the string.
        if (p[2] == '\\')
            memmove(p + 1, p + 3, strlen(p + 3) + 1);

        // it's a "." entry that ends the string.
        else if (p[2] == '\0')
            p[0] = '\0';

        // it's a ".." entry.
        else if (p[2] == '.')
        {
            char *prevEntry = p - 1;
            while ((prevEntry != retval) && (*prevEntry != '\\'))
                prevEntry--;

            if (prevEntry == retval)  // make it look like a "." entry.
                memmove(p + 1, p + 2, strlen(p + 2) + 1);
            else
            {
                if (p[3] != '\0') // doesn't end string.
                    *prevEntry = '\0';
                else // ends string.
                    memmove(prevEntry + 1, p + 4, strlen(p + 4) + 1);

                p = prevEntry;
            } // else
        } // else if

        else
        {
            p++;  // look past current char.
        } // else
    } // while

    // shrink the retval's memory block if possible...
    return (char *) xrealloc(retval, strlen(retval) + 1);
} // MojoPlatform_realpath


char *MojoPlatform_appBinaryPath(void)
{
    DWORD buflen = 64;
    LPWSTR modpath = NULL;
    char *retval = NULL;
    DWORD rc = 0;

    while (true)
    {
        void *ptr = xrealloc(modpath, buflen * sizeof(WCHAR));

        modpath = (LPWSTR) ptr;

        rc = pGetModuleFileNameW(NULL, modpath, buflen);
        if (rc == 0)
        {
            free(modpath);
            return NULL;
        } // if

        if (rc < buflen)
        {
            buflen = rc;
            break;
        } // if

        buflen *= 2;
    } // while

    if (buflen > 0)  // just in case...
    {
        WCHAR *ptr = (modpath + buflen) - 1;
        while (ptr != modpath)
        {
            if (*ptr == '\\')
                break;
            ptr--;
        } // while

        if ((ptr != modpath) || (*ptr == '\\'))  // should always be true...
        {
            *(ptr + 1) = '\0';  // chop off filename.
            retval = unicodeToUtf8Heap(modpath);
        } // else
    } // else

    free(modpath);
    return retval;
} // MojoPlatform_appBinaryPath


// Try to make use of GetUserProfileDirectoryW(), which isn't available on
//  some common variants of Win32. If we can't use this, we just punt and
//  use the binary path for the user dir, too.
//
// On success, module-scope variable (userDir) will have a pointer to
//  a malloc()'d string of the user's profile dir, and a non-zero value is
//  returned. If we can't determine the profile dir, (userDir) will
//  be NULL, and zero is returned.

char *MojoPlatform_homedir(void)
{
    char *userDir = NULL;

    // GetUserProfileDirectoryW() is only available on NT 4.0 and later.
    //  This means Win95/98/ME (and CE?) users have to do without, so for
    //  them, we'll default to the base directory when we can't get the
    //  function pointer. Since this is originally an NT API, we don't
	//  offer a non-Unicode fallback.

    if (pGetUserProfileDirectoryW != NULL)
    {
        HANDLE accessToken = NULL;       // Security handle to process
        HANDLE processHandle = GetCurrentProcess();
        if (OpenProcessToken(processHandle, TOKEN_QUERY, &accessToken))
        {
            DWORD psize = 0;
            WCHAR dummy = 0;
            LPWSTR wstr = NULL;
            BOOL rc = 0;

            // Should fail. Will write the size of the profile path in
            //  psize. Also note that the second parameter can't be
            //  NULL or the function fails.
    		rc = pGetUserProfileDirectoryW(accessToken, &dummy, &psize);
            if (rc == 0)  // this should always be true!
            {
                // Allocate memory for the profile directory
                wstr = (LPWSTR) smallAlloc(psize * sizeof (WCHAR));
                if (wstr != NULL)
                {
                    if (pGetUserProfileDirectoryW(accessToken, wstr, &psize))
                        userDir = unicodeToUtf8Heap(wstr);
                    smallFree(wstr);
                } // else
            } // if

            CloseHandle(accessToken);
        } // if
    } // if

    if (userDir == NULL)  // couldn't get profile for some reason.
    {
        // Might just be a non-NT system; try SHGetFolderPathA()...
        if (pSHGetFolderPathA != NULL)  // can be NULL if IE5+ isn't installed!
        {
            char shellPath[MAX_PATH];
            HRESULT status = pSHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL,
                                               SHGFP_TYPE_CURRENT, shellPath);
            if (SUCCEEDED(status))
                userDir = codepageToUtf8Heap(shellPath);
        } // if
    } // if

    if (userDir == NULL)  // Still nothing?!
    {
        // this either means we had a catastrophic failure, or we're on a
        // Win95 system without at least Internet Explorer 5.0. Bleh!
        userDir = xstrdup("C:\\My Documents");  // good luck with that.
    } // if

    return userDir;
} // MojoPlatform_homedir


// This implementation is a bit naive.
char *MojoPlatform_locale(void)
{
    char lang[16];
    char country[16];

	const int langrc = GetLocaleInfoA(LOCALE_USER_DEFAULT,
                                      LOCALE_SISO639LANGNAME,
                                      lang, sizeof (lang));

	const int ctryrc =  GetLocaleInfoA(LOCALE_USER_DEFAULT,
                                       LOCALE_SISO3166CTRYNAME,
                                       country, sizeof (country));

    // Win95 systems will fail, because they don't have LOCALE_SISO*NAME ...
    if ((langrc != 0) && (ctryrc != 0))
        return format("%0_%1", lang, country);

    return NULL;
} // MojoPlatform_locale


char *MojoPlatform_osType(void)
{
    if (osIsWin9x)
        return xstrdup("win9x");
    else
        return xstrdup("winnt");

    return NULL;
} // MojoPlatform_ostype


char *MojoPlatform_osVersion(void)
{
    return format("%0.%1.%2",
                  numstr(osMajorVer),
                  numstr(osMinorVer),
                  numstr(osBuildVer));
} // MojoPlatform_osversion


char *MojoPlatform_osMachine(void)
{
    // !!! FIXME: return "x86" for win32 and "x64" (bleh!) for win64.
    return NULL;
} // MojoPlatform_osMachine


void MojoPlatform_sleep(uint32 ticks)
{
    Sleep(ticks);
} // MojoPlatform_sleep


uint32 MojoPlatform_ticks(void)
{
    return GetTickCount() - startupTime;
} // MojoPlatform_ticks


void MojoPlatform_die(void)
{
    STUBBED("Win32 equivalent of _exit()?");
    _exit(86);
} // MojoPlatform_die


boolean MojoPlatform_unlink(const char *fname)
{
    int retval = 0;
    LPWSTR wpath;
    UTF8_TO_UNICODE_STACK_MACRO(wpath, fname);
    if (pGetFileAttributesW(wpath) == FILE_ATTRIBUTE_DIRECTORY)
        retval = (pRemoveDirectoryW(wpath) != 0);
    else
        retval = (pDeleteFileW(wpath) != 0);
    smallFree(wpath);
    return retval;
} // MojoPlatform_unlink


boolean MojoPlatform_symlink(const char *src, const char *dst)
{
    STUBBED("Vista has symlink support");
    return false;
} // MojoPlatform_symlink


boolean MojoPlatform_mkdir(const char *path, uint16 perms)
{
    // !!! FIXME: error if already exists?
    // !!! FIXME: perms?
    WCHAR *wpath;
    DWORD rc;
    UTF8_TO_UNICODE_STACK_MACRO(wpath, path);
    rc = pCreateDirectoryW(wpath, NULL);
    smallFree(wpath);
    return (rc != 0);
} // MojoPlatform_mkdir


boolean MojoPlatform_rename(const char *src, const char *dst)
{
    WCHAR *srcwpath;
    WCHAR *dstwpath;
    BOOL rc;
    MojoPlatform_unlink(dst);  // to match Unix rename()...
    UTF8_TO_UNICODE_STACK_MACRO(srcwpath, src);
    UTF8_TO_UNICODE_STACK_MACRO(dstwpath, dst);
    rc = pMoveFileW(srcwpath, dstwpath);
    smallFree(srcwpath);
    smallFree(dstwpath);
    return (rc != 0);
} // MojoPlatform_rename


boolean MojoPlatform_exists(const char *dir, const char *fname)
{
    WCHAR *wpath;
    char *fullpath = NULL;
    boolean retval = false;

    if (fname == NULL)
        fullpath = xstrdup(dir);
    else
    {
        const size_t len = strlen(dir) + strlen(fname) + 1;
        fullpath = (char *) xmalloc(len);
        snprintf(fullpath, len, "%s\\%s", dir, fname);
    } // else

    UTF8_TO_UNICODE_STACK_MACRO(wpath, fullpath);
    retval = (pGetFileAttributesW(wpath) != INVALID_FILE_ATTRIBUTES);
    smallFree(wpath);
    free(fullpath);

    return retval;
} // MojoPlatform_exists


boolean MojoPlatform_writable(const char *fname)
{
    boolean retval = false;
    DWORD attr = 0;
    WCHAR *wpath;
    UTF8_TO_UNICODE_STACK_MACRO(wpath, fname);
    attr = pGetFileAttributesW(wpath);
    smallFree(wpath);
    if (attr != INVALID_FILE_ATTRIBUTES)
        retval = ((attr & FILE_ATTRIBUTE_READONLY) == 0);
    return retval;
} // MojoPlatform_writable


boolean MojoPlatform_isdir(const char *dir)
{
    boolean retval = false;
    LPWSTR wpath;
    UTF8_TO_UNICODE_STACK_MACRO(wpath, dir);
    retval = ((pGetFileAttributesW(wpath) & FILE_ATTRIBUTE_DIRECTORY) != 0);
    smallFree(wpath);
    return retval;
} // MojoPlatform_isdir


static boolean isSymlinkAttrs(const DWORD attr, const DWORD tag)
{
    return ( (attr & FILE_ATTRIBUTE_REPARSE_POINT) && 
             (tag == IO_REPARSE_TAG_SYMLINK) );
} // isSymlinkAttrs


boolean MojoPlatform_issymlink(const char *fname)
{
    // !!! FIXME:
    // Windows Vista can have NTFS symlinks. Can older Windows releases have
    //  them when talking to a network file server? What happens when you
    //  mount a NTFS partition on XP that was plugged into a Vista install
    //  that made a symlink?

    boolean retval = false;
    LPWSTR wpath;
    HANDLE dir;
    WIN32_FIND_DATAW entw;

    // no unicode entry points? Probably no symlinks.
    if (pFindFirstFileW == NULL)
        return false;

    // !!! FIXME: filter wildcard chars?
    UTF8_TO_UNICODE_STACK_MACRO(wpath, fname);
    dir = pFindFirstFileW(wpath, &entw);
    smallFree(wpath);

    if (dir != INVALID_HANDLE_VALUE)
    {
        retval = isSymlinkAttrs(entw.dwFileAttributes, entw.dwReserved0);
        FindClose(dir);
    } // if

    return retval;
} // MojoPlatform_issymlink


boolean MojoPlatform_isfile(const char *dir)
{
    return ((!MojoPlatform_isdir(dir)) && (!MojoPlatform_issymlink(dir)));
} // MojoPlatform_isfile


void *MojoPlatform_stdout(void)
{
    return NULL;  // unsupported on Windows.
} // MojoPlatform_stdout


void *MojoPlatform_open(const char *fname, uint32 flags, uint16 mode)
{
    HANDLE *retval = NULL;
    DWORD accessMode = 0;
    DWORD createMode = 0;
    DWORD shareMode = FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE;
    WCHAR *wpath = NULL;
    HANDLE fd = 0;

    if (flags & MOJOFILE_READ)
        accessMode |= GENERIC_READ;
    if (flags & MOJOFILE_WRITE)
        accessMode |= GENERIC_WRITE;
    if (accessMode == 0)
        return NULL;  // have to specify SOMETHING.

//    if (flags & MOJOFILE_APPEND)
//        unixflags |= O_APPEND;

    if ((flags & MOJOFILE_CREATE) && (flags & MOJOFILE_EXCLUSIVE))
        createMode = CREATE_NEW;
    else if ((flags & MOJOFILE_CREATE) && (flags & MOJOFILE_TRUNCATE))
        createMode = CREATE_ALWAYS;
    else if (flags & MOJOFILE_CREATE)
        createMode = OPEN_ALWAYS;
    else if (flags & MOJOFILE_TRUNCATE)
        createMode = TRUNCATE_EXISTING;
    else
        createMode = OPEN_EXISTING;

    // !!! FIXME
    STUBBED("file permissions");

    UTF8_TO_UNICODE_STACK_MACRO(wpath, fname);
    fd = pCreateFileW(wpath, accessMode, shareMode, NULL, createMode,
                      FILE_ATTRIBUTE_NORMAL, NULL);
    smallFree(wpath);

    if (fd != INVALID_HANDLE_VALUE)
    {
        retval = (HANDLE *) xmalloc(sizeof (HANDLE));
        *retval = fd;
    } // if

    return retval;
} // MojoPlatform_open


int64 MojoPlatform_read(void *fd, void *buf, uint32 bytes)
{
    HANDLE handle = *((HANDLE *) fd);
    DWORD br = 0;

    // Read data from the file
    // !!! FIXME: uint32 might be a greater # than DWORD
    if(!ReadFile(handle, buf, bytes, &br, NULL))
        return -1;

    return (int64) br;
} // MojoPlatform_read


int64 MojoPlatform_write(void *fd, const void *buf, uint32 bytes)
{
    HANDLE handle = *((HANDLE *) fd);
    DWORD bw = 0;

    // Read data from the file
    // !!! FIXME: uint32 might be a greater # than DWORD
    if(!WriteFile(handle, buf, bytes, &bw, NULL))
        return -1;

    return (int64) bw;
} // MojoPlatform_write


int64 MojoPlatform_tell(void *fd)
{
    return MojoPlatform_seek(fd, 0, MOJOSEEK_CURRENT);
} // MojoPlatform_tell


int64 MojoPlatform_seek(void *fd, int64 pos, MojoFileSeek whence)
{
    HANDLE handle = *((HANDLE *) fd);
    DWORD highpos = HIGHORDER_UINT64(pos);
    const DWORD lowpos = LOWORDER_UINT64(pos);
    DWORD winwhence = 0;
    DWORD rc = 0;

    switch (whence)
    {
        case MOJOSEEK_SET: winwhence = FILE_BEGIN; break;
        case MOJOSEEK_CURRENT: winwhence = FILE_CURRENT; break;
        case MOJOSEEK_END: winwhence = FILE_END; break;
        default: return -1;  // !!! FIXME: maybe just abort?
    } // switch

    rc = SetFilePointer(handle, lowpos, &highpos, winwhence);
    if ( (rc == INVALID_SET_FILE_POINTER) && (GetLastError() != NO_ERROR) )
        return -1;

    return (int64) ((((uint64) highpos) << 32) | ((uint64) rc));
} // MojoPlatform_seek


int64 MojoPlatform_flen(void *fd)
{
    HANDLE handle = *((HANDLE *) fd);
    int64 retval = 0;
    DWORD SizeHigh = 0;
    DWORD SizeLow = GetFileSize(handle, &SizeHigh);

    if ((SizeLow == INVALID_SET_FILE_POINTER) && (GetLastError() != NO_ERROR))
        return -1;
    else
    {
        // Combine the high/low order to create the 64-bit position value
        retval = (int64) ((((uint64) SizeHigh) << 32) | ((uint64) SizeLow));
        assert(retval >= 0);
    } // else

    return retval;
} // MojoPlatform_flen


boolean MojoPlatform_flush(void *fd)
{
    HANDLE handle = *((HANDLE *) fd);
    return (FlushFileBuffers(handle) != 0);
} // MojoPlatform_flush


boolean MojoPlatform_close(void *fd)
{
    HANDLE handle = *((HANDLE *) fd);
    boolean retval = (CloseHandle(handle) != 0);
    if (retval)
        free(fd);
    return retval;
} // MojoPlatform_close


typedef struct
{
    HANDLE dir;
    boolean done;
    WIN32_FIND_DATA ent;
    WIN32_FIND_DATAW entw;
} WinApiDir;

void *MojoPlatform_opendir(const char *dirname)
{
    const int unicode = (pFindFirstFileW != NULL) && (pFindNextFileW != NULL);
    size_t len = strlen(dirname);
    char *searchPath = NULL;
    WCHAR *wSearchPath = NULL;
    WinApiDir *retval = (WinApiDir *) xmalloc(sizeof (WinApiDir));

    retval->dir = INVALID_HANDLE_VALUE;
    retval->done = false;

    // Allocate a new string for path, maybe '\\', "*", and NULL terminator
    searchPath = (char *) smallAlloc(len + 3);

    // Copy current dirname
    strcpy(searchPath, dirname);

    // if there's no '\\' at the end of the path, stick one in there.
    if (searchPath[len - 1] != '\\')
    {
        searchPath[len++] = '\\';
        searchPath[len] = '\0';
    } // if

    // Append the "*" to the end of the string
    strcat(searchPath, "*");

    UTF8_TO_UNICODE_STACK_MACRO(wSearchPath, searchPath);

    if (unicode)
        retval->dir = pFindFirstFileW(wSearchPath, &retval->entw);
    else
    {
        const int len = (int) (wStrLen(wSearchPath) + 1);
        char *cp = (char *) smallAlloc(len);
        WideCharToMultiByte(CP_ACP, 0, wSearchPath, len, cp, len, 0, 0);
        retval->dir = FindFirstFileA(cp, &retval->ent);
        smallFree(cp);
    } // else

    smallFree(wSearchPath);
    smallFree(searchPath);
    if (retval->dir == INVALID_HANDLE_VALUE)
    {
        free(retval);
        return NULL;
    } // if

    return retval;
} // MojoPlatform_opendir


char *MojoPlatform_readdir(void *_dirhandle)
{
    const int unicode = (pFindFirstFileW != NULL) && (pFindNextFileW != NULL);
    WinApiDir *dir = (WinApiDir *) _dirhandle;
    char *utf8 = NULL;

    if (dir->done)
        return NULL;

    if (unicode)
    {
        do
        {
            const DWORD attr = dir->entw.dwFileAttributes;
            const DWORD tag = dir->entw.dwReserved0;
            const WCHAR *fn = dir->entw.cFileName;
            if ((fn[0] == '.') && (fn[1] == '\0'))
                continue;
            if ((fn[0] == '.') && (fn[1] == '.') && (fn[2] == '\0'))
                continue;

            utf8 = unicodeToUtf8Heap(fn);
            dir->done = (pFindNextFileW(dir->dir, &dir->entw) == 0);
            return utf8;
        } while (pFindNextFileW(dir->dir, &dir->entw) != 0);
    } // if

    else  // ANSI fallback.
    {
        do
        {
            const DWORD attr = dir->ent.dwFileAttributes;
            const DWORD tag = dir->ent.dwReserved0;
            const char *fn = dir->ent.cFileName;
            if ((fn[0] == '.') && (fn[1] == '\0'))
                continue;
            if ((fn[0] == '.') && (fn[1] == '.') && (fn[2] == '\0'))
                continue;

            utf8 = codepageToUtf8Heap(fn);
            dir->done = (FindNextFileA(dir->dir, &dir->ent) == 0);
            return utf8;
        } while (FindNextFileA(dir->dir, &dir->ent) != 0);
    } // else

    dir->done = true;
    return NULL;
} // MojoPlatform_readdir


void MojoPlatform_closedir(void *dirhandle)
{
    WinApiDir *dir = (WinApiDir *) dirhandle;
    if (dir)
    {
        FindClose(dir->dir);
        free(dir);
    } // if
} // MojoPlatform_closedir


int64 MojoPlatform_filesize(const char *fname)
{
    // !!! FIXME: this is lame.
    int64 retval = -1;
    void *fd = MojoPlatform_open(fname, MOJOFILE_READ, 0);
    STUBBED("use a stat()-like thing instead");
    if (fd != NULL)
    {
        retval = MojoPlatform_seek(fd, 0, MOJOSEEK_END);
        MojoPlatform_close(fd);
    } // if

    return retval;
} // MojoPlatform_filesize


boolean MojoPlatform_perms(const char *fname, uint16 *p)
{
    STUBBED("Windows permissions");
    *p = 0;
    return true;
} // MojoPlatform_perms


uint16 MojoPlatform_defaultFilePerms(void)
{
    STUBBED("Windows permissions");
    return 0644;
} // MojoPlatform_defaultFilePerms


uint16 MojoPlatform_defaultDirPerms(void)
{
    STUBBED("Windows permissions");
    return 0755;
} // MojoPlatform_defaultDirPerms


uint16 MojoPlatform_makePermissions(const char *str, boolean *_valid)
{
    STUBBED("Windows permissions");
    *_valid = true;
    return 0;
} // MojoPlatform_makePermissions


boolean MojoPlatform_chmod(const char *fname, uint16 p)
{
    STUBBED("Windows permissions");
    return true;
    //return (chmod(fname, p) != -1);
} // MojoPlatform_chmod


static BOOL mediaInDrive(const char *drive)
{
    // Prevent windows warning message appearing when checking media size
    UINT oldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    DWORD tmp = 0;
    // If the function succeeds, there's media in the drive
    BOOL retval = GetVolumeInformationA(drive,NULL,0,NULL,NULL,&tmp,NULL,0);

    // Revert back to old windows error handler
    SetErrorMode(oldErrorMode);
    return retval;
} // mediaInDrive


char *MojoPlatform_findMedia(const char *uniquefile)
{
    // !!! FIXME: Probably shouldn't just check drive letters...

    char drive_str[4] = { 'x', ':', '\\', '\0' };
    char ch;
    UINT rc;
    boolean found = false;

    for (ch = 'A'; ch <= 'Z'; ch++)
    {
        drive_str[0] = ch;
        rc = GetDriveTypeA(drive_str);
        if ((rc != DRIVE_UNKNOWN) && (rc != DRIVE_NO_ROOT_DIR))
        {
            if (mediaInDrive(drive_str))
            {
                drive_str[2] = '\0';
                found = (MojoPlatform_exists(drive_str, uniquefile));
                drive_str[2] = '\\';
                if (found)
                    return xstrdup(drive_str);
            } // if
        } // if
    } // for

    return NULL;
} // MojoPlatform_findMedia


void MojoPlatform_log(const char *str)
{
    if (pOutputDebugStringW != NULL) // in case this gets called before init...
    {
        static const WCHAR endl[3] = { '\r', '\n', '\0' };
        WCHAR *wstr;
        UTF8_TO_UNICODE_STACK_MACRO(wstr, str);
        STUBBED("OutputDebugString() is probably not best here");
        pOutputDebugStringW(wstr);
        pOutputDebugStringW(endl);
        smallFree(wstr);
    } // if
} // MojoPlatform_log


typedef struct
{
    HINSTANCE dll;
    HANDLE file;
} WinApiDll;

void *MojoPlatform_dlopen(const uint8 *img, size_t len)
{
    WinApiDll *retval = NULL;
    char path[MAX_PATH];
    char fname[MAX_PATH];
    DWORD bw = 0;
    HANDLE handle;
    HINSTANCE dll;

    GetTempPath(sizeof (path), path);
    GetTempFileName(path, "mojosetup-plugin-", 0, fname);

    handle = CreateFileA(fname, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (handle == INVALID_HANDLE_VALUE)
        return NULL;

    WriteFile(handle, img, len, &bw, NULL);  // dump it to the temp file.

    CloseHandle(handle);
    if (bw != len)
    {
        DeleteFile(fname);
        return NULL;
    } // if

    // The DELETE_ON_CLOSE will cause the kernel to remove the file when
    //  we're done with it, including manually closing or the process
    //  terminating (including crashing). We hold this handle until we close
    //  the library.
    handle = CreateFileA(fname, GENERIC_READ, FILE_SHARE_READ, NULL,
                         OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE, NULL);
    if (handle == INVALID_HANDLE_VALUE)
    {
        DeleteFile(fname);
        return NULL;
    } // if

    dll = LoadLibraryA(fname);
    if (dll == NULL)
    {
        CloseHandle(handle);  // (also deletes temp file.)
        return NULL;
    } // if

    retval = (WinApiDll *) xmalloc(sizeof (WinApiDll));
    retval->dll = dll;
    retval->file = handle;
    return retval;
} // MojoPlatform_dlopen


void *MojoPlatform_dlsym(void *_lib, const char *sym)
{
    const WinApiDll *lib = (const WinApiDll *) _lib;
    return ((lib) ? GetProcAddress(lib->dll, sym) : NULL);
} // MojoPlatform_dlsym


void MojoPlatform_dlclose(void *_lib)
{
    WinApiDll *lib = (WinApiDll *) _lib;
    if (lib)
    {
        FreeLibrary(lib->dll);
        CloseHandle(lib->file);  // this also deletes the temp file.
        free(lib);
    } // if
} // MojoPlatform_dlclose


uint64 MojoPlatform_getuid(void)
{
    return 0;  // !!! FIXME
} // MojoPlatform_getuid


uint64 MojoPlatform_geteuid(void)
{
    return 0;  // !!! FIXME
} // MojoPlatform_geteuid


uint64 MojoPlatform_getgid(void)
{
    return 0;  // !!! FIXME
} // MojoPlatform_getgid



// Get OS info and save the important parts.
//  Returns non-zero if successful, otherwise it returns zero on failure.
static boolean getOSInfo(void)
{
    OSVERSIONINFO osVerInfo;
    osVerInfo.dwOSVersionInfoSize = sizeof(osVerInfo);
    if (!GetVersionEx(&osVerInfo))
        return false;

    osIsWin9x = (osVerInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);
    osMajorVer = (uint32) osVerInfo.dwMajorVersion;
    osMinorVer = (uint32) osVerInfo.dwMinorVersion;
    osBuildVer = (uint32) osVerInfo.dwBuildNumber;

    return true;
} // getOSInfo


static boolean platformInit(void)
{
    startupTime = GetTickCount();

    if (!getOSInfo())
        return false;

    if (!findApiSymbols())
        return false;

    return true;
} // platformInit


static void platformDeinit(void)
{
    HANDLE *libs[] = { &libKernel32, &libUserEnv, &libAdvApi32, &libShell32 };
    int i;

    for (i = 0; i < (sizeof (libs) / sizeof (libs[0])); i++)
    {
        const HANDLE lib = *(libs[i]);
        if (lib)
        {
            FreeLibrary(lib);
            *(libs[i]) = NULL;
        } // if
    } // for
} // platformDeinit


static void buildCommandlineArray(LPSTR szCmd, int *_argc, char ***_argv)
{
    int argc = 0;
    char **argv = NULL;

    // !!! FIXME: STUBBED("parse command line string into array");

    *_argc = argc;
    *_argv = argv;
} // buildCommandlineArray


static void freeCommandlineArray(int argc, char **argv)
{
    while (argc--)
        free(argv[argc]);
    free(argv);
} // freeCommandlineArray


int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmd, int nCmdShow)
{
    int retval = 0;
    int argc = 0;
    char **argv = NULL;

    if (!platformInit())
        retval = 1;
    else
    {
        buildCommandlineArray(szCmd, &argc, &argv);
        //openlog("mojosetup", LOG_PID, LOG_USER);
        //atexit(closelog);
        STUBBED("signal handlers");
        //install_signals();
        retval = MojoSetup_main(argc, argv);
        freeCommandlineArray(argc, argv);

        platformDeinit();
    } // else

    return retval;
} // main

#endif  // PLATFORM_WINDOWS

// end of windows.c ...

