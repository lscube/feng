EXTRA_DIST = index.html index-1.html index-2.html index-3.html index-4.html index-5.html index-6.html 

FENICE_DOCS_DIR = $(DESTDIR)@fenice_docs_dir@

install-data-local:
	
	$(INSTALL) -m 755 -d $(FENICE_DOCS_DIR)
	$(INSTALL_DATA) *.html $(FENICE_DOCS_DIR)

uninstall-local:
	rm -rf $(FENICE_DOCS_DIR)
