FENICE_AVROOT_DIR = $(DESTDIR)@fenice_avroot_dir@

FENICE_TEST_FILES = h264.ds h264_scramble.ds test.mov

EXTRA_DIST = $(FENICE_TEST_FILES)

install-data-local:
	$(INSTALL) -m 755 -d $(FENICE_AVROOT_DIR)
	$(INSTALL_DATA) $(srcdir)/$(FENICE_TEST_FILES) $(FENICE_AVROOT_DIR)

uninstall-local:
	@echo -n "Removing A/V Test Files...";\
	for FILE in $(FENICE_TEST_FILES); do\
		rm $(FENICE_AVROOT_DIR)/$$FILE;\
	done;\
	echo " done."
