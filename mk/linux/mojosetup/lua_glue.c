/**
 * MojoSetup; a portable, flexible installation application.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

#include "universal.h"
#include "lua_glue.h"
#include "platform.h"
#include "fileio.h"
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "gui.h"

#define MOJOSETUP_NAMESPACE "MojoSetup"

static lua_State *luaState = NULL;

// Allocator interface for internal Lua use.
static void *MojoLua_alloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
    if (nsize == 0)
    {
        free(ptr);
        return NULL;
    } // if
    return xrealloc(ptr, nsize);
} // MojoLua_alloc


// Read data from a MojoInput when loading Lua code.
static const char *MojoLua_reader(lua_State *L, void *data, size_t *size)
{
    MojoInput *in = (MojoInput *) data;
    char *retval = (char *) scratchbuf_128k;
    int64 br = in->read(in, scratchbuf_128k, sizeof (scratchbuf_128k));
    if (br <= 0)  // eof or error? (lua doesn't care which?!)
    {
        br = 0;
        retval = NULL;
    } // if

    *size = (size_t) br;
    return retval;
} // MojoLua_reader


// Sets t[sym]=f, where t is on the top of the Lua stack.
// !!! FIXME: why is this a different naming convention?
static inline void set_cfunc(lua_State *L, lua_CFunction f, const char *sym)
{
    lua_pushcfunction(L, f);
    lua_setfield(L, -2, sym);
} // set_cfunc


// Sets t[sym]=f, where t is on the top of the Lua stack.
// !!! FIXME: why is this a different naming convention?
static inline void set_cptr(lua_State *L, void *ptr, const char *sym)
{
    lua_pushlightuserdata(L, ptr);
    lua_setfield(L, -2, sym);
} // set_cptr


// Sets t[sym]=f, where t is on the top of the Lua stack.
// !!! FIXME: why is this a different naming convention?
static inline void set_string(lua_State *L, const char *str, const char *sym)
{
    if (str == NULL)
        lua_pushnil(L);
    else
        lua_pushstring(L, str);
    lua_setfield(L, -2, sym);
} // set_string


// Sets t[sym]=f, where t is on the top of the Lua stack.
// !!! FIXME: why is this a different naming convention?
static inline void set_number(lua_State *L, lua_Number x, const char *sym)
{
    lua_pushnumber(L, x);
    lua_setfield(L, -2, sym);
} // set_number


// Sets t[sym]=f, where t is on the top of the Lua stack.
// !!! FIXME: why is this a different naming convention?
static inline void set_integer(lua_State *L, lua_Integer x, const char *sym)
{
    lua_pushinteger(L, x);
    lua_setfield(L, -2, sym);
} // set_integer


// Sets t[sym]=f, where t is on the top of the Lua stack.
// !!! FIXME: why is this a different naming convention?
static inline void set_boolean(lua_State *L, boolean x, const char *sym)
{
    lua_pushboolean(L, x);
    lua_setfield(L, -2, sym);
} // set_boolean


// !!! FIXME: why is this a different naming convention?
static inline void set_string_array(lua_State *L, int argc, const char **argv,
                                    const char *sym)
{
    int i;
    lua_newtable(L);
    for (i = 0; i < argc; i++)
    {
        lua_pushinteger(L, i+1);  // lua is option base 1!
        lua_pushstring(L, argv[i]);
        lua_settable(L, -3);
    } // for
    lua_setfield(L, -2, sym);
} // set_string_array


void MojoLua_setString(const char *str, const char *sym)
{
    lua_getglobal(luaState, MOJOSETUP_NAMESPACE);
    set_string(luaState, str, sym);
    lua_pop(luaState, 1);
} // MojoLua_setString


void MojoLua_setStringArray(int argc, const char **argv, const char *sym)
{
    lua_getglobal(luaState, MOJOSETUP_NAMESPACE);
    set_string_array(luaState, argc, argv, sym);
    lua_pop(luaState, 1);
} // MojoLua_setStringArray


static inline int retvalString(lua_State *L, const char *str)
{
    if (str != NULL)
        lua_pushstring(L, str);
    else
        lua_pushnil(L);
    return 1;
} // retvalString


static inline int retvalBoolean(lua_State *L, boolean b)
{
    lua_pushboolean(L, b);
    return 1;
} // retvalBoolean


static inline int retvalNumber(lua_State *L, lua_Number n)
{
    lua_pushnumber(L, n);
    return 1;
} // retvalNumber


static inline int retvalLightUserData(lua_State *L, void *data)
{
    if (data != NULL)
        lua_pushlightuserdata(L, data);
    else
        lua_pushnil(L);
    return 1;
} // retvalLightUserData


static int retvalChecksums(lua_State *L, const MojoChecksums *sums)
{
    lua_newtable(L);

    #if SUPPORT_CRC32
    {
        char buf[64];
        snprintf(buf, sizeof (buf), "%X", (unsigned int) sums->crc32);
        set_string(L, buf, "crc32");
    }
    #endif

    #if SUPPORT_MD5
    {
        char buf[64];
        const uint8 *dig = sums->md5;
        snprintf(buf, sizeof (buf), "%X%X%X%X%X%X%X%X%X%X%X%X%X%X%X%X",
                    (int) dig[0],  (int) dig[1],  (int) dig[2],  (int) dig[3],
                    (int) dig[4],  (int) dig[5],  (int) dig[6],  (int) dig[7],
                    (int) dig[8],  (int) dig[9],  (int) dig[10], (int) dig[11],
                    (int) dig[12], (int) dig[13], (int) dig[14], (int) dig[15]);
        set_string(L, buf, "md5");
    }
    #endif

    #if SUPPORT_SHA1
    {
        char buf[64];
        const uint8 *dig = sums->sha1;
        snprintf(buf, sizeof (buf), "%X%X%X%X%X%X%X%X%X%X%X%X%X%X%X%X%X%X%X%X",
                    (int) dig[0],  (int) dig[1],  (int) dig[2],  (int) dig[3],
                    (int) dig[4],  (int) dig[5],  (int) dig[6],  (int) dig[7],
                    (int) dig[8],  (int) dig[9],  (int) dig[10], (int) dig[11],
                    (int) dig[12], (int) dig[13], (int) dig[14], (int) dig[15],
                    (int) dig[16], (int) dig[17], (int) dig[18], (int) dig[19]);
        set_string(L, buf, "sha1");
    }
    #endif

    return 1;
} // retvalChecksums


static inline int snprintfcat(char **ptr, size_t *len, const char *fmt, ...)
{
    int bw = 0;
    va_list ap;
    va_start(ap, fmt);
    bw = vsnprintf(*ptr, *len, fmt, ap);
    va_end(ap);
    *ptr += bw;
    *len -= bw;
    return bw;
} // snprintfcat


static int luahook_stackwalk(lua_State *L)
{
    const char *errstr = lua_tostring(L, 1);
    lua_Debug ldbg;
    int i = 0;

    if (errstr != NULL)
        logDebug("%0", errstr);

    logDebug("Lua stack backtrace:");

    // start at 1 to skip this function.
    for (i = 1; lua_getstack(L, i, &ldbg); i++)
    {
        char *ptr = (char *) scratchbuf_128k;
        size_t len = sizeof (scratchbuf_128k);
        int bw = snprintfcat(&ptr, &len, "#%d", i-1);
        const int maxspacing = 4;
        int spacing = maxspacing - bw;
        while (spacing-- > 0)
            snprintfcat(&ptr, &len, " ");

        if (!lua_getinfo(L, "nSl", &ldbg))
        {
            snprintfcat(&ptr, &len, "???\n");
            logDebug("%0", (const char *) scratchbuf_128k);
            continue;
        } // if

        if (ldbg.namewhat[0])
            snprintfcat(&ptr, &len, "%s ", ldbg.namewhat);

        if ((ldbg.name) && (ldbg.name[0]))
            snprintfcat(&ptr, &len, "function %s ()", ldbg.name);
        else
        {
            if (strcmp(ldbg.what, "main") == 0)
                snprintfcat(&ptr, &len, "mainline of chunk");
            else if (strcmp(ldbg.what, "tail") == 0)
                snprintfcat(&ptr, &len, "tail call");
            else
                snprintfcat(&ptr, &len, "unidentifiable function");
        } // if

        logDebug("%0", (const char *) scratchbuf_128k);
        ptr = (char *) scratchbuf_128k;
        len = sizeof (scratchbuf_128k);

        for (spacing = 0; spacing < maxspacing; spacing++)
            snprintfcat(&ptr, &len, " ");

        if (strcmp(ldbg.what, "C") == 0)
            snprintfcat(&ptr, &len, "in native code");
        else if (strcmp(ldbg.what, "tail") == 0)
            snprintfcat(&ptr, &len, "in Lua code");
        else if ( (strcmp(ldbg.source, "=?") == 0) && (ldbg.currentline == 0) )
            snprintfcat(&ptr, &len, "in Lua code (debug info stripped)");
        else
        {
            snprintfcat(&ptr, &len, "in Lua code at %s", ldbg.short_src);
            if (ldbg.currentline != -1)
                snprintfcat(&ptr, &len, ":%d", ldbg.currentline);
        } // else
        logDebug("%0", (const char *) scratchbuf_128k);
    } // for

    return retvalString(L, errstr ? errstr : "");
} // luahook_stackwalk


// This just lets you punch in one-liners and Lua will run them as individual
//  chunks, but you can completely access all Lua state, including calling C
//  functions and altering tables. At this time, it's more of a "console"
//  than a debugger. You can do "p MojoLua_debugger()" from gdb to launch this
//  from a breakpoint in native code, or call MojoSetup.debugger() to launch
//  it from Lua code (with stacktrace intact, too: type 'bt' to see it).
static int luahook_debugger(lua_State *L)
{
#if DISABLE_LUA_PARSER
    logError("Lua debugger is disabled in this build (no parser).");
#else
    int origtop;
    const MojoSetupLogLevel origloglevel = MojoLog_logLevel;

    lua_pushcfunction(luaState, luahook_stackwalk);
    origtop = lua_gettop(L);

    printf("Quick and dirty Lua debugger. Type 'exit' to quit.\n");

    while (true)
    {
        char *buf = (char *) scratchbuf_128k;
        int len = 0;
        printf("> ");
        fflush(stdout);
        if (fgets(buf, sizeof (scratchbuf_128k), stdin) == NULL)
        {
            printf("\n\n  fgets() on stdin failed: ");
            break;
        } // if

        len = (int) (strlen(buf) - 1);
        while ( (len >= 0) && ((buf[len] == '\n') || (buf[len] == '\r')) )
            buf[len--] = '\0';

        if (strcmp(buf, "q") == 0)
            break;
        else if (strcmp(buf, "quit") == 0)
            break;
        else if (strcmp(buf, "exit") == 0)
            break;
        else if (strcmp(buf, "bt") == 0)
        {
            MojoLog_logLevel = MOJOSETUP_LOG_EVERYTHING;
            strcpy(buf, "MojoSetup.stackwalk()");
        } // else if

        if ( (luaL_loadstring(L, buf) != 0) ||
             (lua_pcall(luaState, 0, LUA_MULTRET, -2) != 0) )
        {
            printf("%s\n", lua_tostring(L, -1));
            lua_pop(L, 1);
        } // if
        else
        {
            printf("Returned %d values.\n", lua_gettop(L) - origtop);
            while (lua_gettop(L) != origtop)
            {
                // !!! FIXME: dump details of values to stdout here.
                lua_pop(L, 1);
            } // while
            printf("\n");
        } // else

        MojoLog_logLevel = origloglevel;
    } // while

    lua_pop(L, 1);
    printf("exiting debugger...\n");
#endif

    return 0;
} // luahook_debugger


void MojoLua_debugger(void)
{
    luahook_debugger(luaState);
} // MojoLua_debugger


boolean MojoLua_callProcedure(const char *funcname)
{
    boolean called = false;
    lua_State *L = luaState;
    int popcount = 0;

    if (L != NULL)
    {
        lua_getglobal(L, MOJOSETUP_NAMESPACE); popcount++;
        if (lua_istable(L, -1))  // namespace is sane?
        {
            lua_getfield(L, -1, funcname); popcount++;
            if (lua_isfunction(L, -1))
            {
                lua_call(L, 0, 0);
                called = true;
            } // if
        } // if
        lua_pop(L, popcount);
    } // if

    return called;
} // MojoLua_callProcedure


boolean MojoLua_runFileFromDir(const char *dir, const char *name)
{
    MojoArchive *ar = GBaseArchive;   // in case we want to generalize later.
    const MojoArchiveEntry *entinfo = NULL;
    boolean retval = false;
    char *clua = format("%0/%1.luac", dir, name);  // compiled filename.
    char *ulua = format("%0/%1.lua", dir, name);   // uncompiled filename.
    int rc = 0;
    MojoInput *io = NULL;

    if (ar->enumerate(ar))
    {
        while ((io == NULL) && ((entinfo = ar->enumNext(ar)) != NULL))
        {
            boolean match = false;

            if (entinfo->type != MOJOARCHIVE_ENTRY_FILE)
                continue;

            match = (strcmp(entinfo->filename, clua) == 0);
            #if !DISABLE_LUA_PARSER
            if (!match)
                match = (strcmp(entinfo->filename, ulua) == 0);
            #endif

            if (match)
                io = ar->openCurrentEntry(ar);
        } // while
    } // if

    free(ulua);
    free(clua);

    if (io != NULL)
    {
        char *realfname = (char *) xmalloc(strlen(entinfo->filename) + 2);
        sprintf(realfname, "@%s", entinfo->filename);
        lua_pushcfunction(luaState, luahook_stackwalk);
        rc = lua_load(luaState, MojoLua_reader, io, realfname);
        free(realfname);
        io->close(io);

        if (rc != 0)
            lua_error(luaState);
        else
        {
            // Call new chunk on top of the stack (lua_pcall will pop it off).
            if (lua_pcall(luaState, 0, 0, -2) != 0)  // retvals are dumped.
                lua_error(luaState);   // error on stack has debug info.
            else
                retval = true;   // if this didn't panic, we succeeded.
        } // if
        lua_pop(luaState, 1);   // dump stackwalker.
    } // if

    return retval;
} // MojoLua_runFileFromDir


boolean MojoLua_runFile(const char *name)
{
    return MojoLua_runFileFromDir("scripts", name);
} // MojoLua_runFile


void MojoLua_collectGarbage(void)
{
    lua_State *L = luaState;
    uint32 ticks = 0;
    int pre = 0;
    int post = 0;

    lua_getglobal(L, MOJOSETUP_NAMESPACE);
    if (lua_istable(L, -1))  // namespace is sane?
        set_integer(L, 0, "garbagecounter");
    lua_pop(L, 1);

    pre = (lua_gc(L, LUA_GCCOUNT, 0) * 1024) + lua_gc(L, LUA_GCCOUNTB, 0);
    logDebug("Collecting garbage (currently using %0 bytes).", numstr(pre));
    ticks = MojoPlatform_ticks();
    lua_gc (L, LUA_GCCOLLECT, 0);
    profile("Garbage collection", ticks);
    post = (lua_gc(L, LUA_GCCOUNT, 0) * 1024) + lua_gc(L, LUA_GCCOUNTB, 0);
    logDebug("Now using %0 bytes (%1 bytes savings).",
             numstr(post), numstr(pre - post));
} // MojoLua_collectGarbage


// You can trigger the garbage collector with more control in the standard
//  Lua runtime, but this notes profiling and statistics via logDebug(),
//  and resets MojoSetup.garbagecounter to zero.
static int luahook_collectgarbage(lua_State *L)
{
    MojoLua_collectGarbage();
    return 0;
} // luahook_collectgarbage


// Since localization is kept in Lua tables, I stuck this in the Lua glue.
const char *translate(const char *str)
{
    const char *retval = str;

    if (luaState != NULL)  // No translations before Lua is initialized.
    {
        if (lua_checkstack(luaState, 3))
        {
            int popcount = 0;
            lua_getglobal(luaState, MOJOSETUP_NAMESPACE); popcount++;
            if (lua_istable(luaState, -1))  // namespace is sane?
            {
                lua_getfield(luaState, -1, "translations"); popcount++;
                if (lua_istable(luaState, -1))  // translation table is sane?
                {
                    const char *tr = NULL;
                    lua_getfield(luaState, -1, str); popcount++;
                    tr = lua_tostring(luaState, -1);
                    if (tr != NULL)  // translated for this locale?
                    {
                        char *dst = (char *) scratchbuf_128k;
                        xstrncpy(dst, tr, sizeof(scratchbuf_128k));
                        retval = dst;
                    } // if
                } // if
            } // if
            lua_pop(luaState, popcount);   // remove our stack salsa.
        } // if
    } // if

    return retval;
} // translate


// Lua interface to format().
static int luahook_format(lua_State *L)
{
    const int argc = lua_gettop(L);
    const char *fmt = luaL_checkstring(L, 1);
    char *formatted = NULL;
    char *s[10];
    int i;

    assert(argc <= 11);  // fmt, plus %0 through %9.

    for (i = 0; i < STATICARRAYLEN(s); i++)
    {
        const char *str = NULL;
        if ((i+2) <= argc)
            str = lua_tostring(L, i+2);
        s[i] = (str == NULL) ? NULL : xstrdup(str);
    } // for

    // I think this is legal (but probably not moral) C code.
    formatted = format(fmt,s[0],s[1],s[2],s[3],s[4],s[5],s[6],s[7],s[8],s[9]);

    for (i = 0; i < STATICARRAYLEN(s); i++)
        free(s[i]);

    lua_pushstring(L, formatted);
    free(formatted);
    return 1;
} // luahook_format


// Use this instead of Lua's error() function if you don't have a
//  programatic error, so you don't get stack callback stuff:
// MojoSetup.fatal("You need the base game to install this expansion pack.")
//  This will also handle cleanup of half-written installations.
//  Doesn't actually return.
static int luahook_fatal(lua_State *L)
{
    const char *errstr = lua_tostring(L, 1);
    if (errstr == NULL)
        return fatal(NULL);  // doesn't actually return.
    return fatal("%0", errstr);  // doesn't actually return.
} // luahook_fatal


// Lua interface to MojoLua_runFile(). This is needed instead of Lua's
//  require(), since it can access scripts inside an archive.
static int luahook_runfile(lua_State *L)
{
    const char *fname = luaL_checkstring(L, 1);
    return retvalBoolean(L, MojoLua_runFile(fname));
} // luahook_runfile


// Lua interface to MojoLua_runFileFromDir(). This is needed instead of Lua's
//  require(), since it can access scripts inside an archive.
static int luahook_runfilefromdir(lua_State *L)
{
    const char *dir = luaL_checkstring(L, 1);
    const char *fname = luaL_checkstring(L, 2);
    return retvalBoolean(L, MojoLua_runFileFromDir(dir, fname));
} // luahook_runfile


// Lua interface to translate().
static int luahook_translate(lua_State *L)
{
    const char *str = luaL_checkstring(L, 1);
    return retvalString(L, translate(str));
} // luahook_translate


static int luahook_ticks(lua_State *L)
{
    return retvalNumber(L, MojoPlatform_ticks());
} // luahook_ticks


static int luahook_launchbrowser(lua_State *L)
{
    const char *url = luaL_checkstring(L, 1);
    return retvalBoolean(L, MojoPlatform_launchBrowser(url));
} // luahook_launchbrowser


static int luahook_verifyproductkey(lua_State *L)
{
    boolean retval = false;

    // ATTENTION: you can use this function to do your CD key verification
    //  in C code. Just remove the #if 0, and then process the ASCII string
    //  in (key), setting (retval) to (true) if the key is valid, and (false)
    //  if it isn't. Then when filling in a Setup.ProductKey in your
    //  config.lua, set (verify) to "MojoSetup.verifyproductkey".
    #if 0
    const char *key = luaL_checkstring(L, 1);
    #endif

    return retvalBoolean(L, retval);
} // luahook_verifyproductkey


static int luahook_msgbox(lua_State *L)
{
    if (GGui != NULL)
    {
        const char *title = luaL_checkstring(L, 1);
        const char *text = luaL_checkstring(L, 2);
        GGui->msgbox(title, text);
    } // if
    return 0;
} // luahook_msgbox


static int luahook_promptyn(lua_State *L)
{
    boolean rc = false;
    if (GGui != NULL)
    {
        const char *title = luaL_checkstring(L, 1);
        const char *text = luaL_checkstring(L, 2);
        const boolean defval = lua_toboolean(L, 3);
        rc = GGui->promptyn(title, text, defval);
    } // if

    return retvalBoolean(L, rc);
} // luahook_promptyn


static int luahook_promptynan(lua_State *L)
{
    MojoGuiYNAN rc = MOJOGUI_NO;
    if (GGui != NULL)
    {
        const char *title = luaL_checkstring(L, 1);
        const char *text = luaL_checkstring(L, 2);
        const boolean defval = lua_toboolean(L, 3);
        rc = GGui->promptynan(title, text, defval);
    } // if

    // Never localize these strings!
    switch (rc)
    {
        case MOJOGUI_YES: return retvalString(L, "yes");
        case MOJOGUI_NO: return retvalString(L, "no");
        case MOJOGUI_ALWAYS: return retvalString(L, "always");
        case MOJOGUI_NEVER: return retvalString(L, "never");
    } // switch

    assert(false && "BUG: unhandled case in switch statement");
    return 0;  // shouldn't hit this.
} // luahook_promptynan


static int luahook_logwarning(lua_State *L)
{
    logWarning("%0", luaL_checkstring(L, 1));
    return 0;
} // luahook_logwarning


static int luahook_logerror(lua_State *L)
{
    logError("%0", luaL_checkstring(L, 1));
    return 0;
} // luahook_logerror


static int luahook_loginfo(lua_State *L)
{
    logInfo("%0", luaL_checkstring(L, 1));
    return 0;
} // luahook_loginfo


static int luahook_logdebug(lua_State *L)
{
    logDebug("%0", luaL_checkstring(L, 1));
    return 0;
} // luahook_logdebug


static int luahook_cmdline(lua_State *L)
{
    const char *arg = luaL_checkstring(L, 1);
    return retvalBoolean(L, cmdline(arg));
} // luahook_cmdline


static int luahook_cmdlinestr(lua_State *L)
{
    const int argc = lua_gettop(L);
    const char *arg = luaL_checkstring(L, 1);
    const char *envr = (argc < 2) ? NULL : lua_tostring(L, 2);  // may be nil
    const char *deflt = (argc < 3) ? NULL : lua_tostring(L, 3);  // may be nil
    return retvalString(L, cmdlinestr(arg, envr, deflt));
} // luahook_cmdlinestr


static int luahook_truncatenum(lua_State *L)
{
    const lua_Number dbl = lua_tonumber(L, 1);
    return retvalNumber(L, (lua_Number) ((int64) dbl));
} // luahook_truncatenum


static int luahook_wildcardmatch(lua_State *L)
{
    const char *str = luaL_checkstring(L, 1);
    const char *pattern = luaL_checkstring(L, 2);
    return retvalBoolean(L, wildcardMatch(str, pattern));
} // luahook_wildcardmatch


// Do a regular C strcmp(), don't let the locale get in the way like it does
//  in Lua's string comparison operators (which uses strcoll()).
static int luahook_strcmp(lua_State *L)
{
    const char *a = luaL_checkstring(L, 1);
    const char *b = luaL_checkstring(L, 2);
    return retvalNumber(L, strcmp(a, b));
} // luahook_strcmp


static int luahook_findmedia(lua_State *L)
{
    // Let user specify overrides of directories to act as drives.
    //  This is good if for some reason MojoSetup can't find them, or they
    //  want to pretend a copy on the filesystem is really a CD, etc.
    // Put your anti-piracy crap in your OWN program.  :)
    //
    // You can specify with command lines or environment variables, command
    //  lines taking precedence:
    // MOJOSETUP_MEDIA0=/my/patch ./installer --media1=/over/there
    //
    // --media and MOJOSETUP_MEDIA are checked first, then --media0 and
    //  MOJOSETUP_MEDIA0, etc until you find the media or run out of
    //  overrides.
    //
    // After the overrides, we ask the platform layer to find the media.

    const char *unique = luaL_checkstring(L, 1);
    const char *override = cmdlinestr("media", "MOJOSETUP_MEDIA", NULL);
    char *physical = NULL;
    char cmdbuf[64];
    char envrbuf[64];
    int i = 0;

    do
    {
        if ( (override) && (MojoPlatform_exists(override, unique)) )
            return retvalString(L, override);

        snprintf(cmdbuf, sizeof (cmdbuf), "media%d", i);
        snprintf(envrbuf, sizeof (envrbuf), "MOJOSETUP_MEDIA%d", i);
    } while ((override = cmdlinestr(cmdbuf, envrbuf, NULL)) != NULL);

    // No override. Try platform layer for real media...
    physical = MojoPlatform_findMedia(unique);
    retvalString(L, physical);  // may push nil.
    free(physical);
    return 1;
} // luahook_findmedia


static boolean writeCallback(uint32 ticks, int64 justwrote, int64 bw,
                             int64 total, void *data)
{
    boolean retval = false;
    lua_State *L = (lua_State *) data;
    // Lua callback is on top of stack...
    if (lua_isnil(L, -1))
        retval = true;
    else
    {
        lua_pushvalue(L, -1);
        lua_pushnumber(L, (lua_Number) ticks);
        lua_pushnumber(L, (lua_Number) justwrote);
        lua_pushnumber(L, (lua_Number) bw);
        lua_pushnumber(L, (lua_Number) total);
        lua_call(L, 4, 1);
        retval = lua_toboolean(L, -1);
        lua_pop(L, 1);
    } // if
    return retval;
} // writeCallback


// !!! FIXME: push this into Lua, make things fatal.
static int do_writefile(lua_State *L, MojoInput *in, uint16 perms)
{
    const char *path = luaL_checkstring(L, 2);
    int retval = 0;
    boolean rc = false;
    MojoChecksums sums;
    int64 maxbytes = -1;

    if (in != NULL)
    {
        if (!lua_isnil(L, 3))
        {
            boolean valid = false;
            const char *permstr = luaL_checkstring(L, 3);
            perms = MojoPlatform_makePermissions(permstr, &valid);
            if (!valid)
                fatal(_("BUG: '%0' is not a valid permission string"), permstr);
        } // if

        if (!lua_isnil(L, 4))
            maxbytes = luaL_checkinteger(L, 4);

        rc = MojoInput_toPhysicalFile(in, path, perms, &sums, maxbytes,
                                      writeCallback, L);
    } // if

    retval += retvalBoolean(L, rc);
    if (rc)
        retval += retvalChecksums(L, &sums);
    return retval;
} // do_writefile


static int luahook_writefile(lua_State *L)
{
    MojoArchive *archive = (MojoArchive *) lua_touserdata(L, 1);
    uint16 perms = archive->prevEnum.perms;
    MojoInput *in = archive->openCurrentEntry(archive);
    return do_writefile(L, in, perms);
} // luahook_writefile


static int luahook_download(lua_State *L)
{
    const char *src = luaL_checkstring(L, 1);
    MojoInput *in = MojoInput_newFromURL(src);
    return do_writefile(L, in, MojoPlatform_defaultFilePerms());
} // luahook_download


static int luahook_copyfile(lua_State *L)
{
    const char *src = luaL_checkstring(L, 1);
    MojoInput *in = MojoInput_newFromFile(src);
    return do_writefile(L, in, MojoPlatform_defaultFilePerms());
} // luahook_copyfile


static int luahook_stringtofile(lua_State *L)
{
    const char *str = luaL_checkstring(L, 1);
    MojoInput *in = NULL;
    size_t len = 0;
    str = lua_tolstring(L, 1, &len);
    in = MojoInput_newFromMemory((const uint8 *) str, (uint32) len, 1);
    assert(in != NULL);  // xmalloc() would fatal(), should not return NULL.
    return do_writefile(L, in, MojoPlatform_defaultFilePerms());
} // luahook_stringtofile


static int luahook_isvalidperms(lua_State *L)
{
    boolean valid = false;
    const char *permstr = NULL;
    if (!lua_isnil(L, 1))
        permstr = luaL_checkstring(L, 1);
    MojoPlatform_makePermissions(permstr, &valid);
    return retvalBoolean(L, valid);
} // luahook_isvalidperms


static int do_checksum(lua_State *L, MojoInput *in)
{
    MojoChecksumContext ctx;
    MojoChecksums sums;
    int64 br = 0;

    MojoChecksum_init(&ctx);

    while (1)
    {
        br = in->read(in, scratchbuf_128k, sizeof (scratchbuf_128k));
        if (br <= 0)
            break;
        MojoChecksum_append(&ctx, scratchbuf_128k, (uint32) br);
    } // while

    MojoChecksum_finish(&ctx, &sums);

    in->close(in);

    return (br < 0) ? 0 : retvalChecksums(L, &sums);
} // do_checksum


static int luahook_checksum(lua_State *L)
{
    const char *fname = luaL_checkstring(L, 1);
    MojoInput *in = MojoInput_newFromFile(fname);
    return do_checksum(L, in);
} // luahook_checksum


static int luahook_archive_fromdir(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    return retvalLightUserData(L, MojoArchive_newFromDirectory(path));
} // luahook_archive_fromdir


static int luahook_archive_fromfile(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    MojoInput *io = MojoInput_newFromFile(path);
    MojoArchive *archive = NULL;
    if (io != NULL)
        archive = MojoArchive_newFromInput(io, path);
    return retvalLightUserData(L, archive);
} // luahook_archive_fromfile


static int luahook_archive_fromentry(lua_State *L)
{
    MojoArchive *ar = (MojoArchive *) lua_touserdata(L, 1);
    MojoInput *io = ar->openCurrentEntry(ar);
    MojoArchive *archive = NULL;
    if (io != NULL)
        archive = MojoArchive_newFromInput(io, ar->prevEnum.filename);
    return retvalLightUserData(L, archive);
} // luahook_archive_fromentry


static int luahook_archive_enumerate(lua_State *L)
{
    MojoArchive *archive = (MojoArchive *) lua_touserdata(L, 1);
    return retvalBoolean(L, archive->enumerate(archive));
} // luahook_archive_enumerate


static int luahook_archive_enumnext(lua_State *L)
{
    MojoArchive *archive = (MojoArchive *) lua_touserdata(L, 1);
    const MojoArchiveEntry *entinfo = archive->enumNext(archive);
    if (entinfo == NULL)
        lua_pushnil(L);
    else
    {
        const char *typestr = NULL;
        if (entinfo->type == MOJOARCHIVE_ENTRY_FILE)
            typestr = "file";
        else if (entinfo->type == MOJOARCHIVE_ENTRY_DIR)
            typestr = "dir";
        else if (entinfo->type == MOJOARCHIVE_ENTRY_SYMLINK)
            typestr = "symlink";
        else
            typestr = "unknown";

        lua_newtable(L);
        set_string(L, entinfo->filename, "filename");
        set_string(L, entinfo->linkdest, "linkdest");
        set_number(L, (lua_Number) entinfo->filesize, "filesize");
        set_string(L, typestr, "type");
    } // else

    return 1;
} // luahook_archive_enumnext


static int luahook_archive_close(lua_State *L)
{
    MojoArchive *archive = (MojoArchive *) lua_touserdata(L, 1);
    archive->close(archive);
    return 0;
} // luahook_archive_close


static int luahook_archive_offsetofstart(lua_State *L)
{
    MojoArchive *archive = (MojoArchive *) lua_touserdata(L, 1);
    return retvalNumber(L, (lua_Number) archive->offsetOfStart);
} // luahook_archive_offsetofstart


static int luahook_platform_unlink(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    return retvalBoolean(L, MojoPlatform_unlink(path));
} // luahook_platform_unlink


static int luahook_platform_exists(lua_State *L)
{
    const char *dir = luaL_checkstring(L, 1);
    const char *fname = lua_tostring(L, 2);  // can be nil.
    return retvalBoolean(L, MojoPlatform_exists(dir, fname));
} // luahook_platform_exists


static int luahook_platform_writable(lua_State *L)
{
    const char *fname = luaL_checkstring(L, 1);
    return retvalBoolean(L, MojoPlatform_writable(fname));
} // luahook_platform_writable


static int luahook_platform_isdir(lua_State *L)
{
    const char *dir = luaL_checkstring(L, 1);
    return retvalBoolean(L, MojoPlatform_isdir(dir));
} // luahook_platform_isdir


static int luahook_platform_issymlink(lua_State *L)
{
    const char *fname = luaL_checkstring(L, 1);
    return retvalBoolean(L, MojoPlatform_issymlink(fname));
} // luahook_platform_issymlink


static int luahook_platform_isfile(lua_State *L)
{
    const char *fname = luaL_checkstring(L, 1);
    return retvalBoolean(L, MojoPlatform_isfile(fname));
} // luahook_platform_isfile


static int luahook_platform_symlink(lua_State *L)
{
    const char *src = luaL_checkstring(L, 1);
    const char *dst = luaL_checkstring(L, 2);
    return retvalBoolean(L, MojoPlatform_symlink(src, dst));
} // luahook_platform_symlink


static int luahook_platform_mkdir(lua_State *L)
{
    const int argc = lua_gettop(L);
    const char *dir = luaL_checkstring(L, 1);
    uint16 perms = 0;
    if ( (argc < 2) || (lua_isnil(L, 2)) )
        perms = MojoPlatform_defaultDirPerms();
    else
    {
        boolean valid = false;
        const char *permstr = luaL_checkstring(L, 2);
        perms = MojoPlatform_makePermissions(permstr, &valid);
        if (!valid)
            fatal(_("BUG: '%0' is not a valid permission string"), permstr);
    } // if
    return retvalBoolean(L, MojoPlatform_mkdir(dir, perms));
} // luahook_platform_mkdir


static int luahook_platform_installdesktopmenuitem(lua_State *L)
{
    const char *data = luaL_checkstring(L, 1);
    return retvalBoolean(L, MojoPlatform_installDesktopMenuItem(data));
} // luahook_platform_installdesktopmenuitem


static int luahook_platform_uninstalldesktopmenuitem(lua_State *L)
{
    const char *data = luaL_checkstring(L, 1);
    return retvalBoolean(L, MojoPlatform_uninstallDesktopMenuItem(data));
} // luahook_platform_uninstalldesktopmenuitem


static int luahook_movefile(lua_State *L)
{
    boolean retval = false;
    const char *src = luaL_checkstring(L, 1);
    const char *dst = luaL_checkstring(L, 2);
    retval = MojoPlatform_rename(src, dst);
    if (!retval)
    {
        MojoInput *in = MojoInput_newFromFile(src);
        if (in != NULL)
        {
            uint16 perms = 0;
            MojoPlatform_perms(src, &perms);
            retval = MojoInput_toPhysicalFile(in,dst,perms,NULL,-1,NULL,NULL);
            if (retval)
            {
                retval = MojoPlatform_unlink(src);
                if (!retval)
                    MojoPlatform_unlink(dst);  // oh well.
            } // if
        } // if
    } // if

    return retvalBoolean(L, retval);
} // luahook_movefile


static void prepareSplash(MojoGuiSplash *splash, const char *fname,
                          const char *splashpos)
{
    MojoInput *io = NULL;
    int64 len = 0;

    memset(splash, '\0', sizeof (*splash));

    if (fname == NULL)
        return;

    io = MojoInput_newFromArchivePath(GBaseArchive, fname);
    if (io == NULL)
        return;

    len = io->length(io);
    if ((len > 0) && (len < 0xFFFFFFFF))
    {
        const uint32 size = (uint32) len;
        uint8 *data = (uint8 *) xmalloc(size);
        if (io->read(io, data, size) == len)
        {
            splash->rgba = decodeImage(data, size, &splash->w, &splash->h);
            if (splash->rgba != NULL)
            {
                const uint32 w = splash->w;
                const uint32 h = splash->h;
                const MojoGuiSplashPos defpos =
                    ((w >= h) ? MOJOGUI_SPLASH_TOP : MOJOGUI_SPLASH_LEFT);

                if (splashpos == NULL)
                    splash->position = defpos;
                else if ((splashpos == NULL) && (splash->w < splash->h))
                    splash->position = MOJOGUI_SPLASH_LEFT;
                else if (strcmp(splashpos, "top") == 0)
                    splash->position = MOJOGUI_SPLASH_TOP;
                else if (strcmp(splashpos, "left") == 0)
                    splash->position = MOJOGUI_SPLASH_LEFT;
                else if (strcmp(splashpos, "bottom") == 0)
                    splash->position = MOJOGUI_SPLASH_BOTTOM;
                else if (strcmp(splashpos, "right") == 0)
                    splash->position = MOJOGUI_SPLASH_RIGHT;
                else if (strcmp(splashpos, "background") == 0)
                    splash->position = MOJOGUI_SPLASH_BACKGROUND;
                else
                    splash->position = defpos;  // oh well.
            } // if
        } // if
        free(data);
    } // if

    io->close(io);
} // prepareSplash


static int luahook_gui_start(lua_State *L)
{
    const char *title = luaL_checkstring(L, 1);
    const char *splashfname = lua_tostring(L, 2);
    const char *splashpos = lua_tostring(L, 3);
    boolean rc = false;
    MojoGuiSplash splash;

    prepareSplash(&splash, splashfname, splashpos);
    rc = GGui->start(title, &splash);
    if (splash.rgba != NULL)
        free((void *) splash.rgba);

    return retvalBoolean(L, rc);
} // luahook_gui_start


static const uint8 *loadFile(const char *fname, size_t *len)
{
    uint8 *retval = NULL;
    MojoInput *io = MojoInput_newFromArchivePath(GBaseArchive, fname);
    if (io != NULL)
    {
        int64 len64 = io->length(io);
        *len = (size_t) len64;
        if (*len == len64)
        {
            retval = (uint8 *) xmalloc(*len + 1);
            if (io->read(io, retval, *len) == *len)
                retval[*len] = '\0';
            else
            {
                free(retval);
                retval = NULL;
            } // else
        } // if
        io->close(io);
    } // if

    return retval;
} // loadFile

static inline boolean canGoBack(int thisstage)
{
    return (thisstage > 1);
} // canGoBack

static inline boolean canGoForward(int thisstage, int maxstage)
{
    return (thisstage < maxstage);
} // canGoForward


static int luahook_gui_readme(lua_State *L)
{
    size_t len = 0;
    const char *name = luaL_checkstring(L, 1);
    const char *fname = luaL_checkstring(L, 2);
    const int thisstage = luaL_checkinteger(L, 3);
    const int maxstage = luaL_checkinteger(L, 4);
    const uint8 *data = loadFile(fname, &len);
    const boolean can_go_back = canGoBack(thisstage);
    const boolean can_go_fwd = canGoForward(thisstage, maxstage);

    if (data == NULL)
        fatal(_("failed to load file '%0'"), fname);

    lua_pushnumber(L, GGui->readme(name, data, len, can_go_back, can_go_fwd));
    free((void *) data);
    return 1;
} // luahook_gui_readme


static int luahook_gui_stop(lua_State *L)
{
    GGui->stop();
    return 0;
} // luahook_gui_stop


// !!! FIXME: would like to push all this tree walking into Lua, and just
// !!! FIXME:  build the final C tree without any validating here.
typedef MojoGuiSetupOptions GuiOptions;   // a little less chatty.

// forward declare this for recursive magic...
static GuiOptions *build_gui_options(lua_State *L, GuiOptions *parent);

// An option table (from Setup.Option{} or Setup.OptionGroup{}) must be at
//  the top of the Lua stack.
static GuiOptions *build_one_gui_option(lua_State *L, GuiOptions *opts,
                                        boolean is_option_group)
{
    GuiOptions *newopt = NULL;
    boolean required = false;
    boolean skipopt = false;

    lua_getfield(L, -1, "required");
    if (lua_toboolean(L, -1))
    {
        lua_pushboolean(L, true);
        lua_setfield(L, -3, "value");
        required = skipopt = true;  // don't pass to GUI.
    } // if
    lua_pop(L, 1);  // remove "required" from stack.

    // "disabled=true" trumps "required=true"
    lua_getfield(L, -1, "disabled");
    if (lua_toboolean(L, -1))
    {
        if (required)
        {
            lua_getfield(L, -2, "description");
            logWarning("Option '%0' is both required and disabled!",
                        lua_tostring(L, -1));
            lua_pop(L, 1);
        } // if
        lua_pushboolean(L, false);
        lua_setfield(L, -3, "value");
        skipopt = true;  // don't pass to GUI.
    } // if
    lua_pop(L, 1);  // remove "disabled" from stack.

    if (skipopt)  // Skip this option, but look for children in required opts.
    {
        if (required)
            newopt = build_gui_options(L, opts);
    } // if

    else  // add this option.
    {
        newopt = (GuiOptions *) xmalloc(sizeof (GuiOptions));
        newopt->is_group_parent = is_option_group;
        newopt->value = true;

        lua_getfield(L, -1, "description");
        newopt->description = xstrdup(lua_tostring(L, -1));
        lua_pop(L, 1);
    
        lua_getfield(L, -1, "tooltip");
        if (!lua_isnil(L, -1))
            newopt->tooltip = xstrdup(lua_tostring(L, -1));
        lua_pop(L, 1);

        if (!is_option_group)
        {
            lua_getfield(L, -1, "value");
            newopt->value = (lua_toboolean(L, -1) ? true : false);
            lua_pop(L, 1);
            lua_getfield(L, -1, "bytes");
            newopt->size = (int64) lua_tonumber(L, -1);
            lua_pop(L, 1);
            newopt->opaque = ((int) lua_objlen(L, 4)) + 1;
            lua_pushinteger(L, newopt->opaque);
            lua_pushvalue(L, -2);
            lua_settable(L, 4);  // position #4 is our local lookup table.
        } // if

        newopt->child = build_gui_options(L, newopt);  // look for children...
        if ((is_option_group) && (!newopt->child))  // skip empty groups.
        {
            free((void *) newopt->description);
            free((void *) newopt->tooltip);
            free(newopt);
            newopt = NULL;
        } // if
    } // else

    if (newopt != NULL)
    {
        GuiOptions *prev = NULL;  // find the end of the list...
        GuiOptions *i = newopt;
        do
        {
            prev = i;
            i = i->next_sibling;
        } while (i != NULL);
        prev->next_sibling = opts;
        opts = newopt;  // prepend to list (we'll reverse it later...)
    } // if

    return opts;
} // build_one_gui_option


static inline GuiOptions *cleanup_gui_option_list(GuiOptions *opts,
                                                  GuiOptions *parent)
{
    const boolean is_group = ((parent) && (parent->is_group_parent));
    GuiOptions *seen_enabled = NULL;
    GuiOptions *prev = NULL;
    GuiOptions *tmp = NULL;

    while (opts != NULL)
    {
        // !!! FIXME: schema should check?
        if ((is_group) && (opts->is_group_parent))
        {
            fatal("OptionGroup '%0' inside OptionGroup '%1'.",
                  opts->description, parent->description);
        } // if

        if ((is_group) && (opts->value))
        {
            if (seen_enabled)
            {
                logWarning("Options '%0' and '%1' are both enabled in group '%2'.",
                            seen_enabled->description, opts->description,
                            parent->description);
                seen_enabled->value = false;
            } // if
            seen_enabled = opts;
        } // if

        // Reverse the linked list, since we added these backwards before...
        tmp = opts->next_sibling;
        opts->next_sibling = prev;
        prev = opts;
        opts = tmp;
    } // while

    if ((prev) && (is_group) && (!seen_enabled))
    {
        logWarning("Option group '%0' has no enabled items, choosing first ('%1').",
                    parent->description, prev->description);
        prev->value = true;
    } // if
        
    return prev;
} // cleanup_gui_option_list


// the top of the stack must be the lua table with options/optiongroups.
//  We build onto (opts) "child" field.
static GuiOptions *build_gui_options(lua_State *L, GuiOptions *parent)
{
    int i = 0;
    GuiOptions *opts = NULL;
    const struct { const char *fieldname; boolean is_group; } opttype[] =
    {
        { "options", false },
        { "optiongroups", true }
    };

    for (i = 0; i < STATICARRAYLEN(opttype); i++)
    {
        const boolean is_group = opttype[i].is_group;
        lua_getfield(L, -1, opttype[i].fieldname);
        if (!lua_isnil(L, -1))
        {
            lua_pushnil(L);  // first key for iteration...
            while (lua_next(L, -2))  // replaces key, pushes value.
            {
                opts = build_one_gui_option(L, opts, is_group);
                lua_pop(L, 1);  // remove table, keep key for next iteration.
            } // while
            opts = cleanup_gui_option_list(opts, parent);
        } // if
        lua_pop(L, 1);  // pop options/optiongroups table.
    } // for

    return opts;
} // build_gui_options


// Free the tree of C structs we generated, and update the mirrored Lua tables
//  with new values...
static void done_gui_options(lua_State *L, GuiOptions *opts)
{
    if (opts != NULL)
    {
        done_gui_options(L, opts->next_sibling);
        done_gui_options(L, opts->child);

        if (opts->opaque)
        {
            // Update Lua table for this option...
            lua_pushinteger(L, opts->opaque);
            lua_gettable(L, 4);  // #4 is our local table
            lua_pushboolean(L, opts->value);
            lua_setfield(L, -2, "value");
            lua_pop(L, 1);
        } // if

        free((void *) opts->description);
        free((void *) opts->tooltip);
        free(opts);
    } // if
} // done_gui_options


static int luahook_gui_options(lua_State *L)
{
    // The options table is arg #1 (hence the assert below).
    const int thisstage = luaL_checkint(L, 2);
    const int maxstage = luaL_checkint(L, 3);
    const boolean can_go_back = canGoBack(thisstage);
    const boolean can_go_fwd = canGoForward(thisstage, maxstage);
    int rc = 1;
    GuiOptions *opts = NULL;

    assert(lua_gettop(L) == 3);

    lua_newtable(L);  // we'll use this for updating the tree later.

    // Now we need to build a tree of C structs from the hierarchical table
    //  we got from Lua...
    lua_pushvalue(L, 1);  // get the Lua table onto the top of the stack...
    opts = build_gui_options(L, NULL);
    lua_pop(L, 1);  // pop the Lua table off the top of the stack...

    if (opts != NULL)  // if nothing to do, we'll go directly to next stage.
        rc = GGui->options(opts, can_go_back, can_go_fwd);

    done_gui_options(L, opts);  // free C structs, update Lua tables...
    lua_pop(L, 1);  // pop table we created.

    return retvalNumber(L, rc);
} // luahook_gui_options


static int luahook_gui_destination(lua_State *L)
{
    const int thisstage = luaL_checkinteger(L, 2);
    const int maxstage = luaL_checkinteger(L, 3);
    const boolean can_go_back = canGoBack(thisstage);
    const boolean can_go_fwd = canGoForward(thisstage, maxstage);
    char **recommend = NULL;
    size_t reccount = 0;
    char *rc = NULL;
    int command = 0;
    size_t i = 0;

    if (lua_istable(L, 1))
    {
        reccount = lua_objlen(L, 1);
        recommend = (char **) xmalloc(reccount * sizeof (char *));
        for (i = 0; i < reccount; i++)
        {
            lua_pushinteger(L, i+1);
            lua_gettable(L, 1);
            recommend[i] = xstrdup(lua_tostring(L, -1));
            lua_pop(L, 1);
        } // for
    } // if

    rc = GGui->destination((const char **) recommend, reccount,
                            &command, can_go_back, can_go_fwd);

    if (recommend != NULL)
    {
        for (i = 0; i < reccount; i++)
            free(recommend[i]);
        free(recommend);
    } // if

    retvalNumber(L, command);
    retvalString(L, rc);  // may push nil.
    free(rc);
    return 2;
} // luahook_gui_destination


// make sure spaces and dashes make it into the string.
//  this counts on (buf) being correctly allocated!
static void sanitize_productkey(const char *fmt, char *buf)
{
    char fmtch;

    if (fmt == NULL)
        return;

    while ((fmtch = *(fmt++)) != '\0')
    {
        const char bufch = *buf;
        if ((fmtch == ' ') || (fmtch == '-'))
        {
            if ((bufch != ' ') && (bufch != '-'))
                memmove(buf + 1, buf, strlen(buf) + 1);
            *buf = fmtch;
        } // else if

        if (bufch != '\0')
            buf++;
    } // while
} // sanitize_productkey


static int luahook_gui_productkey(lua_State *L)
{
    const char *desc = luaL_checkstring(L, 1);
    const char *fmt = lua_tostring(L, 2);
    const char *defval = lua_tostring(L, 3);
    const int thisstage = luaL_checkinteger(L, 4);
    const int maxstage = luaL_checkinteger(L, 5);
    const boolean can_go_back = canGoBack(thisstage);
    const boolean can_go_fwd = canGoForward(thisstage, maxstage);
    const int fmtlen = fmt ? ((int) strlen(fmt) + 1) : 32;
    char *buf = (char *) xmalloc(fmtlen);
    int cmd = 0;

    assert((defval == NULL) || (((int)strlen(defval)) < fmtlen));
    strcpy(buf, (defval == NULL) ? "" : defval);

    cmd = GGui->productkey(desc, fmt, buf, fmtlen, can_go_back, can_go_fwd);
    if (cmd == 1)
        sanitize_productkey(fmt, buf);
    else
    {
        free(buf);
        buf = NULL;
    } // else
    lua_pushinteger(L, cmd);
    lua_pushstring(L, buf);  // may be NULL
    free(buf);
    return 2;
} // luahook_gui_productkey


static int luahook_gui_insertmedia(lua_State *L)
{
    const char *unique = luaL_checkstring(L, 1);
    return retvalBoolean(L, GGui->insertmedia(unique));
} // luahook_gui_insertmedia


static int luahook_gui_progressitem(lua_State *L)
{
    GGui->progressitem();
    return 0;
} // luahook_gui_progressitem


static int luahook_gui_progress(lua_State *L)
{
    const char *type = luaL_checkstring(L, 1);
    const char *component = luaL_checkstring(L, 2);
    const int percent = luaL_checkint(L, 3);
    const char *item = luaL_checkstring(L, 4);
    const boolean canstop = lua_toboolean(L, 5);
    const boolean rc = GGui->progress(type, component, percent, item, canstop);
    return retvalBoolean(L, rc);
} // luahook_gui_progress


static int luahook_gui_final(lua_State *L)
{
    const char *msg = luaL_checkstring(L, 1);
    GGui->final(msg);
    return 0;
} // luahook_gui_final


static const char *logLevelString(void)
{
    switch (MojoLog_logLevel)
    {
        case MOJOSETUP_LOG_NOTHING: return "nothing";
        case MOJOSETUP_LOG_ERRORS: return "errors";
        case MOJOSETUP_LOG_WARNINGS: return "warnings";
        case MOJOSETUP_LOG_INFO: return "info";
        case MOJOSETUP_LOG_DEBUG: return "debug";
        case MOJOSETUP_LOG_EVERYTHING: default: return "everything";
    } // switch
} // logLevelString


static void registerLuaLibs(lua_State *L)
{
    // We always need the string and base libraries (although base has a
    //  few we could trim). The rest you can compile in if you want/need them.
    int i;
    static const luaL_Reg lualibs[] = {
        {"", luaopen_base},
        {LUA_STRLIBNAME, luaopen_string},
        {LUA_TABLIBNAME, luaopen_table},
        #if SUPPORT_LUALIB_PACKAGE
        {LUA_LOADLIBNAME, luaopen_package},
        #endif
        #if SUPPORT_LUALIB_IO
        {LUA_IOLIBNAME, luaopen_io},
        #endif
        #if SUPPORT_LUALIB_OS
        {LUA_OSLIBNAME, luaopen_os},
        #endif
        #if SUPPORT_LUALIB_MATH
        {LUA_MATHLIBNAME, luaopen_math},
        #endif
        #if SUPPORT_LUALIB_DB
        {LUA_DBLIBNAME, luaopen_debug},
        #endif
    };

    for (i = 0; i < STATICARRAYLEN(lualibs); i++)
    {
        lua_pushcfunction(L, lualibs[i].func);
        lua_pushstring(L, lualibs[i].name);
        lua_call(L, 1, 0);
    } // for
} // registerLuaLibs


// !!! FIXME: platform layer?
static int luahook_date(lua_State *L)
{
    const char *datefmt = "%c";  // workaround stupid gcc warning.
    char buf[128];
    time_t t = time(NULL);
    strftime(buf, sizeof (buf), datefmt, gmtime(&t));
    return retvalString(L, buf);
} // luahook_date


boolean MojoLua_initLua(void)
{
    const char *envr = cmdlinestr("locale", "MOJOSETUP_LOCALE", NULL);
    char *homedir = MojoPlatform_homedir();
    char *binarypath = MojoPlatform_appBinaryPath();
    char *locale = (envr != NULL) ? xstrdup(envr) : MojoPlatform_locale();
    char *ostype = MojoPlatform_osType();
    char *osversion = MojoPlatform_osVersion();
    lua_Integer uid = (lua_Integer) MojoPlatform_getuid();
    lua_Integer euid = (lua_Integer) MojoPlatform_geteuid();
    lua_Integer gid = (lua_Integer) MojoPlatform_getgid();

    #if DISABLE_LUA_PARSER
    const boolean luaparser = false;
    #else
    const boolean luaparser = true;
    #endif

    if (locale == NULL) locale = xstrdup("???");
    if (ostype == NULL) ostype = xstrdup("???");
    if (osversion == NULL) osversion = xstrdup("???");

    assert(luaState == NULL);
    luaState = lua_newstate(MojoLua_alloc, NULL);  // calls fatal() on failure.
    lua_atpanic(luaState, luahook_fatal);
    assert(lua_checkstack(luaState, 20));  // Just in case.
    registerLuaLibs(luaState);

    // !!! FIXME: I'd like to change the function name case for the lua hooks.

    // Build MojoSetup namespace for Lua to access and fill in C bridges...
    lua_newtable(luaState);
        // Set up initial C functions, etc we want to expose to Lua code...
        set_cfunc(luaState, luahook_runfile, "runfile");
        set_cfunc(luaState, luahook_runfilefromdir, "runfilefromdir");
        set_cfunc(luaState, luahook_translate, "translate");
        set_cfunc(luaState, luahook_ticks, "ticks");
        set_cfunc(luaState, luahook_format, "format");
        set_cfunc(luaState, luahook_fatal, "fatal");
        set_cfunc(luaState, luahook_launchbrowser, "launchbrowser");
        set_cfunc(luaState, luahook_verifyproductkey, "verifyproductkey");
        set_cfunc(luaState, luahook_msgbox, "msgbox");
        set_cfunc(luaState, luahook_promptyn, "promptyn");
        set_cfunc(luaState, luahook_promptynan, "promptynan");
        set_cfunc(luaState, luahook_stackwalk, "stackwalk");
        set_cfunc(luaState, luahook_logwarning, "logwarning");
        set_cfunc(luaState, luahook_logerror, "logerror");
        set_cfunc(luaState, luahook_loginfo, "loginfo");
        set_cfunc(luaState, luahook_logdebug, "logdebug");
        set_cfunc(luaState, luahook_cmdline, "cmdline");
        set_cfunc(luaState, luahook_cmdlinestr, "cmdlinestr");
        set_cfunc(luaState, luahook_collectgarbage, "collectgarbage");
        set_cfunc(luaState, luahook_debugger, "debugger");
        set_cfunc(luaState, luahook_findmedia, "findmedia");
        set_cfunc(luaState, luahook_writefile, "writefile");
        set_cfunc(luaState, luahook_copyfile, "copyfile");
        set_cfunc(luaState, luahook_stringtofile, "stringtofile");
        set_cfunc(luaState, luahook_download, "download");
        set_cfunc(luaState, luahook_movefile, "movefile");
        set_cfunc(luaState, luahook_wildcardmatch, "wildcardmatch");
        set_cfunc(luaState, luahook_truncatenum, "truncatenum");
        set_cfunc(luaState, luahook_date, "date");
        set_cfunc(luaState, luahook_isvalidperms, "isvalidperms");
        set_cfunc(luaState, luahook_checksum, "checksum");
        set_cfunc(luaState, luahook_strcmp, "strcmp");

        // Set some information strings...
        lua_newtable(luaState);
            set_string(luaState, locale, "locale");
            set_string(luaState, PLATFORM_NAME, "platform");
            set_string(luaState, PLATFORM_ARCH, "arch");
            set_string(luaState, ostype, "ostype");
            set_string(luaState, osversion, "osversion");
            set_string(luaState, GGui->name(), "ui");
            set_string(luaState, GBuildVer, "buildver");
            set_string(luaState, GMojoSetupLicense, "license");
            set_string(luaState, GLuaLicense, "lualicense");
            set_string(luaState, logLevelString(), "loglevel");
            set_string(luaState, homedir, "homedir");
            set_string(luaState, binarypath, "binarypath");
            set_string(luaState, GBaseArchivePath, "basearchivepath");
            set_boolean(luaState, luaparser, "luaparser");
            set_integer(luaState, uid, "uid");
            set_integer(luaState, euid, "euid");
            set_integer(luaState, gid, "gid");
            set_string_array(luaState, GArgc, GArgv, "argv");
            lua_newtable(luaState);
                set_string(luaState, "base", "base");
                set_string(luaState, "media", "media");
                #if SUPPORT_URL_FTP
                set_string(luaState, "ftp", "ftp");
                #endif
                #if SUPPORT_URL_HTTP
                set_string(luaState, "http", "http");
                #endif
                #if SUPPORT_URL_HTTP
                set_string(luaState, "https", "https");
                #endif
            lua_setfield(luaState, -2, "supportedurls");
        lua_setfield(luaState, -2, "info");

        // Set the platform functions...
        lua_newtable(luaState);
            set_cfunc(luaState, luahook_platform_unlink, "unlink");
            set_cfunc(luaState, luahook_platform_exists, "exists");
            set_cfunc(luaState, luahook_platform_writable, "writable");
            set_cfunc(luaState, luahook_platform_isdir, "isdir");
            set_cfunc(luaState, luahook_platform_issymlink, "issymlink");
            set_cfunc(luaState, luahook_platform_isfile, "isfile");
            set_cfunc(luaState, luahook_platform_symlink, "symlink");
            set_cfunc(luaState, luahook_platform_mkdir, "mkdir");
            set_cfunc(luaState, luahook_platform_installdesktopmenuitem, "installdesktopmenuitem");
            set_cfunc(luaState, luahook_platform_uninstalldesktopmenuitem, "uninstalldesktopmenuitem");
        lua_setfield(luaState, -2, "platform");

        // Set the GUI functions...
        lua_newtable(luaState);
            set_cfunc(luaState, luahook_gui_start, "start");
            set_cfunc(luaState, luahook_gui_readme, "readme");
            set_cfunc(luaState, luahook_gui_options, "options");
            set_cfunc(luaState, luahook_gui_destination, "destination");
            set_cfunc(luaState, luahook_gui_productkey, "productkey");
            set_cfunc(luaState, luahook_gui_insertmedia, "insertmedia");
            set_cfunc(luaState, luahook_gui_progressitem, "progressitem");
            set_cfunc(luaState, luahook_gui_progress, "progress");
            set_cfunc(luaState, luahook_gui_final, "final");
            set_cfunc(luaState, luahook_gui_stop, "stop");
        lua_setfield(luaState, -2, "gui");

        // Set the i/o functions...
        lua_newtable(luaState);
            set_cfunc(luaState, luahook_archive_fromdir, "fromdir");
            set_cfunc(luaState, luahook_archive_fromfile, "fromfile");
            set_cfunc(luaState, luahook_archive_fromentry, "fromentry");
            set_cfunc(luaState, luahook_archive_enumerate, "enumerate");
            set_cfunc(luaState, luahook_archive_enumnext, "enumnext");
            set_cfunc(luaState, luahook_archive_close, "close");
            set_cfunc(luaState, luahook_archive_offsetofstart, "offsetofstart");
            set_cptr(luaState, GBaseArchive, "base");
        lua_setfield(luaState, -2, "archive");
    lua_setglobal(luaState, MOJOSETUP_NAMESPACE);

    free(osversion);
    free(ostype);
    free(locale);
    free(binarypath);
    free(homedir);

    // Transfer control to Lua to setup some APIs and state...
    if (!MojoLua_runFile("mojosetup_init"))
        return false;

    MojoLua_collectGarbage();  // get rid of old init crap we don't need.

    return true;
} // MojoLua_initLua


boolean MojoLua_initialized(void)
{
    return (luaState != NULL);
} // MojoLua_initialized


void MojoLua_deinitLua(void)
{
    if (luaState != NULL)
    {
        lua_close(luaState);
        luaState = NULL;
    } // if
} // MojoLua_deinitLua



const char *GMojoSetupLicense =
"Copyright (c) 2007 Ryan C. Gordon and others.\n"
"\n"
"This software is provided 'as-is', without any express or implied warranty.\n"
"In no event will the authors be held liable for any damages arising from\n"
"the use of this software.\n"
"\n"
"Permission is granted to anyone to use this software for any purpose,\n"
"including commercial applications, and to alter it and redistribute it\n"
"freely, subject to the following restrictions:\n"
"\n"
"1. The origin of this software must not be misrepresented; you must not\n"
"claim that you wrote the original software. If you use this software in a\n"
"product, an acknowledgment in the product documentation would be\n"
"appreciated but is not required.\n"
"\n"
"2. Altered source versions must be plainly marked as such, and must not be\n"
"misrepresented as being the original software.\n"
"\n"
"3. This notice may not be removed or altered from any source distribution.\n"
"\n"
"    Ryan C. Gordon <icculus@icculus.org>\n"
"\n";


const char *GLuaLicense =
"Lua:\n"
"\n"
"Copyright (C) 1994-2008 Lua.org, PUC-Rio.\n"
"\n"
"Permission is hereby granted, free of charge, to any person obtaining a copy\n"
"of this software and associated documentation files (the \"Software\"), to deal\n"
"in the Software without restriction, including without limitation the rights\n"
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n"
"copies of the Software, and to permit persons to whom the Software is\n"
"furnished to do so, subject to the following conditions:\n"
"\n"
"The above copyright notice and this permission notice shall be included in\n"
"all copies or substantial portions of the Software.\n"
"\n"
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE\n"
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n"
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN\n"
"THE SOFTWARE.\n"
"\n";

// !!! FIXME: need BSD and MIT licenses...put all the licenses in one string.

// end of lua_glue.c ...

