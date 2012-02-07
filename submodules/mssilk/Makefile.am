SILK_VERSION = 1.0.8
#SILK_DIR=$(srcdir)/sdk/SILK_SDK_SRC_v$(SILK_VERSION)/SILK_SDK_SRC_$(SILK_FLAVOUR)_v$(SILK_VERSION)
SILK_DIR=$(CURDIR)/sdk/SILK_SDK_SRC_v$(SILK_VERSION)/SILK_SDK_SRC_$(SILK_FLAVOUR)_v$(SILK_VERSION)

SUBDIRS = sdk


pluginsdir = $(libdir)/mediastreamer/plugins

plugins_LTLIBRARIES=libmssilk.la

libmssilk_la_SOURCES=silk_dec.c silk_enc.c


SILK_LIBS = -L${SILK_DIR} -lSKP_SILK_SDK
libmssilk_la_LIBADD=\
	$(ORTP_LIBS) \
        $(MEDIASTREAMER_LIBS) \
        $(SILK_LIBS)

libmssilk_la_LDFLAGS=-module -no-undefined

SILK_CFLAGS = -I$(SILK_DIR)/interface 
AM_CFLAGS=      $(SILK_CFLAGS) \
                $(MEDIASTREAMER_CFLAGS)


