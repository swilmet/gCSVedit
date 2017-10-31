dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.64)

AC_INIT([gCSVedit],
        [0.7.0],
	[https://github.com/swilmet/gCSVedit/issues],
	[gcsvedit],
	[https://github.com/swilmet/gCSVedit])

AX_REQUIRE_DEFINED([AX_IS_RELEASE])
AX_IS_RELEASE([git-directory])

# Dependencies
GLIB_REQUIRED_VERSION=2.44
GTK_REQUIRED_VERSION=3.22
GTKSOURCEVIEW_REQUIRED_VERSION=3.99.6
TEPL_REQUIRED_VERSION=3.99.0

AC_CONFIG_SRCDIR([src/gcsv-main.c])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.14 tar-ustar dist-xz no-dist-gzip gnu -Wno-portability])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Check for programs
AC_PROG_CC
AC_PROG_INSTALL

#Initialize libtool
LT_PREREQ([2.2.6])
LT_INIT([disable-static])

# Pull glib-mkenums & co.
AC_PATH_PROG(GLIB_MKENUMS, glib-mkenums)
AC_PATH_PROG(GLIB_COMPILE_RESOURCES, glib-compile-resources)

# Compile warnings. There can be deprecation warnings at any time, so disable
# -Werror by default with the [yes] argument.
AX_REQUIRE_DEFINED([AX_COMPILER_FLAGS])
AX_COMPILER_FLAGS([WARN_CFLAGS], [WARN_LDFLAGS], [yes])

# pkg-config
AX_REQUIRE_DEFINED([PKG_CHECK_MODULES])
PKG_CHECK_MODULES(DEP, [
	glib-2.0 >= $GLIB_REQUIRED_VERSION
	gio-2.0 >= $GLIB_REQUIRED_VERSION
	gtk+-3.0 >= $GTK_REQUIRED_VERSION
	gtksourceview-4 >= $GTKSOURCEVIEW_REQUIRED_VERSION
	tepl-4 >= $TEPL_REQUIRED_VERSION
])

# i18n
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.4])
AC_SUBST([GETTEXT_PACKAGE], [$PACKAGE])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Define to the gettext package name.])

# Code coverage for unit tests
AX_REQUIRE_DEFINED([AX_CODE_COVERAGE])
AX_CODE_COVERAGE

# Misc
AX_REQUIRE_DEFINED([APPSTREAM_XML])
APPSTREAM_XML

# Output files
AC_CONFIG_FILES([
	Makefile
	data/Makefile
	po/Makefile.in
	src/Makefile
	src/resources/Makefile
	testsuite/Makefile
])

AC_OUTPUT

echo "
Configuration:

	${PACKAGE_NAME} version ${PACKAGE_VERSION}

	Source code location:	${srcdir}
	Prefix:			${prefix}
	Compiler:		${CC}
	Code coverage:		${enable_code_coverage}
"
