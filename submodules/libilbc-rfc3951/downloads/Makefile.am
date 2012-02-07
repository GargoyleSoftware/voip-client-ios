# here are the file names and urls for the technical specification
technical_spec = rfc3951.txt
technical_spec_url = http://www.ietf.org/rfc/$(technical_spec)

# for the extract script
extract_script = extract-cfile.awk
extract_script_url = http://www.webrtc.org/ilbc-freeware/ilbc-source-code-and-utili/ilbc-utilities/$(extract_script)

downloaded_files = $(technical_spec) $(extract_script) $(license)
checkmarks = $(downloaded_files:%=%.check)
success_marker = ../src/iLBC_define.h
BUILT_SOURCES = $(downloaded_files) $(checkmarks) $(success_marker)

checksum_dir = $(srcdir)/known_checksums
checksum_files = $(addprefix $(checksum_dir)/, $(downloaded_files:%=%.md5))
EXTRA_DIST = $(checksum_files)

$(technical_spec):
	$(WGET) $(technical_spec_url) -O ${technical_spec}

$(extract_script):
	$(WGET) $(extract_script_url) -O ${extract_script}


# for check, go to $(srcdir) or ./, depending on where file is
$(checkmarks): %.check: %
	cd $(<D); $(MD5SUM) -c $(checksum_dir)/$(<F).md5
	touch $@

# cleaning up in ../src is necessary because the awk script appends
$(success_marker): $(extract_script) $(technical_spec) $(checkmarks)
	cd ../src; rm *.c *.h &> /dev/null; $(AWK) -f $(CURDIR)/$< \
		$(CURDIR)/$(word 2, $+) > /dev/null

DISTCLEANFILES = $(BUILT_SOURCES)
