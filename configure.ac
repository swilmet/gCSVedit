dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.64)

AC_INIT([gCSVedit],
        [0.0.1],
	[http://catl.ions.ucl.ac.be/index.html],
	[gcsvedit],
	[http://catl.ions.ucl.ac.be/git/?p=gcsvedit.git;a=summary])

AX_IS_RELEASE([git-directory])

# Dependencies
GLIB_REQUIRED_VERSION=2.44
GTK_REQUIRED_VERSION=3.16
GTKSOURCEVIEW_REQUIRED_VERSION=3.16

AC_CONFIG_SRCDIR([src/gcsv-main.c])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11 tar-ustar dist-xz no-dist-gzip foreign])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Check for programs
AC_PROG_CC
AC_PROG_INSTALL

# Pull glib-mkenums & co.
AC_PATH_PROG(GLIB_MKENUMS, glib-mkenums)
AC_PATH_PROG(GLIB_COMPILE_RESOURCES, glib-compile-resources)

# Compile warnings
AX_COMPILER_FLAGS([WARN_CFLAGS], [WARN_LDFLAGS])

PKG_CHECK_MODULES(DEP, [
	glib-2.0 >= $GLIB_REQUIRED_VERSION
	gio-2.0 >= $GLIB_REQUIRED_VERSION
	gtk+-3.0 >= $GTK_REQUIRED_VERSION
	gtksourceview-3.0 >= $GTKSOURCEVIEW_REQUIRED_VERSION
])

# Output files
AC_CONFIG_FILES([
	Makefile
	src/Makefile
])

AC_OUTPUT
