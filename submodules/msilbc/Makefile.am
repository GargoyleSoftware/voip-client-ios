ACLOCAL_AMFLAGS = -I m4

plugindir = $(libdir)/mediastreamer/plugins

plugin_LTLIBRARIES = libmsilbc.la

libmsilbc_la_SOURCES = ilbc.c

libmsilbc_la_CFLAGS = $(ORTP_CFLAGS) $(MEDIASTREAMER_CFLAGS) $(ILBC_CFLAGS) \
	$(MSILBC_CFLAGS)

libmsilbc_la_LIBADD = $(ORTP_LIBS) $(MEDIASTREAMER_LIBS) $(ILBC_LIBS)

libmsilbc_la_LDFLAGS = -module
