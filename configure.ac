dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.64)

AC_INIT([gCSVedit],
        [0.3.0],
	[https://sourceforge.net/p/gcsvedit/tickets/],
	[gcsvedit],
	[http://gcsvedit.sourceforge.net/])

AX_REQUIRE_DEFINED([AX_IS_RELEASE])
AX_IS_RELEASE([git-directory])

# Dependencies
GLIB_REQUIRED_VERSION=2.44
GTK_REQUIRED_VERSION=3.20
GTKSOURCEVIEW_REQUIRED_VERSION=3.23.1
GTEF_REQUIRED_VERSION=1.99.2

AC_CONFIG_SRCDIR([src/gcsv-main.c])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11 tar-ustar dist-xz no-dist-gzip gnu -Wno-portability])

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

# Compile warnings
AX_REQUIRE_DEFINED([AX_COMPILER_FLAGS])
AX_COMPILER_FLAGS([WARN_CFLAGS], [WARN_LDFLAGS])

# pkg-config
AX_REQUIRE_DEFINED([PKG_CHECK_MODULES])
PKG_CHECK_MODULES(DEP, [
	glib-2.0 >= $GLIB_REQUIRED_VERSION
	gio-2.0 >= $GLIB_REQUIRED_VERSION
	gtk+-3.0 >= $GTK_REQUIRED_VERSION
	gtksourceview-3.0 >= $GTKSOURCEVIEW_REQUIRED_VERSION
	gtef-2 >= $GTEF_REQUIRED_VERSION
])

# i18n
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.4])
AC_SUBST([GETTEXT_PACKAGE], [$PACKAGE])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Define to the gettext package name.])

# Code coverage for unit tests
AX_REQUIRE_DEFINED([AX_CODE_COVERAGE])
AX_CODE_COVERAGE()

# Misc
AX_REQUIRE_DEFINED([APPSTREAM_XML])
APPSTREAM_XML

# Check for win32 platform
AC_MSG_CHECKING([for Win32 platform])
case "$host" in
  *-*-mingw*|*-*-cygwin*)
        archive_cmds='$CC -shared $libobjs $deplibs $compiler_flags -o $output_objdir/$soname ${wl}--enable-auto-image-base -Xlinker --output-def -Xlinker $lib.def && $DLLTOOL --dllname $soname --def $lib.def --output-delaylib $lib'
	# If the export-symbols file already is a .def file (1st line
	# is EXPORTS), use it as is; otherwise, prepend...
	archive_expsym_cmds='if test "x`$SED 1q $export_symbols`" = xEXPORTS; then
	  cp $export_symbols $output_objdir/$soname.def;
	else
	  echo EXPORTS > $output_objdir/$soname.def;
	  cat $export_symbols >> $output_objdir/$soname.def;
	fi~
	$CC -shared $output_objdir/$soname.def $libobjs $deplibs $compiler_flags -o $output_objdir/$soname ${wl}--enable-auto-image-base;
	$DLLTOOL --dllname $soname --def $output_objdir/$soname.def --output-delaylib $lib'
    platform_win32=yes
    ;;
  *)
    platform_win32=no
    ;;
esac
AC_MSG_RESULT([$platform_win32])
AM_CONDITIONAL(PLATFORM_WIN32, test "$platform_win32" = "yes")

AC_MSG_CHECKING([for native Win32])
case "$host" in
  *-*-mingw*)
    os_win32=yes
    AC_CHECK_TOOL(WINDRES, windres)
    ACTIVE_PLUGINS="${ACTIVE_PLUGINS}, 'checkupdate'"
    ;;
  *)
    os_win32=no
    ;;
esac
AC_MSG_RESULT([$os_win32])
AM_CONDITIONAL(OS_WIN32, test "$os_win32" = "yes")

# Output files
AC_CONFIG_FILES([
	Makefile
	data/Makefile
	po/Makefile.in
	src/Makefile
	src/resources/Makefile
	testsuite/Makefile
	win32/Makefile
])

AC_OUTPUT

echo "
Configuration:

	Source code location:	${srcdir}
	Compiler:		${CC}
	Code coverage:		${enable_code_coverage}
"
