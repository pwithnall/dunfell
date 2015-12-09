AC_PREREQ(2.65)

# Release version
m4_define([dfl_version_major],[0])
m4_define([dfl_version_minor],[3])
m4_define([dfl_version_micro],[0])

# API version
m4_define([dfl_api_version],[0])

AC_INIT([dunfell],[dfl_version_major.dfl_version_minor.dfl_version_micro],
        [mailto:bugzilla@tecnocode.co.uk],[dunfell],
        [https://gitlab.com/pwithnall/dunfell])

AX_CHECK_ENABLE_DEBUG
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SRCDIR([libdunfell/dunfell.h])
AC_CONFIG_HEADERS([config.h])
AC_USE_SYSTEM_EXTENSIONS

AM_INIT_AUTOMAKE([1.9 dist-xz no-dist-gzip check-news subdir-objects])

# Enable silent build when available (Automake 1.11)
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_PROG_CXX
AM_PROG_CC_C_O
LT_INIT
PKG_PROG_PKG_CONFIG
AC_PROG_LN_S

# Requirements
GLIB_REQS=2.31.0
GTK_REQS=3.0.0

# Before making a release, the DFL_LT_VERSION string should be modified. The
# string is of the form c:r:a. Follow these instructions sequentially:
#
#  1. If the library source code has changed at all since the last update, then
#     increment revision (‘c:r:a’ becomes ‘c:r+1:a’).
#  2. If any interfaces have been added, removed, or changed since the last
#     update, increment current, and set revision to 0.
#  3. If any interfaces have been added since the last public release, then
#     increment age.
#  4. If any interfaces have been removed or changed since the last public
#     release, then set age to 0.
AC_SUBST([DFL_LT_VERSION],[2:0:0])

AC_SUBST([DFL_VERSION_MAJOR],DFL_version_major)
AC_SUBST([DFL_VERSION_MINOR],DFL_version_minor)
AC_SUBST([DFL_VERSION_MICRO],DFL_version_micro)
AC_SUBST([DFL_API_VERSION],DFL_api_version)

# Dependencies
AX_PKG_CHECK_MODULES([GLIB],[glib-2.0 >= $GLIB_REQS gio-2.0 gobject-2.0],[])
AX_PKG_CHECK_MODULES([GTK],[gtk+-3.0 >= $GTK_REQS],[])

# Code coverage
AX_CODE_COVERAGE

# General macros
AX_COMPILER_FLAGS
AX_VALGRIND_CHECK

GOBJECT_INTROSPECTION_CHECK([0.9.7])

VAPIGEN_CHECK()

AC_PATH_PROG([GLIB_COMPILE_RESOURCES],[glib-compile-resources])
AS_IF([test "$GLIB_COMPILE_RESOURCES" == ""],[
	AC_MSG_ERROR([glib-compile-resources not found])
])

AC_SUBST([AM_CPPFLAGS])
AC_SUBST([AM_CFLAGS])
AC_SUBST([AM_CXXFLAGS])
AC_SUBST([AM_LDFLAGS])

GTK_DOC_CHECK([1.14],[--flavour no-tmpl])

AC_CONFIG_FILES([
Makefile
libdunfell/libdunfell-$DFL_API_VERSION.pc:libdunfell/libdunfell.pc.in
libdunfell/dfl-version.h
libdunfell/tests/Makefile
libdunfell/docs/Makefile
],[],
[DFL_API_VERSION='$DFL_API_VERSION'])
AC_OUTPUT
