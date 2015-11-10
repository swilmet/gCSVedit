SUBDIRS = resources

AM_CPPFLAGS =				\
	-I$(top_srcdir)			\
	-DDATADIR=\""$(datadir)"\"	\
	$(WARN_CFLAGS)			\
	$(DEP_CFLAGS)

AM_LDFLAGS = $(WARN_LDFLAGS)

BUILT_SOURCES =			\
	gcsv-resources.c

# Internal library, so that unit tests can link to it.
noinst_LTLIBRARIES = libgcsvedit.la

libgcsvedit_la_SOURCES =		\
	gcsv-alignment.c		\
	gcsv-alignment.h		\
	gcsv-buffer.c			\
	gcsv-buffer.h			\
	gcsv-properties-chooser.c	\
	gcsv-properties-chooser.h	\
	gcsv-utils.c			\
	gcsv-utils.h			\
	gcsv-window.c			\
	gcsv-window.h			\
	gtktextregion.c			\
	gtktextregion.h

libgcsvedit_la_LIBADD = $(DEP_LIBS)

# The application
bin_PROGRAMS = gcsvedit

gcsvedit_SOURCES = gcsv-main.c

# do not distribute generated files
nodist_gcsvedit_SOURCES = $(BUILT_SOURCES)

gcsvedit_LDADD =	\
	libgcsvedit.la	\
	$(DEP_LIBS)

xml_res = $(srcdir)/resources/gcsvedit.gresource.xml
gcsv-resources.c: $(xml_res) $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/resources --generate-dependencies $(xml_res))
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/resources --generate-source $(xml_res)

CLEANFILES = $(BUILT_SOURCES)

-include $(top_srcdir)/git.mk
