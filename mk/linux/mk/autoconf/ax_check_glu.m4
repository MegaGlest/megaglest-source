dnl @synopsis AX_CHECK_GLU
dnl
dnl Check for GLU. If GLU is found, the required preprocessor and
dnl linker flags are included in the output variables "GLU_CFLAGS" and
dnl "GLU_LIBS", respectively. This macro adds the configure option
dnl "--with-apple-opengl-framework", which users can use to indicate
dnl that Apple's OpenGL framework should be used on Mac OS X. If
dnl Apple's OpenGL framework is used, the symbol
dnl "HAVE_APPLE_OPENGL_FRAMEWORK" is defined. If no GLU implementation
dnl is found, "no_glu" is set to "yes".
dnl
dnl @category InstalledPackages
dnl @author Braden McDaniel <braden@endoframe.com>
dnl @version 2004-11-15
dnl @license AllPermissive

AC_DEFUN([AX_CHECK_GLU],
[AC_REQUIRE([AX_CHECK_GL])dnl
AC_REQUIRE([AC_PROG_CXX])dnl
GLU_CFLAGS="${GL_CFLAGS}"
if test "X${with_apple_opengl_framework}" != "Xyes"; then
  AC_CACHE_CHECK([for OpenGL Utility library], [ax_cv_check_glu_libglu],
  [ax_cv_check_glu_libglu="no"
  ax_save_CPPFLAGS="${CPPFLAGS}"
  CPPFLAGS="${GL_CFLAGS} ${CPPFLAGS}"
  ax_save_LIBS="${LIBS}"
  LIBS=""
  ax_check_libs="-lglu32 -lGLU"
  for ax_lib in ${ax_check_libs}; do
    if test X$ax_compiler_ms = Xyes; then
      ax_try_lib=`echo $ax_lib | sed -e 's/^-l//' -e 's/$/.lib/'`
    else
      ax_try_lib="${ax_lib}"
    fi
    LIBS="${ax_try_lib} ${GL_LIBS} ${ax_save_LIBS}"
    #
    # libGLU typically links with libstdc++ on POSIX platforms. However,
    # setting the language to C++ means that test program source is named
    # "conftest.cc"; and Microsoft cl doesn't know what to do with such a
    # file.
    #
    AC_LANG_PUSH([C++])
    if test X$ax_compiler_ms = Xyes; then
      AC_LANG_PUSH([C])
    fi
    AC_LINK_IFELSE(
    [AC_LANG_PROGRAM([[
# if HAVE_WINDOWS_H && defined(_WIN32)
#   include <windows.h>
# endif
# include <GL/glu.h>]],
                     [[gluBeginCurve(0)]])],
    [ax_cv_check_glu_libglu="${ax_try_lib}"; break])
    if test X$ax_compiler_ms = Xyes; then
      AC_LANG_POP([C])
    fi
    AC_LANG_POP([C++])
  done
  LIBS=${ax_save_LIBS}
  CPPFLAGS=${ax_save_CPPFLAGS}])
  if test "X${ax_cv_check_glu_libglu}" = "Xno"; then
    no_glu="yes"
    GLU_CFLAGS=""
    GLU_LIBS=""
  else
    GLU_LIBS="${ax_cv_check_glu_libglu} ${GL_LIBS}"
  fi
fi
AC_SUBST([GLU_CFLAGS])
AC_SUBST([GLU_LIBS])
])
