FENICE_AVROOT_DIR = $(DESTDIR)/var/fenice/avroot/

FENICE_TEST_FILES = test.sd audio.mp3 video.mpg

install-data-local:
	$(INSTALL) -m 755 -d $(FENICE_AVROOT_DIR)
	$(INSTALL) $(FENICE_TEST_FILES) $(FENICE_AVROOT_DIR)

uninstall-local:
	@echo -n "Removing A/V Test Files...";\
	for FILE in $(FENICE_TEST_FILES); do\
		rm $(FENICE_AVROOT_DIR)/$$FILE;\
	done;\
	echo " done."
