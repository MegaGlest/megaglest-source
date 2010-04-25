/**
 * MojoSetup; a portable, flexible installation application.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

#ifndef _INCL_UNIVERSAL_H_
#define _INCL_UNIVERSAL_H_

// Include this file from everywhere...it provides basic type sanity, etc.

// Include the Holy Trinity...
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// and some others...
#include <assert.h>
#include <time.h>  // !!! FIXME: maybe use this in platform layer?

// Windows system headers conflict with MojoSetup typedefs, so chop out
//  all the massive and unnecessary dependencies that windows.h pulls in...
#if PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN 1
#endif

#if _MSC_VER
#include <malloc.h>  // need this to get alloca() in MSVC.
// !!! FIXME: temporary solution.
#define snprintf _snprintf
#define strcasecmp(x,y) _stricmp(x,y)
#endif

#ifdef __cplusplus
extern "C" {
#endif

// !!! FIXME: bad.
typedef char int8;
typedef unsigned char uint8;
typedef short int16;
typedef unsigned short uint16;
typedef int int32;
typedef unsigned int uint32;
typedef long long int64;
typedef unsigned long long uint64;

// These are likely to get stolen by some overzealous library header...
#ifdef boolean
#error Something is defining boolean. Resolve this before including universal.h.
#endif
#ifdef true
#error Something is defining true. Resolve this before including universal.h.
#endif
#ifdef false
#error Something is defining false. Resolve this before including universal.h.
#endif

typedef int boolean;
#define true 1
#define false 0

// MSVC doesn't support the "inline" keyword for normal C sources, just C++.
#if defined(_MSC_VER) && !defined(__cplusplus) && !defined(inline)
#define inline __inline
#endif

// Compiler-enforced printf() safety helper.
// This is appended to function declarations that use printf-style semantics,
//  and will make sure your passed the right params to "..." for the
//  format you specified, so gcc can catch programmer bugs at compile time.
#ifdef __GNUC__
#define ISPRINTF(x,y) __attribute__((format (printf, x, y)))
#else
#define ISPRINTF(x,y)
#endif

// Compiler-enforced sentinel safety helper.
// This is appended to function declarations that use sentinel-style semantics,
//  and will make sure your passed the right params to "..." where a NULL
//  is needed at the end of the list.
#ifdef __GNUC__
#define ISSENTINEL __attribute__((sentinel))
#else
#define ISSENTINEL
#endif

// Command line access outside of main().
extern int GArgc;
extern const char **GArgv;

// Build-specific details.
extern const char *GBuildVer;

// Static, non-stack memory for scratch work...not thread safe!
extern uint8 scratchbuf_128k[128 * 1024];


#define UNICODE_BOGUS_CHAR_VALUE 0xFFFFFFFF
#define UNICODE_BOGUS_CHAR_CODEPOINT '?'
// !!! FIXME: document me!
uint32 utf8codepoint(const char **str);

// !!! FIXME: document me!
int utf8len(const char *str);

// !!! FIXME: document me!
char **splitText(const char *text, int scrw, int *_count, int *_w);


// Format a string, sort of (but not exactly!) like sprintf().
//  The only formatters accepted are %0 through %9 (and %%), which do not
//  have to appear in order in the string, but match the varargs passed to the
//  function. Only strings are accepted for varargs. This function allocates
//  memory as necessary, so you need to free() the result, but don't need to
//  preallocate a buffer and be concerned about overflowing it.
// This does not use scratchbuf_128k.
char *format(const char *fmt, ...);

// Convert an int to a string. This uses incremental pieces of
//  scratchbuf_128k for a buffer to store the results, and
//  will overwrite itself after some number of calls when the memory
//  is all used, but note that other things use scratchbuf_128k too,
//  so this is only good for printf() things:
// fmtfunc("mission took %0 seconds, %1 points", numstr(secs), numstr(pts));
const char *numstr(int val);

// Call this for fatal errors that require immediate app termination.
//  Does not clean up, or translate the error string. Try to avoid this.
//  These are for crucial lowlevel issues that preclude any meaningful
//  recovery (GUI didn't initialize, etc)
// Doesn't return, but if it did, you can assume it returns zero, so you can
//  do:  'return panic("out of memory");' or whatnot.
int panic(const char *err);

// Call this for a fatal problem that attempts an orderly shutdown (system
//  is fully working, but config file is hosed, etc).
// This makes an attempt to clean up any files already created by the
//  current installer (successfully run Setup.Install blocks in the config
//  file are not cleaned up).
// If there's no GUI or Lua isn't initialized, this calls panic(). That's bad.
// Doesn't return, but if it did, you can assume it returns zero, so you can
//  do:  'return fatal("missing config file");' or whatnot.
// THIS DOES NOT USE PRINTF-STYLE FORMAT CODES. Please see the comments for
//  format() for details.
int fatal(const char *fmt, ...);

// The platform layer should set up signal/exception handlers before calling
//  MojoSetup_main(), that will call these functions. "crashed" for bug
//  signals (SIGSEGV, GPF, etc), and "terminated" for external forces that
//  destroy the process (SIGKILL, SIGINT, etc). These functions do not return.
void MojoSetup_crashed(void);
void MojoSetup_terminated(void);

// Malloc replacements that blow up on allocation failure.
// Please note that xmalloc() will zero the newly-allocated memory buffer,
//  like calloc() would, but xrealloc() makes no such promise!
void *xmalloc(size_t bytes);
void *xrealloc(void *ptr, size_t bytes);
char *xstrdup(const char *str);

// strncpy() that promises to null-terminate the string, even on overflow.
char *xstrncpy(char *dst, const char *src, size_t len);


#ifdef malloc
#undef malloc
#endif
#define malloc(x) DO_NOT_CALL_MALLOC__USE_XMALLOC_INSTEAD

#ifdef calloc
#undef calloc
#endif
#define calloc(x,y) DO_NOT_CALL_CALLOC__USE_XMALLOC_INSTEAD

#ifdef realloc
#undef realloc
#endif
#define realloc(x,y) DO_NOT_CALL_REALLOC__USE_XREALLOC_INSTEAD

#ifdef strdup
#undef strdup
#endif
#define strdup(x) DO_NOT_CALL_STRDUP__USE_XSTRDUP_INSTEAD

#ifdef strncpy
#undef strncpy
#endif
#define strncpy(x,y,z) DO_NOT_CALL_STRNCPY__USE_XSTRNCPY_INSTEAD

#if 0  // !!! FIXME: write me.
#ifdef strcasecmp
#undef strcasecmp
#endif
#define strcasecmp(x,y) DO_NOT_CALL_STRCASECMP__USE_UTF8STRICMP_INSTEAD
#endif

#if 0  // !!! FIXME: write me.
#ifdef stricmp
#undef stricmp
#endif
#define stricmp(x,y) DO_NOT_CALL_STRICMP__USE_UTF8STRICMP_INSTEAD
#endif

#if 0  // !!! FIXME: write me.
#ifdef strcmpi
#undef strcmpi
#endif
#define strcmpi(x,y) DO_NOT_CALL_STRCMPI__USE_UTF8STRICMP_INSTEAD
#endif

// Localization support.
const char *translate(const char *str);
#ifdef _
#undef _
#endif
#define _(x) translate(x)

// Call this with what you are profiling and the start time to log it:
//   uint32 start = MojoPlatform_ticks();
//     ...do something...
//   profile("Something I did", start);
uint32 profile(const char *what, uint32 start_time);

// This tries to decode a graphic file in memory into an RGBA framebuffer,
//  first with platform-specific facilities, if any, and then any built-in
//  decoders, if that fails.
// (data) points to the compressed data, (size) is the number of bytes
//  of compressed data. (*w) and (*h) will contain the images dimensions on
//  return.
// Returns NULL on failure (unsupported, etc) and a pointer to the
//  uncompressed data on success. Caller must free() the returned pointer!
uint8 *decodeImage(const uint8 *data, uint32 size, uint32 *w, uint32 *h);

// See if a given string is in a valid product key format.
// (fmt) points to a string that would be a Setup.ProductKey.format.
// (key) is the key as it currently stands. It might be partially entered.
// Returns true if the key is valid for the format, false otherwise.
boolean isValidProductKey(const char *fmt, const char *key);

// See if a given flag was on the command line
//
// cmdline("nosound") will return true if "-nosound", "--nosound",
//  etc was on the command line. The option must start with a '-' on the
//  command line to be noticed by this function.
//
//  \param arg the command line flag to find
// \return true if arg was on the command line, false otherwise.
boolean cmdline(const char *arg);


// Get robust command line options, with optional default for missing ones.
//
//  If the command line was ./myapp --a=b -c=d ----e f
//    - cmdlinestr("a") will return "b"
//    - cmdlinestr("c") will return "d"
//    - cmdlinestr("e") will return "f"
//    - cmdlinestr("g") will return the default string.
//
// Like cmdline(), the option must start with a '-'.
//
//  \param arg The command line option to find.
//  \param envr optional environment var to check if command line wasn't found.
//  \param deflt The return value if (arg) isn't on the command line.
// \return The command line option, or (deflt) if the command line wasn't
//         found. This return value is READ ONLY. Do not free it, either.
const char *cmdlinestr(const char *arg, const char *envr, const char *deflt);

// Returns true if (str) matches (pattern), false otherwise.
//  This is NOT a regexp! It only understands '?' and '*', similar to how
//  wildcards worked in MS-DOS command lines. '?' matches one char, and
//  '*' matches until the end of string or the next char in the pattern
//  is seen. Case matters!
boolean wildcardMatch(const char *str, const char *pattern);

// Logging functions.
typedef enum
{
    MOJOSETUP_LOG_NOTHING,
    MOJOSETUP_LOG_ERRORS,
    MOJOSETUP_LOG_WARNINGS,
    MOJOSETUP_LOG_INFO,
    MOJOSETUP_LOG_DEBUG,
    MOJOSETUP_LOG_EVERYTHING
} MojoSetupLogLevel;

extern MojoSetupLogLevel MojoLog_logLevel;
void MojoLog_initLogging(void);
void MojoLog_deinitLogging(void);

// Logging facilities.
// THESE DO NOT USE PRINTF-STYLE FORMAT CODES. Please see the comments for
//  format() for details.
void logWarning(const char *fmt, ...);
void logError(const char *fmt, ...);
void logInfo(const char *fmt, ...);
void logDebug(const char *fmt, ...);


// Checksums.

typedef uint32 MojoCrc32;
void MojoCrc32_init(MojoCrc32 *context);
void MojoCrc32_append(MojoCrc32 *context, const uint8 *buf, uint32 len);
void MojoCrc32_finish(MojoCrc32 *context, uint32 *digest);


typedef struct MojoMd5
{
    uint32 count[2];
    uint32 abcd[4];
    uint8 buf[64];
} MojoMd5;

void MojoMd5_init(MojoMd5 *pms);
void MojoMd5_append(MojoMd5 *pms, const uint8 *data, int nbytes);
void MojoMd5_finish(MojoMd5 *pms, uint8 digest[16]);


typedef struct
{
    uint32 state[5];
    uint32 count[2];
    uint8 buffer[64];
} MojoSha1;

void MojoSha1_init(MojoSha1 *context);
void MojoSha1_append(MojoSha1 *context, const uint8 *data, uint32 len);
void MojoSha1_finish(MojoSha1 *context, uint8 digest[20]);

typedef struct MojoChecksumContext
{
    MojoCrc32 crc32;
    MojoMd5 md5;
    MojoSha1 sha1;
} MojoChecksumContext;


typedef struct MojoChecksums
{
    uint32 crc32;
    uint8 md5[16];
    uint8 sha1[20];
} MojoChecksums;


void MojoChecksum_init(MojoChecksumContext *ctx);
void MojoChecksum_append(MojoChecksumContext *c, const uint8 *data, uint32 ln);
void MojoChecksum_finish(MojoChecksumContext *c, MojoChecksums *sums);


// A pointer to this struct is passed to plugins, so they can access
//  functionality in the base application. (Add to this as you need to.)
typedef struct MojoSetupEntryPoints
{
    void *(*xmalloc)(size_t bytes);
    void *(*xrealloc)(void *ptr, size_t bytes);
    char *(*xstrdup)(const char *str);
    char *(*xstrncpy)(char *dst, const char *src, size_t len);
    const char *(*translate)(const char *str);
    void (*logWarning)(const char *fmt, ...);
    void (*logError)(const char *fmt, ...);
    void (*logInfo)(const char *fmt, ...);
    void (*logDebug)(const char *fmt, ...);
    char *(*format)(const char *fmt, ...);
    const char *(*numstr)(int val);
    uint32 (*ticks)(void);
    uint32 (*utf8codepoint)(const char **_str);
    int (*utf8len)(const char *str);
    char **(*splitText)(const char *text, int scrw, int *_count, int *_w);
    boolean (*isValidProductKey)(const char *f, const char *k);
} MojoSetupEntryPoints;
extern MojoSetupEntryPoints GEntryPoints;


#ifndef DOXYGEN_SHOULD_IGNORE_THIS
#if ((defined _MSC_VER) || (PLATFORM_BEOS))
#define __EXPORT__ __declspec(dllexport)
#elif (defined SUNPRO_C)
#define __EXPORT__ __global
#elif (__GNUC__ >= 3)
#define __EXPORT__ __attribute__((visibility("default")))
#else
#define __EXPORT__
#endif
#endif  // DOXYGEN_SHOULD_IGNORE_THIS

#define DEFINE_TO_STR2(x) #x
#define DEFINE_TO_STR(x) DEFINE_TO_STR2(x)

#ifndef DOXYGEN_SHOULD_IGNORE_THIS
#define STUBBED2(prelog, x) \
do { \
    static boolean seen_this = false; \
    if (!seen_this) \
    { \
        seen_this = true; \
        prelog logDebug("STUBBED: %0 at %1 (%2:%3)\n", x, __FUNCTION__, \
               __FILE__, DEFINE_TO_STR(__LINE__)); \
    } \
} while (false)
#define STUBBED(x) STUBBED2(,x)
#endif

#define STATICARRAYLEN(x) ( (sizeof ((x))) / (sizeof ((x)[0])) )

#ifdef __cplusplus
}
#endif

#endif

// end of universal.h ...

