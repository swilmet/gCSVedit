include $(top_srcdir)/aminclude_static.am
clean-local: code-coverage-clean
distclean-local: code-coverage-dist-clean

AM_CPPFLAGS =				\
	-I$(top_srcdir)			\
	-DGCSV_DATADIR=\""$(datadir)"\"	\
	$(DEP_CFLAGS)			\
	$(WARN_CFLAGS)			\
	$(CODE_COVERAGE_CPPFLAGS)

AM_LDFLAGS = $(WARN_LDFLAGS)

# Internal library, so that unit tests can link to it.
noinst_LTLIBRARIES = libgcsvedit.la

libgcsvedit_la_SOURCES =		\
	gcsv-alignment.c		\
	gcsv-alignment.h		\
	gcsv-application.c		\
	gcsv-application.h		\
	gcsv-buffer.c			\
	gcsv-buffer.h			\
	gcsv-factory.c			\
	gcsv-factory.h			\
	gcsv-properties-chooser.c	\
	gcsv-properties-chooser.h	\
	gcsv-tab.c			\
	gcsv-tab.h			\
	gcsv-utils.c			\
	gcsv-utils.h			\
	gcsv-window.c			\
	gcsv-window.h

libgcsvedit_la_CFLAGS = $(CODE_COVERAGE_CFLAGS)
libgcsvedit_la_LIBADD =		\
	$(DEP_LIBS)		\
	$(CODE_COVERAGE_LIBS)

# The application
bin_PROGRAMS = gcsvedit

gcsvedit_SOURCES = gcsv-main.c

gcsvedit_LDADD =	\
	libgcsvedit.la	\
	$(DEP_LIBS)

if OS_WIN32
# Do not open a console, open directly the application window.
AM_LDFLAGS += -mwindows
endif

-include $(top_srcdir)/git.mk
