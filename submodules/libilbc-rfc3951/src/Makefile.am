extracted_sources = \
anaFilter.c  FrameClassify.c  iCBConstruct.c  LPCencode.c \
anaFilter.h  FrameClassify.h  iCBConstruct.h  LPCencode.h \
constants.c  gainquant.c      iCBSearch.c     lsf.c \
constants.h  gainquant.h      iCBSearch.h     lsf.h \
createCB.c   getCBvec.c       iLBC_decode.c   packing.c \
createCB.h   getCBvec.h       iLBC_decode.h   packing.h \
doCPLC.c     helpfun.c        iLBC_define.h   StateConstructW.c \
doCPLC.h     helpfun.h        iLBC_encode.c   StateConstructW.h \
enhancer.c   hpInput.c        iLBC_encode.h   StateSearchW.c \
enhancer.h   hpInput.h        iLBC_test.c     StateSearchW.h \
filter.c     hpOutput.c       LPCdecode.c     syntFilter.c \
filter.h     hpOutput.h       LPCdecode.h     syntFilter.h

checkmarks = $(extracted_sources:%=%.check)

BUILT_SOURCES = $(extracted_sources) $(checkmarks)

checksum_dir = $(srcdir)/known_checksums
checksum_files = $(addprefix $(checksum_dir)/, $(extracted_sources:%=%.md5))
EXTRA_DIST = $(checksum_files)

$(checkmarks): %.check: %
	cd $(<D); $(MD5SUM) -c $(checksum_dir)/$(<F).md5
	touch $@

lib_LTLIBRARIES = libilbc.la
nodist_libilbc_la_SOURCES = $(extracted_sources)
nodist_ilbc_include_HEADERS = iLBC_decode.h iLBC_encode.h iLBC_define.h
ilbc_includedir = $(includedir)/ilbc
#libilbc_la_LDFLAGS = -lm


libilbc_la_LDFLAGS= -no-undefined

DISTCLEANFILES = $(BUILT_SOURCES)