#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(librttopo, 1.1.0, strk@kbt.io)
AC_LANG(C)
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([foreign])
AM_MAINTAINER_MODE
AM_CONFIG_HEADER(src/rttopo_config.h)

# disabling debug support
AH_TEMPLATE([NDEBUG],
            [Must be defined in order to disable debug mode.])
AC_DEFINE(NDEBUG)
AH_TEMPLATE([LIBRTGEOM_VERSION],
            [RTTOPO library version.])
AC_DEFINE_UNQUOTED([LIBRTGEOM_VERSION], ["$VERSION"], [rtgeom version])
AC_SUBST([LIBRTGEOM_VERSION])
AH_TEMPLATE([RTGEOM_GEOS_VERSION],
            [RTTOPO GEOS version.])

AC_ARG_ENABLE([debug], AC_HELP_STRING([--enable-debug@<:@=LEVEL@:>@],
  [Enable debugging messages up to LEVEL (4 if omitted)]),
  [
if test "$enableval" = "no"; then
  RTGEOM_DEBUG_LEVEL=0;
else
  if test "$enableval" = "yes"; then
    RTGEOM_DEBUG_LEVEL=4;
  else
    RTGEOM_DEBUG_LEVEL=$enableval;
  fi;
fi
  ],
  [RTGEOM_DEBUG_LEVEL=0])
AC_DEFINE_UNQUOTED([RTGEOM_DEBUG_LEVEL], [$RTGEOM_DEBUG_LEVEL], [RTGEOM Debug level])


# Checks for header files.
AC_CHECK_HEADERS(stdlib.h,, [AC_MSG_ERROR([cannot find stdlib.h, bailing out])])
AC_CHECK_HEADERS(stdio.h,, [AC_MSG_ERROR([cannot find stdio.h, bailing out])])
AC_CHECK_HEADERS(string.h,, [AC_MSG_ERROR([cannot find string.h, bailing out])])
AC_CHECK_HEADERS(math.h,, [AC_MSG_ERROR([cannot find math.h, bailing out])])
AC_CHECK_HEADERS(float.h,, [AC_MSG_ERROR([cannot find float.h, bailing out])])
AC_CHECK_HEADERS(inttypes.h,, [AC_MSG_ERROR([cannot find inttypes.h, bailing out])])
AC_CHECK_HEADERS(stdint.h,, [AC_MSG_ERROR([cannot find stdint.h, bailing out])])
AC_CHECK_HEADERS(ctype.h,, [AC_MSG_ERROR([cannot find ctype.h, bailing out])])
AC_CHECK_HEADERS(errno.h,, [AC_MSG_ERROR([cannot find errno.h, bailing out])])
AC_CHECK_HEADERS(assert.h,, [AC_MSG_ERROR([cannot find assert.h, bailing out])])
AC_CHECK_HEADERS(stdarg.h,, [AC_MSG_ERROR([cannot find stdarg.h, bailing out])])


# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_C_VOLATILE

# Checks for library functions.
AC_FUNC_LSTAT
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MEMCMP
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_CHECK_FUNCS([memset sqrt strcasecmp strerror strncasecmp strstr fdatasync ftruncate getcwd gettimeofday localtime_r memmove strerror])

AC_CONFIG_FILES([Makefile \
		src/Makefile \
		headers/Makefile \
		headers/librttopo_geom.h \
		rttopo.pc])

#-----------------------------------------------------------------------
#   --with-geosconfig
#
AC_ARG_WITH([geosconfig],
    [AS_HELP_STRING([--with-geosconfig=FILE], [specify an alternative geos-config file])],
	[GEOSCONFIG="$withval"], [GEOSCONFIG=""])
if test "x$GEOSCONFIG" = "x"; then
    # GEOSCONFIG was not specified, so search within the current path
    AC_PATH_PROG([GEOSCONFIG], [geos-config])
    # If we couldn't find geos-config, display an error
    if test "x$GEOSCONFIG" = "x"; then
            AC_MSG_ERROR([could not find geos-config within the current path. You may need to try re-running configure with a --with-geosconfig parameter.])
    fi
else
    # GEOSCONFIG was specified; display a message to the user
    if test "x$GEOSCONFIG" = "xyes"; then
            AC_MSG_ERROR([you must specify a parameter to --with-geosconfig, e.g. --with-geosconfig=/path/to/geos-config])
    else
            if test -f $GEOSCONFIG; then
                    AC_MSG_RESULT([Using user-specified geos-config file: $GEOSCONFIG])
            else
                    AC_MSG_ERROR([the user-specified geos-config file $GEOSCONFIG does not exist])
            fi
    fi
fi
# Extract the linker and include flags
GEOS_LDFLAGS=`$GEOSCONFIG --ldflags`
GEOS_CFLAGS=-I`$GEOSCONFIG --includes`
AC_SUBST([GEOS_LDFLAGS])
AC_SUBST([GEOS_CFLAGS])
# Ensure that we can parse geos_c.h
CPPFLAGS_SAVE="$CPPFLAGS"
CPPFLAGS="$GEOS_CFLAGS"
AC_CHECK_HEADERS([geos_c.h],, [AC_MSG_ERROR([could not find geos_c.h - you may need to specify the directory of a geos-config file using --with-geosconfig])])
CPPFLAGS="$CPPFLAGS_SAVE"
# Ensure we can link against libgeos_c
LIBS_SAVE="$LIBS"
LIBS="$GEOS_LDFLAGS"
AC_SEARCH_LIBS(GEOSContext_setErrorMessageHandler_r,geos_c,,AC_MSG_ERROR([could not find libgeos_c (or obsolete 'libgeos_c' < v.3.5.0 found) - you may need to specify the directory of a geos-config file using --with-geosconfig]))
LIBS="$LIBS_SAVE"
LIBS="$LIBS $GEOS_LDFLAGS -lgeos_c"
GEOS_MAJOR_VERSION=`$GEOSCONFIG --version | cut -d. -f1 | sed 's/[[^0-9]]//g'`
GEOS_MINOR_VERSION=`$GEOSCONFIG --version | cut -d. -f2 | sed 's/[[^0-9]]//g'`
RTGEOM_GEOS_VERSION="$GEOS_MAJOR_VERSION$GEOS_MINOR_VERSION"
AC_DEFINE_UNQUOTED([RTGEOM_GEOS_VERSION], [$RTGEOM_GEOS_VERSION], [GEOS library version])
AC_SUBST([RTGEOM_GEOS_VERSION])

# SRID stuff
SRID_MAX=999999
SRID_USR_MAX=998999
AC_SUBST([SRID_MAX])
AC_SUBST([SRID_USR_MAX])

AC_OUTPUT

