/**
 * MojoSetup; a portable, flexible installation application.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 *
    Copyright (c) 2006-2010 Ryan C. Gordon and others.

   This software is provided 'as-is', without any express or implied warranty.
   In no event will the authors be held liable for any damages arising from
   the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software in a
   product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source distribution.

       Ryan C. Gordon <icculus@icculus.org>
 *
 */

/*
 * This is in a separate file so that we can recompile it every time
 *  without it forcing a recompile on something ccache would otherwise not
 *  have to rebuild...this file's checksum changes every time you build it
 *  due to the __DATE__ and __TIME__ macros.
 *
 * The makefile will rebuild this file everytime it relinks an executable
 *  so that we'll always have a unique build string.
 *
 * APPNAME and APPREV need to be predefined in the build system.
 *  The rest are supposed to be supplied by the compiler.
 */

#ifndef APPID
#error Please define APPID in the build system.
#endif

#ifndef APPREV
#error Please define APPREV in the build system.
#endif

#if (defined __GNUC__)
#   define VERSTR2(x) #x
#   define VERSTR(x) VERSTR2(x)
#   define COMPILERVER " " VERSTR(__GNUC__) "." VERSTR(__GNUC_MINOR__) "." VERSTR(__GNUC_PATCHLEVEL__)
#elif (defined __SUNPRO_C)
#   define VERSTR2(x) #x
#   define VERSTR(x) VERSTR2(x)
#   define COMPILERVER " " VERSTR(__SUNPRO_C)
#elif (defined __VERSION__)
#   define COMPILERVER " " __VERSION__
#else
#   define COMPILERVER ""
#endif

#ifndef __DATE__
#define __DATE__ "(Unknown build date)"
#endif

#ifndef __TIME__
#define __TIME__ "(Unknown build time)"
#endif

#ifndef COMPILER
  #if (defined __GNUC__)
    #define COMPILER "GCC"
  #elif (defined _MSC_VER)
    #define COMPILER "Visual Studio"
  #elif (defined __SUNPRO_C)
    #define COMPILER "Sun Studio"
  #else
    #error Please define your platform.
  #endif
#endif

// macro mess so we can turn APPID and APPREV into a string literal...
#define MAKEBUILDVERSTRINGLITERAL2(id, rev) \
    #id ", revision " #rev ", built " __DATE__ " " __TIME__ \
    ", by " COMPILER COMPILERVER

#define MAKEBUILDVERSTRINGLITERAL(id, rev) MAKEBUILDVERSTRINGLITERAL2(id, rev)

const char *GBuildVer = MAKEBUILDVERSTRINGLITERAL(APPID, APPREV);

// end of buildver.c ...

