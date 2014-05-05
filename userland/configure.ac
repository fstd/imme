#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([immectl], [0.0.0], [van.fisted@googlemail.com])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/immectl.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

# Checks for programs.
AC_PROG_CC_C99
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_MAKE_SET

AC_CHECK_LIB([srsbsns], [splitquoted], [], [AC_MSG_ERROR([libsrsbsns not installed?])])

AC_CANONICAL_HOST
case $host_os in
	linux*)
AC_DEFINE([PLATFORM_LUNIX],[1],[defined if we are on the losers unix])
	;;
	*bsd*)
AC_DEFINE([PLATFORM_BSD],[1],[defined if we are on a BSD])
	;;
	*)
AC_DEFINE([PLATFORM_UNKNOWN],[1],[defined if we cant tell])
	;;
esac

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT

echo \
"-------------------------------------------------

 ${PACKAGE_NAME} Version ${PACKAGE_VERSION}

 Prefix: '${prefix}'.
 Compiler: '${CC} ${CFLAGS}'
 Libraries: '${LIBS}'

--------------------------------------------------"
