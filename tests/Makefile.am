@CODE_COVERAGE_RULES@

AM_CPPFLAGS = 			\
	-I$(top_srcdir)		\
	-I$(top_srcdir)/src	\
	$(WARN_CFLAGS) 		\
	$(DEP_CFLAGS)

AM_LDFLAGS = $(WARN_LDFLAGS)

LDADD =	$(top_builddir)/src/libgcsvedit.la \
	$(DEP_LIBS)

noinst_PROGRAMS = $(UNIT_TEST_PROGS)
TESTS = $(UNIT_TEST_PROGS)

UNIT_TEST_PROGS = test-utils
test_utils_SOURCES = test-utils.c

UNIT_TEST_PROGS += test-alignment
test_alignment_SOURCES = test-alignment.c

-include $(top_srcdir)/git.mk
