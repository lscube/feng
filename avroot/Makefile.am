FENICE_ROOT_DIR = /var/fenice/root/
FENICE_TEST_FILES = test.sd audio.mp3 video.mpg

install-data-local:
	$(INSTALL) -m 755 -d $(FENICE_ROOT_DIR)
	$(INSTALL) $(FENICE_TEST_FILES) $(FENICE_ROOT_DIR)

uninstall-local:
	rm -rf $(FENICE_ROOT_DIR)
