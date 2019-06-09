#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(webspider, 0.1, ltzmage@163.com)
AC_CONFIG_SRCDIR([src/main.cpp])
AM_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE(webspider, 0.1)

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Get adns include and library dir
AC_ARG_WITH([adns],
  [AS_HELP_STRING([--with-adns],
    [location of the ADNS headers and libraries])],
  [CPPFLAGS="$CPPFLAGS -I$withval/include"
    LDFLAGS="$LDFLAGS -L$withval/lib -ladns"])

# Checks for libraries.
AC_CHECK_LIB(adns, [adns_init, adns_submit, adns_check, adns_finish])
#AC_CHECK_LIB(adns, adns_submit)
#AC_CHECK_LIB(adns, adns_check)
#AC_CHECK_LIB(adns, adns_finish)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(adns.h)

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST

# Checks for library functions.
AC_OUTPUT([Makefile])
