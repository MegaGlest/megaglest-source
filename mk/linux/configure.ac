#----------------------------------------------------------------------------
# Autoconf input script. Start the ./autgen.sh script for producing a
# configure script.
#----------------------------------------------------------------------------
AC_PREREQ([2.54])

AC_INIT([megaglest], [3.3.2], [matze@braunis.de])
AC_CONFIG_SRCDIR([mk/jam/build.jam])
AC_CONFIG_AUX_DIR([mk/autoconf])

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

#----------------------------------------------------------------------------
# Configuration header
#----------------------------------------------------------------------------
AC_C_BIGENDIAN()

# stupid autoconf is adding default -g -O2 flags when we don't want this :-/
test ".$CXXFLAGS" = "." && CXXFLAGS=" "

#----------------------------------------------------------------------------
# Check for build variant (debug, profile, optimize)
#----------------------------------------------------------------------------
VARIANT=optimize
AC_ARG_ENABLE([optimize], [AC_HELP_STRING([--enable-optimize],
    [build with optimizations enabled (default YES)])],
    [test "$enableval" = "yes" && VARIANT=optimize])
                                                                                
AC_ARG_ENABLE([debug], [AC_HELP_STRING([--enable-debug],
    [build with debugging information (default NO)])],
    [test "$enableval" = "yes" && VARIANT=debug])
                                                                                
AC_ARG_ENABLE([profile], [AC_HELP_STRING([--enable-profile],
    [build with profiling information (default NO)])],
    [test "$enableval" = "yes" && VARIANT=profile])
AC_SUBST([VARIANT])

#----------------------------------------------------------------------------
# find applications
#----------------------------------------------------------------------------
AC_PROG_CXX
AC_PROG_INSTALL

#----------------------------------------------------------------------------
# Check for operating system
#----------------------------------------------------------------------------
AC_MSG_CHECKING([for target host])
winapi_available=no
case $host_os in
    mingw*|cygwin*)
        AC_MSG_RESULT([WIN32])
		winapi_available=yes
        AC_DEFINE([USE_WINSOCK], [], [Define if winsock should be used])
        LIBS="$LIBS -lwsock32"
        ;;
    *)
        AC_MSG_RESULT([assume posix])
		AC_DEFINE([USE_POSIX_SOCKETS], [],
				[Define if posix sockets should be used])

		AC_PATH_X
		if test "$no_x" != "yes"; then                                   		
			AC_DEFINE([X11_AVAILABLE], [1], [Defined if X11 is avilable])
		else
			AC_MSG_ERROR("Please install X11 libraries and headers.")
		fi
        ;;
esac

#----------------------------------------------------------------------------
# find libraries
#----------------------------------------------------------------------------
AC_CHECK_HEADERS([glob.h sys/ioctl.h sys/filio.h])

AX_TYPE_SOCKLEN_T

AM_PATH_SDL([1.2.5], 
	[AC_DEFINE([USE_SDL], [], [Define when SDL should be used])], [
if test "$winapi_available" != "yes"; then
	AC_MSG_ERROR([Please install libsdl >= 1.2.])
else
	AC_DEFINE([USE_WINAPI], [], [Define if win32 API should be used])
fi
])

AC_LANG_PUSH([C++])
NP_FINDLIB([XERCES], [xerces], [xerces],
		NP_LANG_PROGRAM([#include <xercesc/util/PlatformUtils.hpp>
XERCES_CPP_NAMESPACE_USE;], [XMLPlatformUtils::Initialize();]),
		[], [-lxerces-c -lpthread],
		[],
		[AC_MSG_ERROR([Please install xerces-c])],
		[], [])
AC_LANG_POP([C++])

NP_FINDLIB([OPENAL], [OpenAL], [OpenAL],
		NP_LANG_PROGRAM([#include <AL/al.h>],
			[alcOpenDevice(0);]),
		[], [-lopenal],
		[],
		[AC_MSG_ERROR([Please intall OpenAL])],
		[], [])

NP_FINDLIB([JPEG], [LibJPEG], [LibJPEG],
		NP_LANG_PROGRAM([
		#include <stdio.h>
		#include <sys/types.h>
		#include <jpeglib.h>],
			[jpeg_create_decompress(0);]),
		[], [-ljpeg],
		[],
		[AC_MSG_ERROR([Please intall libjpeg])],
		[], [])

NP_FINDLIB([PNG], [LibPng], [LibPNG],
		NP_LANG_PROGRAM([#include <png.h>],
			[png_sig_cmp(0, 0, 8);]),
		[], [-lpng],
		[],
		[AC_MSG_ERROR([Please intall libpng])],
		[], [])

CHECK_LUA([], [AC_MSG_ERROR([Please install lua 5.1])])

AX_CHECK_GL
if test "$no_gl" = "yes"; then
    AC_MSG_ERROR([You need to have OpenGL headers and libraries installed])
fi
AX_CHECK_GLU
if test "$no_glu" = "yes"; then
    AC_MSG_ERROR([You need to have GLU headers and libraries installed])
fi

XIPH_PATH_VORBIS(, [AC_MSG_ERROR([Please install vorbis])])
XIPH_PATH_OGG(, [AC_MSG_ERROR([Please install ogg/vorbis])])

AM_OPTIONS_WXCONFIG
AM_PATH_WXCONFIG([2.6.0], [WX_AVAILABLE="yes"], [WX_AVAILABLE="no"], [std,gl], [--unicode=yes])
AC_SUBST([WX_AVAILABLE])

AC_INIT_JAM
AC_CONFIG_FILES([Jamconfig])
AC_OUTPUT

AC_MSG_NOTICE([

Notice: This project uses jam (and not make) as build tool.
])
