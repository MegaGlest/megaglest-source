/**
 * MojoSetup; a portable, flexible installation application.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

#include <stdarg.h>

#include "universal.h"
#include "platform.h"
#include "gui.h"
#include "lua_glue.h"
#include "fileio.h"

#define TEST_LAUNCH_BROWSER_CODE 0
int MojoSetup_testLaunchBrowserCode(int argc, char **argv);

#define TEST_ARCHIVE_CODE 0
int MojoSetup_testArchiveCode(int argc, char **argv);

#define TEST_NETWORK_CODE 0
int MojoSetup_testNetworkCode(int argc, char **argv);


uint8 scratchbuf_128k[128 * 1024];
MojoSetupEntryPoints GEntryPoints =
{
    xmalloc,
    xrealloc,
    xstrdup,
    xstrncpy,
    translate,
    logWarning,
    logError,
    logInfo,
    logDebug,
    format,
    numstr,
    MojoPlatform_ticks,
    utf8codepoint,
    utf8len,
    splitText,
    isValidProductKey,
};

int GArgc = 0;
const char **GArgv = NULL;

static char *crashedmsg = NULL;
static char *termedmsg = NULL;

void MojoSetup_crashed(void)
{
    if (crashedmsg == NULL)
        panic("BUG: crash at startup");
    else
        fatal(crashedmsg);
} // MojoSetup_crash


void MojoSetup_terminated(void)
{
    if (termedmsg == NULL)  // no translation yet.
        panic("The installer has been stopped by the system.");
    else
        fatal(termedmsg);
} // MojoSetup_crash


#if !SUPPORT_MULTIARCH
#define trySwitchBinaries()
#else
static void trySwitchBinary(MojoArchive *ar)
{
    MojoInput *io = ar->openCurrentEntry(ar);
    if (io != NULL)
    {
        const uint32 imglen = (uint32) io->length(io);
        uint8 *img = (uint8 *) xmalloc(imglen);
        const uint32 br = io->read(io, img, imglen);
        io->close(io);
        if (br == imglen)
        {
            logInfo("Switching binary with '%0'...", ar->prevEnum.filename);
            MojoPlatform_switchBin(img, imglen);  // no return on success.
            logError("...Switch binary failed.");
        } // if
        free(img);
    } // if
} // trySwitchBinary


static void trySwitchBinaries(void)
{
    if (cmdlinestr("nobinswitch", "MOJOSETUP_NOBINSWITCH", NULL) != NULL)
        return;  // we are already switched or the user is preventing it.

    setenv("MOJOSETUP_NOBINSWITCH", "1", 1);
    setenv("MOJOSETUP_BASE", GBaseArchivePath, 1);

    if (GBaseArchive->enumerate(GBaseArchive))
    {
        const MojoArchiveEntry *entinfo;
        while ((entinfo = GBaseArchive->enumNext(GBaseArchive)) != NULL)
        {
            if (entinfo->type != MOJOARCHIVE_ENTRY_FILE)
                continue;

            if (strncmp(entinfo->filename, "arch/", 5) != 0)
                continue;

            trySwitchBinary(GBaseArchive);
        } // while
    } // if
    
} // trySwitchBinaries
#endif


static boolean trySpawnTerminalGui(void)
{
    if (cmdlinestr("notermspawn", "MOJOSETUP_NOTERMSPAWN", NULL) != NULL)
        return false;  // we already spawned or the user is preventing it.

    if (MojoPlatform_istty())  // maybe we can spawn a terminal for stdio?
        return false;  // We're a terminal already, no need to spawn one.

    logInfo("No usable GUI found. Trying to spawn a terminal...");
    if (!MojoPlatform_spawnTerminal())
    {
        logError("...Terminal spawning failed.");
        return false;
    } // if

    assert(MojoPlatform_istty());
    return (MojoGui_initGuiPlugin() != NULL);
} // trySpawnTerminalGui


static boolean initEverything(void)
{
    MojoLog_initLogging();

    logInfo("MojoSetup starting up...");

    // We have to panic on errors until the GUI is ready. Try to make things
    //  "succeed" unless they are catastrophic, and report problems later.

    // Start with the base archive work, since it might have GUI plugins.
    //  None of these panic() calls are localized, since localization isn't
    //  functional until MojoLua_initLua() succeeds.
    if (!MojoArchive_initBaseArchive())
        panic("Initial setup failed. Cannot continue.");

    trySwitchBinaries();  // may not return.

    if (!MojoGui_initGuiPlugin())
    {
        // This could terminate the process (and relaunch).
        if (!trySpawnTerminalGui())
            panic("Failed to start GUI. Is your download incomplete or corrupt?");
    } // if

    else if (!MojoLua_initLua())
    {
        // (...but if you're the developer: are your files in the wrong place?)
        panic("Failed to start. Is your download incomplete or corrupt?");
    } // else if

    crashedmsg = xstrdup(_("The installer has crashed due to a bug."));
    termedmsg = xstrdup(_("The installer has been stopped by the system."));

    return true;
} // initEverything


static void deinitEverything(void)
{
    char *tmp = NULL;

    logInfo("MojoSetup shutting down...");
    MojoLua_deinitLua();
    MojoGui_deinitGuiPlugin();
    MojoArchive_deinitBaseArchive();
    MojoLog_deinitLogging();

    tmp = crashedmsg;
    crashedmsg = NULL;
    free(tmp);
    tmp = termedmsg;
    termedmsg = NULL;
    free(tmp);
} // deinitEverything


void MojoChecksum_init(MojoChecksumContext *ctx)
{
    memset(ctx, '\0', sizeof (MojoChecksumContext));
    #if SUPPORT_CRC32
    MojoCrc32_init(&ctx->crc32);
    #endif
    #if SUPPORT_MD5
    MojoMd5_init(&ctx->md5);
    #endif
    #if SUPPORT_SHA1
    MojoSha1_init(&ctx->sha1);
    #endif
} // MojoChecksum_init


void MojoChecksum_append(MojoChecksumContext *ctx, const uint8 *d, uint32 len)
{
    #if SUPPORT_CRC32
    MojoCrc32_append(&ctx->crc32, d, len);
    #endif
    #if SUPPORT_MD5
    MojoMd5_append(&ctx->md5, d, len);
    #endif
    #if SUPPORT_SHA1
    MojoSha1_append(&ctx->sha1, d, len);
    #endif
} // MojoChecksum_append


void MojoChecksum_finish(MojoChecksumContext *ctx, MojoChecksums *sums)
{
    memset(sums, '\0', sizeof (MojoChecksums));
    #if SUPPORT_CRC32
    MojoCrc32_finish(&ctx->crc32, &sums->crc32);
    #endif
    #if SUPPORT_MD5
    MojoMd5_finish(&ctx->md5, sums->md5);
    #endif
    #if SUPPORT_SHA1
    MojoSha1_finish(&ctx->sha1, sums->sha1);
    #endif
} // MojoChecksum_finish


boolean cmdline(const char *arg)
{
    int argc = GArgc;
    const char **argv = GArgv;
    int i;

    if ((arg == NULL) || (argv == NULL))
        return false;

    while (*arg == '-')  // Skip all '-' chars, so "--nosound" == "-nosound"
        arg++;

    for (i = 1; i < argc; i++)
    {
        const char *thisarg = argv[i];
        if (*thisarg != '-')
            continue;  // no dash in the string, skip it.
        while (*(++thisarg) == '-') { /* keep looping. */ }
        if (strcmp(arg, thisarg) == 0)
            return true;
    } // for

    return false;
} // cmdline


const char *cmdlinestr(const char *arg, const char *envr, const char *deflt)
{
    uint32 len = 0;
    int argc = GArgc;
    const char **argv = GArgv;
    int i;

    if (envr != NULL)
    {
        const char *val = getenv(envr);
        if (val != NULL)
            return val;
    } // if

    if (arg == NULL)
        return deflt;

    while (*arg == '-')  // Skip all '-' chars, so "--nosound" == "-nosound"
        arg++;

    len = strlen(arg);

    for (i = 1; i < argc; i++)
    {
        const char *thisarg = argv[i];
        if (*thisarg != '-')
            continue;  // no dash in the string, skip it.

        while (*(++thisarg) == '-') { /* keep looping. */ }
        if (strncmp(arg, thisarg, len) != 0)
            continue;   // not us.

        thisarg += len;  // skip ahead in string to end of match.

        if (*thisarg == '=')  // --a=b format.
            return (thisarg + 1);
        else if (*thisarg == '\0')  // --a b format.
            return ((argv[i+1] == NULL) ? deflt : argv[i+1]);
    } // for

    return deflt;
} // cmdlinestr


boolean wildcardMatch(const char *str, const char *pattern)
{
    char sch = *(str++);
    while (true)
    {
        const char pch = *(pattern++);
        if (pch == '?')
        {
            if ((sch == '\0') || (sch == '/'))
                return false;
            sch = *(str++);
        } // else if

        else if (pch == '*')
        {
            char nextpch = *pattern;
            if ((nextpch != '?') && (nextpch != '*'))
            {
                while ((sch != '\0') && (sch != nextpch))
                    sch = *(str++);
            } // if
        } // else if

        else
        {
            if (pch != sch)
                return false;
            else if (pch == '\0')
                break;
            sch = *(str++);
        } // else
    } // while

    return true;
} // wildcardMatch


const char *numstr(int val)
{
    static int pos = 0;
    char *ptr = ((char *) scratchbuf_128k) + (pos * 128);
    snprintf(ptr, 128, "%d", val);
    pos = (pos + 1) % 1000;
    return ptr;
} // numstr


static char *format_internal(const char *fmt, va_list ap)
{
    // This is kinda nasty. String manipulation in C always is.
    char *retval = NULL;
    const char *strs[10];  // 0 through 9.
    const char *ptr = NULL;
    char *wptr = NULL;
    size_t len = 0;
    int maxfmtid = -2;
    int i;

    // figure out what this format string contains...
    for (ptr = fmt; *ptr; ptr++)
    {
        if (*ptr == '%')
        {
            const char ch = *(++ptr);
            if (ch == '%')  // a literal '%'
                maxfmtid = (maxfmtid == -2) ? -1 : maxfmtid;
            else if ((ch >= '0') && (ch <= '9'))
                maxfmtid = ((maxfmtid > (ch - '0')) ? maxfmtid : (ch - '0'));
            else
                fatal(_("BUG: Invalid format() string"));
        } // if
    } // while

    if (maxfmtid == -2)  // no formatters present at all.
        return xstrdup(fmt);  // just copy it, we're done.

    for (i = 0; i <= maxfmtid; i++)  // referenced varargs --> linear array.
    {
        strs[i] = va_arg(ap, const char *);
        if (strs[i] == NULL)
            strs[i] = "(null)";  // just to match sprintf() behaviour...
    } // for

    // allocate the string we'll need in one shot, so we don't have to resize.
    for (ptr = fmt; *ptr; ptr++)
    {
        if (*ptr != '%')
            len++;
        else
        {
            const char ch = *(++ptr);
            if (ch == '%')  // a literal '%'
                len++;  // just want '%' char.
            else //if ((ch >= '0') && (ch <= '9'))
                len += strlen(strs[ch - '0']);
        } // else
    } // while

    // Now write the formatted string...
    wptr = retval = (char *) xmalloc(len+1);
    for (ptr = fmt; *ptr; ptr++)
    {
        const char strch = *ptr;
        if (strch != '%')
            *(wptr++) = strch;
        else
        {
            const char ch = *(++ptr);
            if (ch == '%')  // a literal '%'
                *(wptr++) = '%';
            else //if ((ch >= '0') && (ch <= '9'))
            {
                const char *str = strs[ch - '0'];
                strcpy(wptr, str);
                wptr += strlen(str);
            } // else
        } // else
    } // while
    *wptr = '\0';

    return retval;
} // format_internal


char *format(const char *fmt, ...)
{
    char *retval = NULL;
    va_list ap;
    va_start(ap, fmt);
    retval = format_internal(fmt, ap);
    va_end(ap);
    return retval;
} // format


#if ((defined _NDEBUG) || (defined NDEBUG))
#define DEFLOGLEV "info"
#else
#define DEFLOGLEV "everything"
#endif
MojoSetupLogLevel MojoLog_logLevel = MOJOSETUP_LOG_EVERYTHING;
static void *logFile = NULL;

void MojoLog_initLogging(void)
{
    const char *level = cmdlinestr("loglevel","MOJOSETUP_LOGLEVEL", DEFLOGLEV);
    const char *fname = cmdlinestr("log", "MOJOSETUP_LOG", NULL);

    if (strcmp(level, "nothing") == 0)
        MojoLog_logLevel = MOJOSETUP_LOG_NOTHING;
    else if (strcmp(level, "errors") == 0)
        MojoLog_logLevel = MOJOSETUP_LOG_ERRORS;
    else if (strcmp(level, "warnings") == 0)
        MojoLog_logLevel = MOJOSETUP_LOG_WARNINGS;
    else if (strcmp(level, "info") == 0)
        MojoLog_logLevel = MOJOSETUP_LOG_INFO;
    else if (strcmp(level, "debug") == 0)
        MojoLog_logLevel = MOJOSETUP_LOG_DEBUG;
    else  // Unknown string gets everything...that'll teach you.
        MojoLog_logLevel = MOJOSETUP_LOG_EVERYTHING;

    if ((fname != NULL) && (strcmp(fname, "-") == 0))
        logFile = MojoPlatform_stdout();
    else if (fname != NULL)
    {
        const uint32 flags = MOJOFILE_WRITE|MOJOFILE_CREATE|MOJOFILE_TRUNCATE;
        const uint16 mode = MojoPlatform_defaultFilePerms();
        logFile = MojoPlatform_open(fname, flags, mode);
    } // if
} // MojoLog_initLogging


void MojoLog_deinitLogging(void)
{
    if (logFile != NULL)
    {
        MojoPlatform_close(logFile);
        logFile = NULL;
    } // if
} // MojoLog_deinitLogging


static inline void addLog(MojoSetupLogLevel level, char levelchar,
                          const char *fmt, va_list ap)
{
    if (level <= MojoLog_logLevel)
    {
        char *str = format_internal(fmt, ap);
        //int len = vsnprintf(buf + 2, sizeof (buf) - 2, fmt, ap) + 2;
        //buf[0] = levelchar;
        //buf[1] = ' ';
        int len = strlen(str);
        while ( (--len >= 0) && ((str[len] == '\n') || (str[len] == '\r')) ) {}
        str[len+1] = '\0';  // delete trailing newline crap.
        MojoPlatform_log(str);
        if (logFile != NULL)
        {
            const char *endl = MOJOPLATFORM_ENDLINE;
            MojoPlatform_write(logFile, str, strlen(str));
            MojoPlatform_write(logFile, endl, strlen(endl));
            MojoPlatform_flush(logFile);
        } // if
        free(str);
    } // if
} // addLog


void logWarning(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    addLog(MOJOSETUP_LOG_WARNINGS, '-', fmt, ap);
    va_end(ap);
} // logWarning


void logError(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    addLog(MOJOSETUP_LOG_ERRORS, '!', fmt, ap);
    va_end(ap);
} // logError


void logInfo(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    addLog(MOJOSETUP_LOG_INFO, '*', fmt, ap);
    va_end(ap);
} // logInfo


void logDebug(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    addLog(MOJOSETUP_LOG_DEBUG, '#', fmt, ap);
    va_end(ap);
} // logDebug


uint32 profile(const char *what, uint32 start_time)
{
    uint32 retval = MojoPlatform_ticks() - start_time;
    if (what != NULL)
        logDebug("%0 took %1 ms.", what, numstr((int) retval));
    return retval;
} // profile_start


int fatal(const char *fmt, ...)
{
    static boolean in_fatal = false;

    if (in_fatal)
        return panic("BUG: fatal() called more than once!");

    in_fatal = true;

    // may not want to show a message, since we displayed one elsewhere, etc.
    if (fmt != NULL)
    {
        char *buf = NULL;
        va_list ap;
        va_start(ap, fmt);
        buf = format_internal(fmt, ap);
        va_end(ap);

        logError("FATAL: %0", buf);

        if (GGui != NULL)
            GGui->msgbox(_("Fatal error"), buf);

        free(buf);
    } // if

    // Shouldn't call fatal() before app is initialized!
    if ( (GGui == NULL) || (!MojoLua_initialized()) )
        panic("fatal() called before app is initialized! Panicking...");

    MojoLua_callProcedure("revertinstall");

    deinitEverything();
    exit(23);
    return 0;
} // fatal


int panic(const char *err)
{
    static int panic_runs = 0;

    panic_runs++;
    if (panic_runs == 1)
    {
        logError("PANIC: %0", err);
        panic(err);
    } // if

    else if (panic_runs == 2)
    {
        boolean domsgbox = ((GGui != NULL) && (GGui->msgbox != NULL));
        if (domsgbox)
            GGui->msgbox(_("PANIC"), err);

        if ((GGui != NULL) && (GGui->deinit != NULL))
            GGui->deinit();

        if (!domsgbox)
            panic(err);  /* no GUI plugin...double-panic. */
    } // if

    else if (panic_runs == 3)  // no GUI or panic panicked...write to stderr...
        fprintf(stderr, "\n\n\n%s\n  %s\n\n\n", _("PANIC"), err);

    else  // panic is panicking in a loop, terminate without any cleanup...
        MojoPlatform_die();

    exit(22);
    return 0;  // shouldn't hit this.
} // panic


char *xstrncpy(char *dst, const char *src, size_t len)
{
    snprintf(dst, len, "%s", src);
    return dst;
} // xstrncpy


uint32 utf8codepoint(const char **_str)
{
    const char *str = *_str;
    uint32 retval = 0;
    uint32 octet = (uint32) ((uint8) *str);
    uint32 octet2, octet3, octet4;

    if (octet == 0)  // null terminator, end of string.
        return 0;

    else if (octet < 128)  // one octet char: 0 to 127
    {
        (*_str)++;  // skip to next possible start of codepoint.
        return octet;
    } // else if

    else if ((octet > 127) && (octet < 192))  // bad (starts with 10xxxxxx).
    {
        // Apparently each of these is supposed to be flagged as a bogus
        //  char, instead of just resyncing to the next valid codepoint.
        (*_str)++;  // skip to next possible start of codepoint.
        return UNICODE_BOGUS_CHAR_VALUE;
    } // else if

    else if (octet < 224)  // two octets
    {
        octet -= (128+64);
        octet2 = (uint32) ((uint8) *(++str));
        if ((octet2 & (128+64)) != 128)  // Format isn't 10xxxxxx?
            return UNICODE_BOGUS_CHAR_VALUE;

        *_str += 2;  // skip to next possible start of codepoint.
        retval = ((octet << 6) | (octet2 - 128));
        if ((retval >= 0x80) && (retval <= 0x7FF))
            return retval;
    } // else if

    else if (octet < 240)  // three octets
    {
        octet -= (128+64+32);
        octet2 = (uint32) ((uint8) *(++str));
        if ((octet2 & (128+64)) != 128)  // Format isn't 10xxxxxx?
            return UNICODE_BOGUS_CHAR_VALUE;

        octet3 = (uint32) ((uint8) *(++str));
        if ((octet3 & (128+64)) != 128)  // Format isn't 10xxxxxx?
            return UNICODE_BOGUS_CHAR_VALUE;

        *_str += 3;  // skip to next possible start of codepoint.
        retval = ( ((octet << 12)) | ((octet2-128) << 6) | ((octet3-128)) );

        // There are seven "UTF-16 surrogates" that are illegal in UTF-8.
        switch (retval)
        {
            case 0xD800:
            case 0xDB7F:
            case 0xDB80:
            case 0xDBFF:
            case 0xDC00:
            case 0xDF80:
            case 0xDFFF:
                return UNICODE_BOGUS_CHAR_VALUE;
        } // switch

        // 0xFFFE and 0xFFFF are illegal, too, so we check them at the edge.
        if ((retval >= 0x800) && (retval <= 0xFFFD))
            return retval;
    } // else if

    else if (octet < 248)  // four octets
    {
        octet -= (128+64+32+16);
        octet2 = (uint32) ((uint8) *(++str));
        if ((octet2 & (128+64)) != 128)  // Format isn't 10xxxxxx?
            return UNICODE_BOGUS_CHAR_VALUE;

        octet3 = (uint32) ((uint8) *(++str));
        if ((octet3 & (128+64)) != 128)  // Format isn't 10xxxxxx?
            return UNICODE_BOGUS_CHAR_VALUE;

        octet4 = (uint32) ((uint8) *(++str));
        if ((octet4 & (128+64)) != 128)  // Format isn't 10xxxxxx?
            return UNICODE_BOGUS_CHAR_VALUE;

        *_str += 4;  // skip to next possible start of codepoint.
        retval = ( ((octet << 18)) | ((octet2 - 128) << 12) |
                   ((octet3 - 128) << 6) | ((octet4 - 128)) );
        if ((retval >= 0x10000) && (retval <= 0x10FFFF))
            return retval;
    } // else if

    // Five and six octet sequences became illegal in rfc3629.
    //  We throw the codepoint away, but parse them to make sure we move
    //  ahead the right number of bytes and don't overflow the buffer.

    else if (octet < 252)  // five octets
    {
        octet = (uint32) ((uint8) *(++str));
        if ((octet & (128+64)) != 128)  // Format isn't 10xxxxxx?
            return UNICODE_BOGUS_CHAR_VALUE;

        octet = (uint32) ((uint8) *(++str));
        if ((octet & (128+64)) != 128)  // Format isn't 10xxxxxx?
            return UNICODE_BOGUS_CHAR_VALUE;

        octet = (uint32) ((uint8) *(++str));
        if ((octet & (128+64)) != 128)  // Format isn't 10xxxxxx?
            return UNICODE_BOGUS_CHAR_VALUE;

        octet = (uint32) ((uint8) *(++str));
        if ((octet & (128+64)) != 128)  // Format isn't 10xxxxxx?
            return UNICODE_BOGUS_CHAR_VALUE;

        *_str += 5;  // skip to next possible start of codepoint.
        return UNICODE_BOGUS_CHAR_VALUE;
    } // else if

    else  // six octets
    {
        octet = (uint32) ((uint8) *(++str));
        if ((octet & (128+64)) != 128)  // Format isn't 10xxxxxx?
            return UNICODE_BOGUS_CHAR_VALUE;

        octet = (uint32) ((uint8) *(++str));
        if ((octet & (128+64)) != 128)  // Format isn't 10xxxxxx?
            return UNICODE_BOGUS_CHAR_VALUE;

        octet = (uint32) ((uint8) *(++str));
        if ((octet & (128+64)) != 128)  // Format isn't 10xxxxxx?
            return UNICODE_BOGUS_CHAR_VALUE;

        octet = (uint32) ((uint8) *(++str));
        if ((octet & (128+64)) != 128)  // Format isn't 10xxxxxx?
            return UNICODE_BOGUS_CHAR_VALUE;

        octet = (uint32) ((uint8) *(++str));
        if ((octet & (128+64)) != 128)  // Format isn't 10xxxxxx?
            return UNICODE_BOGUS_CHAR_VALUE;

        *_str += 6;  // skip to next possible start of codepoint.
        return UNICODE_BOGUS_CHAR_VALUE;
    } // else if

    return UNICODE_BOGUS_CHAR_VALUE;
} // utf8codepoint


int utf8len(const char *str)
{
    int retval = 0;
    while (utf8codepoint(&str))
        retval++;
    return retval;
} // utf8len


static char *strfrombuf(const char *text, int len)
{
    char *retval = xmalloc(len + 1);
    memcpy(retval, text, len);
    retval[len] = '\0';
    return retval;
} // strfrombuf


char **splitText(const char *text, int scrw, int *_count, int *_w)
{
    int i = 0;
    int j = 0;
    char **retval = NULL;
    int count = 0;
    int w = 0;

    *_count = *_w = 0;
    while (*text)
    {
        const char *utf8text = text;
        uint32 ch = 0;
        int pos = 0;
        int furthest = 0;

        for (i = 0; ((ch = utf8codepoint(&utf8text))) && (i < scrw); i++)
        {
            if ((ch == '\r') || (ch == '\n'))
            {
                const char nextbyte = *utf8text;
                count++;
                retval = (char **) xrealloc(retval, count * sizeof (char *));
                retval[count-1] = strfrombuf(text, utf8text - text);
                if ((ch == '\r') && (nextbyte == '\n'))  // DOS endlines!
                    utf8text++; // skip it.
                text = (char *) utf8text;  // update to start of new line.

                if (i > w)
                    w = i;
                i = -1;  // will be zero on next iteration...
            } // if
            else if ((ch == ' ') || (ch == '\t'))
            {
                if (i != 0)  // trim spaces from start of line...
                    furthest = i;
                else
                {
                    text++;
                    i = -1;  // it'll be zero on next iteration.
                } // else
            } // else if
        } // for

        // line overflow or end of stream...
        pos = (ch) ? furthest : i;
        if ((ch) && (furthest == 0))  // uhoh, no split at all...hack it.
        {
            pos = utf8len(text);
            if (pos > scrw)  // too big, have to chop a string in the middle.
                pos = scrw;
        } // if

        if (pos > 0)
        {
            utf8text = text;  // adjust pointer by redecoding from start...
            for (j = 0; j < pos; j++)
                utf8codepoint(&utf8text);

            count++;
            retval = (char **) xrealloc(retval, count * sizeof (char*));
            retval[count-1] = strfrombuf(text, utf8text - text);
            text = (char *) utf8text;
            if (pos > w)
                w = pos;
        } // if
    } // while

    *_count = count;
    *_w = w;
    return retval;
} // splitText


static void outOfMemory(void)
{
    // Try to translate "out of memory", but not if it causes recursion.
    static boolean already_panicked = false;
    const char *errstr = "out of memory";
    if (!already_panicked)
    {
        already_panicked = true;
        errstr = translate(errstr);
    } // if
    panic(errstr);
} // outOfMemory


#undef calloc
void *xmalloc(size_t bytes)
{
    void *retval = calloc(1, bytes);
    if (retval == NULL)
        outOfMemory();
    return retval;
} // xmalloc
#define calloc(x,y) DO_NOT_CALL_CALLOC__USE_XMALLOC_INSTEAD

#undef realloc
void *xrealloc(void *ptr, size_t bytes)
{
    void *retval = realloc(ptr, bytes);
    if (retval == NULL)
        outOfMemory();
    return retval;
} // xrealloc
#define realloc(x,y) DO_NOT_CALL_REALLOC__USE_XREALLOC_INSTEAD

char *xstrdup(const char *str)
{
    char *retval = (char *) xmalloc(strlen(str) + 1);
    strcpy(retval, str);
    return retval;
} // xstrdup


// We have to supply this function under certain build types.
#if MOJOSETUP_INTERNAL_BZLIB && BZ_NO_STDIO
void bz_internal_error(int errcode)
{
    fatal(_("bzlib triggered an internal error: %0"), numstr(errcode));
} // bz_internal_error
#endif


#if SUPPORT_STBIMAGE
unsigned char *stbi_load_from_memory(unsigned char *buffer, int len, int *x,
                                     int *y, int *comp, int req_comp);
#endif

uint8 *decodeImage(const uint8 *data, uint32 size, uint32 *w, uint32 *h)
{
    uint8 *retval = MojoPlatform_decodeImage(data, size, w, h);

    #if SUPPORT_STBIMAGE
    if (retval == NULL)  // try our built-in routines.
    {
        const int siz = (int) size;
        unsigned char *buf = (unsigned char *) data;
        int x = 0, y = 0, comp = 0;
        retval = (uint8 *) stbi_load_from_memory(buf, siz, &x, &y, &comp, 4);
        *w = (uint32) x;
        *h = (uint32) y;
    } // if
    #endif

    if (retval == NULL)
        *w = *h = 0;

    return retval;
} // decodeImage


boolean isValidProductKey(const char *fmt, const char *key)
{
    if (fmt == NULL)
        return true;
    else if (key == NULL)
        return false;

    while (*fmt)
    {
        const char fmtch = *(fmt++);
        const char keych = *(key++);
        switch (fmtch)
        {
            case '-':
            case ' ':
                if ((keych == ' ') || (keych == '-'))
                    break;
                key--;  // user didn't type this, roll back.
                break;

            case '#':
                if ((keych >= '0') && (keych <= '9'))
                    break;
                return false;

            case 'X':
                if ( ((keych >= 'A') && (keych <= 'Z')) ||
                     ((keych >= 'a') && (keych <= 'z')) )
                    break;
                return false;

            case '?':
                if ( ((keych >= 'A') && (keych <= 'Z')) ||
                     ((keych >= 'a') && (keych <= 'z')) ||
                     ((keych >= '0') && (keych <= '9')) )
                    break;
                return false;

            case '*':
                break;

            default:
                // this should have been caught by schema sanitize.
                assert(false && "Invalid product key format.");
                return false;
        } // switch
    } // while

    return (*key == '\0');
} // isValidProductKey


// This is called from main()/WinMain()/whatever.
int MojoSetup_main(int argc, char **argv)
{
    GArgc = argc;
    GArgv = (const char **) argv;

    if (cmdline("buildver"))
    {
        printf("%s\n", GBuildVer);
        return 0;
    } // if

    #if TEST_LAUNCH_BROWSER_CODE
    return MojoSetup_testLaunchBrowserCode(argc, argv);
    #endif

    #if TEST_ARCHIVE_CODE
    return MojoSetup_testArchiveCode(argc, argv);
    #endif

    #if TEST_NETWORK_CODE
    return MojoSetup_testNetworkCode(argc, argv);
    #endif

    if (!initEverything())
        return 1;

    // Jump into Lua for the heavy lifting.
    MojoLua_runFile("mojosetup_mainline");
    deinitEverything();

    return 0;
} // MojoSetup_main



#if TEST_LAUNCH_BROWSER_CODE
int MojoSetup_testLaunchBrowserCode(int argc, char **argv)
{
    int i;

    if (!MojoArchive_initBaseArchive())  // Maybe need for xdg-open script.
        panic("Initial setup failed. Cannot continue.");

    printf("Testing browser launching code...\n\n");
    for (i = 1; i < argc; i++)
    {
        const boolean rc = MojoPlatform_launchBrowser(argv[i]);
        printf("Launch '%s': %s\n", argv[i], rc ? "success" : "failure");
    } // for

    MojoArchive_deinitBaseArchive();
    return 0;
} // MojoSetup_testLaunchBrowserCode
#endif


#if TEST_ARCHIVE_CODE
int MojoSetup_testArchiveCode(int argc, char **argv)
{
    int i;
    printf("Testing archiver code...\n\n");
    for (i = 1; i < argc; i++)
    {
        MojoArchive *archive = MojoArchive_newFromDirectory(argv[i]);
        if (archive != NULL)
            printf("directory '%s'...\n", argv[i]);
        else
        {
            MojoInput *io = MojoInput_newFromFile(argv[i]);
            if (io != NULL)
            {
                archive = MojoArchive_newFromInput(io, argv[i]);
                if (archive != NULL)
                    printf("archive '%s'...\n", argv[i]);
            } // if
        } // else

        if (archive == NULL)
            fprintf(stderr, "Couldn't handle '%s'\n", argv[i]);
        else
        {
            if (!archive->enumerate(archive))
                fprintf(stderr, "enumerate() failed.\n");
            else
            {
                const MojoArchiveEntry *ent;
                while ((ent = archive->enumNext(archive)) != NULL)
                {
                    printf("%s ", ent->filename);
                    if (ent->type == MOJOARCHIVE_ENTRY_FILE)
                    {
                        printf("(file, %d bytes, %o)\n",
                                (int) ent->filesize, ent->perms);

                        MojoInput *input = archive->openCurrentEntry(archive);

                        if(&archive->prevEnum != ent)
                        {
                            fprintf(stderr, "Address of MojoArchiveEntry pointer differs\n");
                            exit(EXIT_FAILURE);
                        } // if

                        if(!input)
                        {
                            fprintf(stderr, "Could not open current entry\n");
                            exit(EXIT_FAILURE);
                        } // if

                        if(!input->ready(input))
                        {
                            input->close(input);
                            continue;
                        } // if

                        uint64 pos = input->tell(input);
                        if(0 != pos)
                        {
                            fprintf(stderr, "position has to be 0 on start, is: %d\n", (int) pos);
                            exit(EXIT_FAILURE);
                        } // if

                        int64 filesize = input->length(input);
                        if(filesize != ent->filesize)
                        {
                            fprintf(stderr, "file size mismatch %d != %d\n",
                            		(int) filesize, (int) ent->filesize);
                            exit(EXIT_FAILURE);
                        } // if

                        boolean ret = input->seek(input, filesize - 1);
                        if(!ret)
                        {
                            fprintf(stderr, "seek() has to return 'true'.\n");
                            exit(EXIT_FAILURE);
                        } // if

                        ret = input->seek(input, filesize);
                        if(ret)
                        {
                            fprintf(stderr, "seek() has to return 'false'.\n");
                            exit(EXIT_FAILURE);
                        } // if

                        pos = input->tell(input);
                        if(filesize -1  != pos)
                        {
                            fprintf(stderr, "position should be %d after seek(), is: %d\n",
                            		(int) filesize - 1, (int) pos);
                            exit(EXIT_FAILURE);
                        } // if

                        input->close(input);
                    } // if
                    else if (ent->type == MOJOARCHIVE_ENTRY_DIR)
                        printf("(dir, %o)\n", ent->perms);
                    else if (ent->type == MOJOARCHIVE_ENTRY_SYMLINK)
                        printf("(symlink -> '%s')\n", ent->linkdest);
                    else
                    {
                        printf("(UNKNOWN?!, %d bytes, -> '%s', %o)\n",
                                (int) ent->filesize, ent->linkdest,
                                ent->perms);
                    } // else
                } // while
            } // else
            archive->close(archive);
            printf("\n\n");
        } // else
    } // for

    return 0;
} // MojoSetup_testArchiveCode
#endif


#if TEST_NETWORK_CODE
int MojoSetup_testNetworkCode(int argc, char **argv)
{
    int i;
    fprintf(stderr, "Testing networking code...\n\n");
    for (i = 1; i < argc; i++)
    {
        static char buf[64 * 1024];
        uint32 start = 0;
        const char *url = argv[i];
        int64 length = -1;
        int64 total_br = 0;
        int64 br = 0;
        printf("\n\nFetching '%s' ...\n", url);
        MojoInput *io = MojoInput_newFromURL(url);
        if (io == NULL)
        {
            fprintf(stderr, "failed!\n");
            continue;
        } // if

        start = MojoPlatform_ticks();
        while (!io->ready(io))
            MojoPlatform_sleep(10);
        fprintf(stderr, "took about %d ticks to get started\n",
                (int) (MojoPlatform_ticks() - start));

        length = io->length(io);
        fprintf(stderr, "Ready to read (%lld) bytes.\n",
                (long long) length);

        do
        {
            start = MojoPlatform_ticks();
            if (!io->ready(io))
            {
                fprintf(stderr, "Not ready!\n");
                while (!io->ready(io))
                    MojoPlatform_sleep(10);
                fprintf(stderr, "took about %d ticks to get ready\n",
                        (int) (MojoPlatform_ticks() - start));
            } // if

            start = MojoPlatform_ticks();
            br = io->read(io, buf, sizeof (buf));
            fprintf(stderr, "read blocked for about %d ticks\n",
                    (int) (MojoPlatform_ticks() - start));
            if (br > 0)
            {
                total_br += br;
                fprintf(stderr, "read %lld bytes\n", (long long) br);
                fwrite(buf, br, 1, stdout);
            } // if
        } while (br > 0);

        if (br < 0)
            fprintf(stderr, "ERROR IN TRANSMISSION.\n\n");
        else
        {
            fprintf(stderr, "TRANSMISSION COMPLETE!\n\n");
            fprintf(stderr, "(Read %lld bytes, expected %lld.)\n",
                    (long long) total_br, length);
        } // else
        io->close(io);
    } // for

    return 0;
} // MojoSetup_testNetworkCode
#endif

// end of mojosetup.c ...

