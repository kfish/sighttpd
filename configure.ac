# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(sighttpd, 0.9.0, [conrad@metadecks.org])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_RANLIB

dnl Check for pkgconfig first
AC_CHECK_PROG(HAVE_PKGCONFIG, pkg-config, yes, no)

dnl Give error and exit if we don't have pkgconfig
if test "x$HAVE_PKGCONFIG" = "xno"; then
  AC_MSG_ERROR(you need to have pkgconfig installed !)
fi

# Checks for libraries.

dnl
dnl  Detect pthreads libraries
dnl

PTHREAD_LIBS=error
AC_CHECK_LIB(pthread, pthread_attr_init, PTHREAD_LIBS="-lpthread")
if test "x$PTHREAD_LIBS" = xerror; then
        AC_CHECK_LIB(pthreads, pthread_attr_init, PTHREAD_LIBS="-lpthreads")
fi
if test "x$PTHREAD_LIBS" = xerror; then
        AC_CHECK_LIB(c_r, pthread_attr_init, PTHREAD_LIBS="-lc_r")
fi
if test "x$PTHREAD_LIBS" = xerror; then
        AC_CHECK_FUNC(pthread_attr_init, PTHREAD_LIBS="")
fi
AC_SUBST(PTHREAD_LIBS)

dnl
dnl Detect librt
dnl

RT_LIBS=error
AC_CHECK_LIB(rt, clock_gettime, RT_LIBS="-lrt")
AC_SUBST(RT_LIBS)

dnl
dnl Check for libuiomux
dnl
PKG_CHECK_MODULES(UIOMUX, uiomux, HAVE_UIOMUX="yes", HAVE_UIOMUX="no")
if test "x$HAVE_UIOMUX" = "xyes" ; then
  AC_DEFINE(HAVE_UIOMUX, [], [Define if have libuiomux])
  AC_SUBST(UIOMUX_CFLAGS)
  AC_SUBST(UIOMUX_LIBS)
fi
AM_CONDITIONAL(HAVE_UIOMUX, [test "x$HAVE_UIOMUX" = "xyes"])

dnl
dnl Check for libshveu
dnl
PKG_CHECK_MODULES(SHVEU, shveu >= 0.5.0, HAVE_SHVEU="yes", HAVE_SHVEU="no")
if test "x$HAVE_SHVEU" = "xyes" ; then
  AC_DEFINE(HAVE_SHVEU, [], [Define if have libshveu])
  AC_SUBST(SHVEU_CFLAGS)
  AC_SUBST(SHVEU_LIBS)
fi
AM_CONDITIONAL(HAVE_SHVEU, [test "x$HAVE_SHVEU" = "xyes"])

dnl
dnl Check for libshcodecs
dnl
PKG_CHECK_MODULES(SHCODECS, shcodecs >= 0.9.9, HAVE_SHCODECS="yes", HAVE_SHCODECS="no")
if test "x$HAVE_SHCODECS" = "xyes" ; then
  AC_DEFINE(HAVE_SHCODECS, [], [Define if have libshcodecs])
  AC_SUBST(SHCODECS_CFLAGS)
  AC_SUBST(SHCODECS_LIBS)
fi
AM_CONDITIONAL(HAVE_SHCODECS, [test "x$HAVE_SHCODECS" = "xyes"])

# Checks for header files.
AC_HEADER_RESOLV
AC_HEADER_STDC
AC_CHECK_HEADERS([netdb.h netinet/in.h stdint.h stdlib.h string.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_STRTOD
AC_CHECK_FUNCS([bzero clock_gettime memset socket strcasecmp strcspn strdup strspn])

AC_CONFIG_FILES([
Makefile
src/Makefile
src/shrec/Makefile
doc/Makefile
examples/Makefile
])

AC_OUTPUT

AC_MSG_RESULT([
------------------------------------------------------------------------
  $PACKAGE $VERSION:  Automatic configuration OK.

  Module configuration:

    UIOMux support: .............. $HAVE_UIOMUX
    SHVEU support: ............... $HAVE_SHVEU
    SHCodecs support: ............ $HAVE_SHCODECS

------------------------------------------------------------------------
])

