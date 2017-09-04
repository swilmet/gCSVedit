desktopdir = $(datadir)/applications
dist_desktop_DATA = com.github.swilmet.gcsvedit.desktop

appstream_XML = com.github.swilmet.gcsvedit.appdata.xml
@APPSTREAM_XML_RULES@

dbusservicedir = $(datadir)/dbus-1/services
dbusservice_DATA = com.github.swilmet.gcsvedit.service

com.github.swilmet.gcsvedit.service: Makefile
	$(AM_V_GEN) (echo '[D-BUS Service]'; \
		     echo 'Name=com.github.swilmet.gcsvedit'; \
		     echo 'Exec=${bindir}/gcsvedit --gapplication-service') > $@.tmp && \
		     mv $@.tmp $@

EXTRA_DIST = com.github.swilmet.gcsvedit.appdata.xml

CLEANFILES = $(dbusservice_DATA)

-include $(top_srcdir)/git.mk
