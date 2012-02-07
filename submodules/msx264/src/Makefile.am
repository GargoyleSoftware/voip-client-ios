pluginsdir=$(PACKAGE_PLUGINS_DIR)

plugins_LTLIBRARIES=libmsx264.la

libmsx264_la_SOURCES=msx264.c

libmsx264_la_LIBADD=\
	$(MEDIASTREAMER_LIBS) \
	$(X264_LIBS)

libmsx264_la_LDFLAGS=-module -no-undefined

AM_CFLAGS=	\
		$(MEDIASTREAMER_CFLAGS)\
		$(X264_CFLAGS)
