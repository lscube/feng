EXTRA_DIST = fenice-howto.html

FENICE_DOCS_DIR_HOWTO = $(DESTDIR)@fenice_docs_dir@/howto

install-data-local:
	
	$(INSTALL) -m 755 -d $(FENICE_DOCS_DIR_HOWTO)
	$(INSTALL_DATA) $(srcdir)/*.html $(FENICE_DOCS_DIR_HOWTO)

uninstall-local:
	rm -rf $(FENICE_DOCS_DIR_HOWTO)

update:
	wget https://streaming.polito.it/book/print/31 -O - | tidy -quiet --show-warnings no -indent --indent-spaces 4 > fenice-howto.html || :
