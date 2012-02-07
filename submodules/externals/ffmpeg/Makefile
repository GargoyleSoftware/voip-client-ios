include config.mak

SRC_DIR = $(SRC_PATH_BARE)

vpath %.texi $(SRC_PATH_BARE)

PROGS-$(CONFIG_FFMPEG)   += ffmpeg
PROGS-$(CONFIG_FFPLAY)   += ffplay
PROGS-$(CONFIG_FFPROBE)  += ffprobe
PROGS-$(CONFIG_FFSERVER) += ffserver

PROGS      := $(PROGS-yes:%=%$(EXESUF))
PROGS_G     = $(PROGS-yes:%=%_g$(EXESUF))
OBJS        = $(PROGS-yes:%=%.o) cmdutils.o
MANPAGES    = $(PROGS-yes:%=doc/%.1)
PODPAGES    = $(PROGS-yes:%=doc/%.pod)
HTMLPAGES   = $(PROGS-yes:%=doc/%.html)
TOOLS       = $(addprefix tools/, $(addsuffix $(EXESUF), cws2fws graph2dot lavfi-showfiltfmts pktdumper probetest qt-faststart trasher))
TESTTOOLS   = audiogen videogen rotozoom tiny_psnr base64
HOSTPROGS  := $(TESTTOOLS:%=tests/%)

BASENAMES   = ffmpeg ffplay ffprobe ffserver
ALLPROGS    = $(BASENAMES:%=%$(EXESUF))
ALLPROGS_G  = $(BASENAMES:%=%_g$(EXESUF))
ALLMANPAGES = $(BASENAMES:%=%.1)

FFLIBS-$(CONFIG_AVDEVICE) += avdevice
FFLIBS-$(CONFIG_AVFILTER) += avfilter
FFLIBS-$(CONFIG_AVFORMAT) += avformat
FFLIBS-$(CONFIG_AVCODEC)  += avcodec
FFLIBS-$(CONFIG_POSTPROC) += postproc
FFLIBS-$(CONFIG_SWSCALE)  += swscale

FFLIBS := avutil

DATA_FILES := $(wildcard $(SRC_DIR)/ffpresets/*.ffpreset)

SKIPHEADERS = cmdutils_common_opts.h

include common.mak

FF_LDFLAGS   := $(FFLDFLAGS)
FF_EXTRALIBS := $(FFEXTRALIBS)
FF_DEP_LIBS  := $(DEP_LIBS)

ALL_TARGETS-$(CONFIG_DOC)       += documentation

ifdef PROGS
INSTALL_TARGETS-yes             += install-progs install-data
INSTALL_TARGETS-$(CONFIG_DOC)   += install-man
endif
INSTALL_PROGS_TARGETS-$(CONFIG_SHARED) = install-libs

all: $(FF_DEP_LIBS) $(PROGS) $(ALL_TARGETS-yes)

$(PROGS): %$(EXESUF): %_g$(EXESUF)
	$(CP) $< $@
	$(STRIP) $@

config.h: .config
.config: $(wildcard $(FFLIBS:%=$(SRC_DIR)/lib%/all*.c))
	@-tput bold 2>/dev/null
	@-printf '\nWARNING: $(?F) newer than config.h, rerun configure\n\n'
	@-tput sgr0 2>/dev/null

SUBDIR_VARS := OBJS FFLIBS CLEANFILES DIRS TESTPROGS EXAMPLES SKIPHEADERS \
               ALTIVEC-OBJS MMX-OBJS NEON-OBJS X86-OBJS YASM-OBJS-FFT YASM-OBJS \
               HOSTPROGS BUILT_HEADERS TESTOBJS ARCH_HEADERS ARMV6-OBJS

define RESET
$(1) :=
$(1)-yes :=
endef

define DOSUBDIR
$(foreach V,$(SUBDIR_VARS),$(eval $(call RESET,$(V))))
SUBDIR := $(1)/
include $(1)/Makefile
endef

$(foreach D,$(FFLIBS),$(eval $(call DOSUBDIR,lib$(D))))

ffplay_g$(EXESUF): FF_EXTRALIBS += $(SDL_LIBS)
ffserver_g$(EXESUF): FF_LDFLAGS += $(FFSERVERLDFLAGS)

%_g$(EXESUF): %.o cmdutils.o $(FF_DEP_LIBS)
	$(LD) $(FF_LDFLAGS) -o $@ $< cmdutils.o $(FF_EXTRALIBS)

tools/%$(EXESUF): tools/%.o
	$(LD) $(FF_LDFLAGS) -o $@ $< $(FF_EXTRALIBS)

tools/%.o: tools/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $(CC_O) $<

-include $(wildcard tools/*.d)

ffplay.o: CFLAGS += $(SDL_CFLAGS)

VERSION_SH  = $(SRC_PATH_BARE)/version.sh
GIT_LOG     = $(SRC_PATH_BARE)/.git/logs/HEAD

.version: $(wildcard $(GIT_LOG)) $(VERSION_SH) config.mak
.version: M=@

version.h .version:
	$(M)$(VERSION_SH) $(SRC_PATH) version.h $(EXTRA_VERSION)
	$(Q)touch .version

# force version.sh to run whenever version might have changed
-include .version

alltools: $(TOOLS)

DOCS = $(addprefix doc/, developer.html faq.html general.html libavfilter.html) $(HTMLPAGES) $(MANPAGES) $(PODPAGES)

documentation: $(DOCS)

-include $(wildcard $(DOCS:%=%.d))

TEXIDEP = awk '/^@include/ { printf "$@: $(@D)/%s\n", $$2 }' <$< >$(@:%=%.d)

doc/%.html: TAG = HTML
doc/%.html: doc/%.texi $(SRC_PATH_BARE)/doc/t2h.init
	$(Q)$(TEXIDEP)
	$(M)cd doc && texi2html -monolithic --init-file $(SRC_PATH_BARE)/doc/t2h.init $(<:doc/%=%)

doc/%.pod: TAG = POD
doc/%.pod: doc/%.texi
	$(Q)$(TEXIDEP)
	$(M)doc/texi2pod.pl $< $@

doc/%.1: TAG = MAN
doc/%.1: doc/%.pod
	$(M)pod2man --section=1 --center=" " --release=" " $< > $@

install: $(INSTALL_TARGETS-yes)

install-progs: $(PROGS) $(INSTALL_PROGS_TARGETS-yes)
	$(Q)mkdir -p "$(BINDIR)"
	$(INSTALL) -c -m 755 $(PROGS) "$(BINDIR)"

install-data: $(DATA_FILES)
	$(Q)mkdir -p "$(DATADIR)"
	$(INSTALL) -m 644 $(DATA_FILES) "$(DATADIR)"

install-man: $(MANPAGES)
	$(Q)mkdir -p "$(MANDIR)/man1"
	$(INSTALL) -m 644 $(MANPAGES) "$(MANDIR)/man1"

uninstall: uninstall-progs uninstall-data uninstall-man

uninstall-progs:
	$(RM) $(addprefix "$(BINDIR)/", $(ALLPROGS))

uninstall-data:
	$(RM) -r "$(DATADIR)"

uninstall-man:
	$(RM) $(addprefix "$(MANDIR)/man1/",$(ALLMANPAGES))

testclean:
	$(RM) -r tests/vsynth1 tests/vsynth2 tests/data
	$(RM) $(addprefix tests/,$(CLEANSUFFIXES))
	$(RM) tests/seek_test$(EXESUF) tests/seek_test.o
	$(RM) $(TESTTOOLS:%=tests/%$(HOSTEXESUF))

clean:: testclean
	$(RM) $(ALLPROGS) $(ALLPROGS_G)
	$(RM) $(CLEANSUFFIXES)
	$(RM) doc/*.html doc/*.pod doc/*.1
	$(RM) $(TOOLS)
	$(RM) $(CLEANSUFFIXES:%=tools/%)

distclean::
	$(RM) $(DISTCLEANSUFFIXES)
	$(RM) version.h config.* libavutil/avconfig.h

config:
	$(SRC_PATH)/configure $(value FFMPEG_CONFIGURATION)

# regression tests

check: test checkheaders

fulltest test: codectest lavftest lavfitest seektest

FFSERVER_REFFILE = $(SRC_PATH)/tests/ffserver.regression.ref
SEEK_REFFILE     = $(SRC_PATH)/tests/seek.regression.ref

codectest: fate-codec
lavftest:  fate-lavf
lavfitest: fate-lavfi
seektest:  fate-seek

AREF = tests/data/acodec.ref.wav
VREF = tests/data/vsynth1.ref.yuv
REFS = $(AREF) $(VREF)

$(REFS): TAG = GEN

$(VREF): ffmpeg$(EXESUF) tests/vsynth1/00.pgm tests/vsynth2/00.pgm
	$(M)$(SRC_PATH)/tests/codec-regression.sh vref vsynth1 tests/vsynth1 "$(TARGET_EXEC)" "$(TARGET_PATH)"
	$(Q)$(SRC_PATH)/tests/codec-regression.sh vref vsynth2 tests/vsynth2 "$(TARGET_EXEC)" "$(TARGET_PATH)"

$(AREF): ffmpeg$(EXESUF) tests/data/asynth1.sw
	$(M)$(SRC_PATH)/tests/codec-regression.sh aref acodec tests/acodec "$(TARGET_EXEC)" "$(TARGET_PATH)"

ffservertest: ffserver$(EXESUF) tests/vsynth1/00.pgm tests/data/asynth1.sw
	@echo
	@echo "Unfortunately ffserver is broken and therefore its regression"
	@echo "test fails randomly. Treat the results accordingly."
	@echo
	$(SRC_PATH)/tests/ffserver-regression.sh $(FFSERVER_REFFILE) $(SRC_PATH)/tests/ffserver.conf

tests/vsynth1/00.pgm: tests/videogen$(HOSTEXESUF)
	@mkdir -p tests/vsynth1
	$(M)$(BUILD_ROOT)/$< 'tests/vsynth1/'

tests/vsynth2/00.pgm: tests/rotozoom$(HOSTEXESUF)
	@mkdir -p tests/vsynth2
	$(M)$(BUILD_ROOT)/$< 'tests/vsynth2/' $(SRC_PATH)/tests/lena.pnm

tests/data/asynth1.sw: tests/audiogen$(HOSTEXESUF)
	@mkdir -p tests/data
	$(M)$(BUILD_ROOT)/$< $@

tests/data/asynth1.sw tests/vsynth%/00.pgm: TAG = GEN

tests/seek_test$(EXESUF): tests/seek_test.o $(FF_DEP_LIBS)
	$(LD) $(FF_LDFLAGS) -o $@ $< $(FF_EXTRALIBS)

tools/lavfi-showfiltfmts$(EXESUF): tools/lavfi-showfiltfmts.o $(FF_DEP_LIBS)
	$(LD) $(FF_LDFLAGS) -o $@ $< $(FF_EXTRALIBS)

include $(SRC_PATH_BARE)/tests/fate.mak
include $(SRC_PATH_BARE)/tests/fate2.mak

include $(SRC_PATH_BARE)/tests/fate/aac.mak
include $(SRC_PATH_BARE)/tests/fate/als.mak
include $(SRC_PATH_BARE)/tests/fate/fft.mak
include $(SRC_PATH_BARE)/tests/fate/h264.mak
include $(SRC_PATH_BARE)/tests/fate/mp3.mak
include $(SRC_PATH_BARE)/tests/fate/vorbis.mak
include $(SRC_PATH_BARE)/tests/fate/vp8.mak

FATE_ACODEC  = $(ACODEC_TESTS:%=fate-acodec-%)
FATE_VSYNTH1 = $(VCODEC_TESTS:%=fate-vsynth1-%)
FATE_VSYNTH2 = $(VCODEC_TESTS:%=fate-vsynth2-%)
FATE_VCODEC  = $(FATE_VSYNTH1) $(FATE_VSYNTH2)
FATE_LAVF    = $(LAVF_TESTS:%=fate-lavf-%)
FATE_LAVFI   = $(LAVFI_TESTS:%=fate-lavfi-%)
FATE_SEEK    = $(SEEK_TESTS:seek_%=fate-seek-%)

FATE = $(FATE_ACODEC)                                                   \
       $(FATE_VCODEC)                                                   \
       $(FATE_LAVF)                                                     \
       $(FATE_LAVFI)                                                    \
       $(FATE_SEEK)                                                     \

$(FATE_ACODEC): $(AREF)
$(FATE_VCODEC): $(VREF)
$(FATE_LAVF):   $(REFS)
$(FATE_LAVFI):  $(REFS) tools/lavfi-showfiltfmts$(EXESUF)
$(FATE_SEEK):   fate-codec fate-lavf tests/seek_test$(EXESUF)

$(FATE_ACODEC):  CMD = codectest acodec
$(FATE_VSYNTH1): CMD = codectest vsynth1
$(FATE_VSYNTH2): CMD = codectest vsynth2
$(FATE_LAVF):    CMD = lavftest
$(FATE_LAVFI):   CMD = lavfitest
$(FATE_SEEK):    CMD = seektest

fate-codec:  fate-acodec fate-vcodec
fate-acodec: $(FATE_ACODEC)
fate-vcodec: $(FATE_VCODEC)
fate-lavf:   $(FATE_LAVF)
fate-lavfi:  $(FATE_LAVFI)
fate-seek:   $(FATE_SEEK)

ifdef SAMPLES
FATE += $(FATE_TESTS)
else
$(FATE_TESTS):
	@echo "SAMPLES not specified, cannot run FATE"
endif

FATE_UTILS = base64 tiny_psnr

fate: $(FATE)

$(FATE): ffmpeg$(EXESUF) $(FATE_UTILS:%=tests/%$(HOSTEXESUF))
	@echo "TEST    $(@:fate-%=%)"
	$(Q)$(SRC_PATH)/tests/fate-run.sh $@ "$(SAMPLES)" "$(TARGET_EXEC)" "$(TARGET_PATH)" '$(CMD)' '$(CMP)' '$(REF)' '$(FUZZ)' '$(THREADS)'

fate-list:
	@printf '%s\n' $(sort $(FATE))

.PHONY: documentation *test regtest-* alltools check config
