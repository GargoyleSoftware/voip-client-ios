silk_version = 1.0.8
silk_extracted_directory = SILK_SDK_SRC_v$(silk_version)
silk_src_dir = $(silk_extracted_directory)/SILK_SDK_SRC_$(SILK_FLAVOUR)_v$(silk_version)

silk_zip = $(silk_extracted_directory).zip
silk_url = http://developer.skype.com/silk/$(silk_zip)

downloaded_files = $(silk_zip)
checkmarks = $(downloaded_files:%=%.check)
BUILT_SOURCES = $(downloaded_files) $(checkmarks) $(silk_extracted_directory) $(silk_src_dir) $(silk_extracted_directory)

checksum_dir = $(srcdir)/known_checksums
checksum_files = $(addprefix $(checksum_dir)/, $(downloaded_files:%=%.md5))
EXTRA_DIST = $(checksum_files)

$(srcdir)/$(silk_zip):
	$(WGET) $(silk_url) -O $(srcdir)/${silk_zip}


# for check, go to $(srcdir) or ./, depending on where file is
$(checkmarks): %.check: %
	cd $(<D); $(MD5SUM) -c $(checksum_dir)/$(<F).md5
	touch $@

$(silk_extracted_directory): $(checkmarks)
	$(UNZIP) $(srcdir)/$(silk_zip) $(silk_src_dir)/*

$(silk_src_dir): $(silk_extracted_directory)
	cp $(srcdir)/patch_pic.diff $(silk_src_dir)
	cd $(silk_src_dir) && $(PATCH) -p0 < patch_pic.diff

extract-sources: $(silk_src_dir)


# Call Skype Makefile to build the library
all-local: $(silk_src_dir)
	cd $(silk_src_dir) && $(MAKE) AR=$(AR) RANLIB="$(RANLIB)" CC="$(CC) -mno-thumb" LD="$(LD)" $(SILK_MAKE_OPTIONS) $(AM_MAKEFLAGS) lib
check-local: $(silk_src_dir)
	cd $(silk_src_dir) && $(MAKE) $(AM_MAKEFLAGS) test
clean-local: $(silk_src_dir)
	cd $(silk_src_dir) && $(MAKE) $(AM_MAKEFLAGS) clean

install-data-local: $(silk_src_dir)
	cp $(silk_src_dir)/libSKP_SILK_SDK.a $(prefix)/lib

DISTCLEANFILES = $(BUILT_SOURCES)
