# derived from lua.m4 in mod_wombat from httpd.apache.org
# http://svn.apache.org/viewvc/httpd/mod_wombat/trunk/build/ac-macros/lua.m4?view=markup
# which is under Apache License, Version 2.0, http://www.apache.org/licenses/LICENSE-2.0
#
# Apache mod_wombat
# Copyright 2006 The Apache Software Foundation
#
# This product includes software developed at
# The Apache Software Foundation (http://www.apache.org/).
#
# This software makes use of Lua ( http://www.lua.org/ ) 
# developed by Copyright 1994-2006 Lua.org, PUC-Rio. 
# Lua is distributed under the MIT license 
# ( http://www.lua.org/license.html ).
#
# ==========================================
# Check for Lua 5.1 Libraries
# CHECK_LUA(ACTION-IF-FOUND, ACTION-IF-NOT-FOUND)
# Sets:
#  LUA_AVAILABLE
#  LUA_CFLAGS
#  LUA_LIBS
AC_DEFUN([CHECK_LUA],
[

AC_ARG_WITH(
    lua,
    [AC_HELP_STRING([--with-lua=PFX],[Prefix where Lua 5.1 is installed (optional)])],
    lua_pfx="$withval",
    :)

# Determine lua lib directory
if test -z "${lua_pfx}"; then
    paths="/usr/local /usr"
else
    paths="${lua_pfx}"
fi

[LUA_AVAILABLE=no
 LUA_LIBS=""
 LUA_CFLAGS=""]

AC_CHECK_LIB([m], [pow], lib_m=" -lm")
AC_LANG_PUSH([C++])
for p in $paths ; do
    AC_MSG_CHECKING([for lua.hpp in ${p}/include/lua5.1])
    if test -f ${p}/include/lua5.1/lua.hpp; then
        AC_MSG_RESULT([yes])
        save_CFLAGS=$CFLAGS
        save_LDFLAGS=$LDFLAGS
        CFLAGS="$CFLAGS"
        LDFLAGS="-L${p}/lib $LDFLAGS $lib_m"
        AC_CHECK_LIB(lua5.1, luaL_newstate,
            [
            LUA_AVAILABLE=yes
            LUA_LIBS="-L${p}/lib -llua5.1"
            LUA_CFLAGS="-I${p}/include/lua5.1"
            ])
        CFLAGS=$save_CFLAGS
        LDFLAGS=$save_LDFLAGS
        break
    else
        AC_MSG_RESULT([no])
    fi
    AC_MSG_CHECKING([for lua.hpp in ${p}/include])
    if test -f ${p}/include/lua.hpp; then
        AC_MSG_RESULT([yes])
        save_CFLAGS=$CFLAGS
        save_LDFLAGS=$LDFLAGS
        CFLAGS="$CFLAGS"
        LDFLAGS="-L${p}/lib $LDFLAGS $lib_m"
        AC_CHECK_LIB(lua, luaL_newstate,
            [
            LUA_AVAILABLE=yes
            LUA_LIBS="-L${p}/lib -llua"
            LUA_CFLAGS="-I${p}/include"
            ])
        CFLAGS=$save_CFLAGS
        LDFLAGS=$save_LDFLAGS
        break
    else
        AC_MSG_RESULT([no])
    fi
done
AC_LANG_POP([C++])

AC_SUBST(LUA_AVAILABLE)
AC_SUBST(LUA_LIBS)
AC_SUBST(LUA_CFLAGS)

if test -z "${LUA_LIBS}"; then
    ifelse([$2], , :, [$2])
else
    ifelse([$1], , :, [$1])
fi

])
