/*
* min.c -- a minimal Lua interpreter
* loads stdin only with minimal error handling.
* no interaction, and no standard library, only a "print" function.
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

#include <stdio.h>

#include "lua.h"
#include "lauxlib.h"

static int print(lua_State *L)
{
 int n=lua_gettop(L);
 int i;
 for (i=1; i<=n; i++)
 {
  if (i>1) printf("\t");
  if (lua_isstring(L,i))
   printf("%s",lua_tostring(L,i));
  else if (lua_isnil(L,i))
   printf("%s","nil");
  else if (lua_isboolean(L,i))
   printf("%s",lua_toboolean(L,i) ? "true" : "false");
  else
   printf("%s:%p",luaL_typename(L,i),lua_topointer(L,i));
 }
 printf("\n");
 return 0;
}

int main(void)
{
 lua_State *L=lua_open();
 lua_register(L,"print",print);
 if (luaL_dofile(L,NULL)!=0) fprintf(stderr,"%s\n",lua_tostring(L,-1));
 lua_close(L);
 return 0;
}
