EXTRA_DIST = fenice-howto.html

FENICE_DOCS_DIR = $(DESTDIR)@fenice_docs_dir@

install-data-local:
	
	$(INSTALL) -m 755 -d $(FENICE_DOCS_DIR)
	$(INSTALL_DATA) *.html $(FENICE_DOCS_DIR)

uninstall-local:
	rm -rf $(FENICE_DOCS_DIR)

update:
	wget https://killgates.polito.it/streaming/book/print/31 -O - | tidy -quiet --show-warnings no -indent --indent-spaces 4 > fenice-howto.html
