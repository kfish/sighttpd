# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(camserv, 0.1.0, [conrad@metadecks.org])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC

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
])

AC_OUTPUT

AC_MSG_RESULT([
------------------------------------------------------------------------
  $PACKAGE $VERSION:  Automatic configuration OK.
------------------------------------------------------------------------
])

