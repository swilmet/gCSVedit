bin_PROGRAMS = gcsvedit

gcsvedit_SOURCES =			\
	gcsv-main.c			\
	gcsv-window.c			\
	gcsv-window.h

gcsvedit_CPPFLAGS =		\
	-I$(top_srcdir)		\
	$(WARN_CFLAGS)		\
	$(DEP_CFLAGS)

gcsvedit_LDFLAGS = $(WARN_LDFLAGS)

gcsvedit_LDADD = $(DEP_LIBS)

-include $(top_srcdir)/git.mk
