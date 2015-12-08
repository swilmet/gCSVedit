desktopdir = $(datadir)/applications
dist_desktop_DATA = be.uclouvain.gcsvedit.desktop

appdatadir = $(datadir)/appdata
dist_appdata_DATA = be.uclouvain.gcsvedit.appdata.xml

dbusservicedir = $(datadir)/dbus-1/services
dbusservice_DATA = be.uclouvain.gcsvedit.service

be.uclouvain.gcsvedit.service: Makefile
	$(AM_V_GEN) (echo '[D-BUS Service]'; \
		     echo 'Name=be.uclouvain.gcsvedit'; \
		     echo 'Exec=${bindir}/gcsvedit --gapplication-service') > $@.tmp && \
		     mv $@.tmp $@

DISTCLEANFILES =		\
	$(dbusservice_DATA)

-include $(top_srcdir)/git.mk
