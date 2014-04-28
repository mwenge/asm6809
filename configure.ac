#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([asm6809], [2.0pre5])
AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR([src/asm6809.c])
AC_CONFIG_HEADERS([config.h])

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
AC_CHECK_HEADERS([OS.h fcntl.h float.h inttypes.h langinfo.h libintl.h limits.h locale.h malloc.h netdb.h stddef.h stdint.h stdlib.h string.h sys/param.h unistd.h wchar.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_FUNC_OBSTACK
AC_TYPE_OFF_T
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_MBRTOWC
AC_FUNC_REALLOC
AC_FUNC_STRERROR_R
AC_FUNC_STRNLEN
AC_FUNC_STRTOD
AC_CHECK_FUNCS([getpagesize localeconv mblen mbrlen memchr memset nl_langinfo strchr strerror strndup strtol])

AC_CONFIG_FILES([README Makefile gnulib/Makefile src/Makefile tests/Makefile])
AC_OUTPUT
