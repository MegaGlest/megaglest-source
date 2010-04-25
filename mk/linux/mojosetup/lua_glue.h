/**
 * MojoSetup; a portable, flexible installation application.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

#ifndef _INCL_LUA_GLUE_H_
#define _INCL_LUA_GLUE_H_

#include "universal.h"

#ifdef __cplusplus
extern "C" {
#endif

// License text for MojoSetup.
extern const char *GMojoSetupLicense;

// License text for Lua.
extern const char *GLuaLicense;


boolean MojoLua_initLua(void);
void MojoLua_deinitLua(void);
boolean MojoLua_initialized(void);

// Run the code in a given Lua file. This is JUST the base filename.
//  We will look for it in GBaseArchive in the (dir) directory, both as
//  fname.luac and fname.lua. This code chunk will accept no arguments, and
//  return no results, but it can change the global state and alter tables,
//  etc, so it can have lasting side effects.
// Will return false if the file couldn't be loaded, or true if the chunk
//  successfully ran. Will not return if there's a runtime error in the
//  chunk, as it will call fatal() instead.
boolean MojoLua_runFileFromDir(const char *dir, const char *name);

// This is shorthand for MojoLua_runFileFromDir("scripts", fname);
boolean MojoLua_runFile(const char *fname);

// Call a function in Lua. This calls MojoSetup.funcname, if it exists and
//  is a function. It will not pass any parameters and it will not return
//  any values. The call is made unprotected, so if Lua triggers an error,
//  this C function will not return. Don't use this if you don't know what
//  you're doing.
// Returns true if function was called, false otherwise.
boolean MojoLua_callProcedure(const char *funcname);

// Set a Lua variable in the MojoSetup namespace to a string:
//  MojoLua_setString("bob", "name");
//  in Lua: print(MojoSetup.name)  -- outputs: bob
void MojoLua_setString(const char *str, const char *sym);

// Same as MojoLua_setString, but it creates an ordered table (array).
void MojoLua_setStringArray(int argc, const char **argv, const char *sym);

void MojoLua_collectGarbage(void);

void MojoLua_debugger(void);

#ifdef __cplusplus
}
#endif

#endif

// end of lua_glue.h ...

