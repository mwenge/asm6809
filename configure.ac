#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([asm6809], [2.0pre5])
AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR([src/asm6809.c])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_CC_C99
gl_EARLY
AC_PROG_YACC
AC_PROG_LEX
AC_PROG_RANLIB

# Checks for libraries.

# Checks for header files.
gl_INIT
AC_FUNC_ALLOCA
AC_HEADER_STDC
AC_CHECK_HEADERS([inttypes.h libintl.h malloc.h stddef.h stdint.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([memset strerror strtol])

AC_CONFIG_FILES([README Makefile gnulib/Makefile src/Makefile])
AC_OUTPUT
