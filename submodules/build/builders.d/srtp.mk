srtp_dir?=externals/srtp

$(BUILDER_SRC_DIR)/$(srtp_dir)/configure: 
	cd $(BUILDER_SRC_DIR)/$(srtp_dir) \
	&& autoconf
$(BUILDER_BUILD_DIR)/$(srtp_dir)/Makefile: $(BUILDER_SRC_DIR)/$(srtp_dir)/configure
	mkdir -p $(BUILDER_BUILD_DIR)/$(srtp_dir)
	cd $(BUILDER_BUILD_DIR)/$(srtp_dir)/\
	&& CONFIG_SITE=$(BUILDER_SRC_DIR)/build/$(config_site) \
	$(BUILDER_SRC_DIR)/$(srtp_dir)/configure -prefix=$(prefix) --host=$(host) ${library_mode}

build-srtp: $(BUILDER_BUILD_DIR)/$(srtp_dir)/Makefile
	cd $(BUILDER_BUILD_DIR)/$(srtp_dir) && make libsrtp.a && make uninstall && make install

clean-srtp:
	-cd $(BUILDER_BUILD_DIR)/$(srtp_dir)  && make clean

veryclean-srtp:
	-cd $(BUILDER_BUILD_DIR)/$(srtp_dir) && make distclean
	-rm -rf $(BUILDER_BUILD_DIR)/$(srtp_dir)
	-rm -f $(BUILDER_SRC_DIR)/$(srtp_dir)/configure

clean-makefile-srtp:
	-cd $(BUILDER_BUILD_DIR)/$(srtp_dir) && rm -f Makefile

