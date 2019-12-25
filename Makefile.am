SUBDIRS = data po src testsuite

include $(top_srcdir)/aminclude_static.am
clean-local: code-coverage-clean
distclean-local: code-coverage-dist-clean

AM_DISTCHECK_CONFIGURE_FLAGS = --enable-appstream-util

EXTRA_DIST =		\
	HACKING		\
	README.md

MAINTAINERCLEANFILES = \
	$(GITIGNORE_MAINTAINERCLEANFILES_TOPLEVEL) \
	$(GITIGNORE_MAINTAINERCLEANFILES_MAKEFILE_IN) \
	$(GITIGNORE_MAINTAINERCLEANFILES_M4_LIBTOOL) \
	aminclude_static.am \
	INSTALL

-include $(top_srcdir)/git.mk
