AM_CPPFLAGS =			\
	-I$(top_srcdir)		\
	$(WARN_CFLAGS)		\
	$(DEP_CFLAGS)

AM_LDFLAGS = $(WARN_LDFLAGS)

# Internal library, so that unit tests can link to it.
noinst_LTLIBRARIES = libgcsvedit.la

libgcsvedit_la_SOURCES =		\
	gcsv-alignment.c		\
	gcsv-alignment.h		\
	gcsv-utils.c			\
	gcsv-utils.h			\
	gcsv-window.c			\
	gcsv-window.h

libgcsvedit_la_LIBADD = $(DEP_LIBS)

# The application
bin_PROGRAMS = gcsvedit

gcsvedit_SOURCES = gcsv-main.c

gcsvedit_LDADD =	\
	libgcsvedit.la	\
	$(DEP_LIBS)

-include $(top_srcdir)/git.mk
