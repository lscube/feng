SUBDIRS = howto

man_MANS = fenice.1

EXTRA_DIST = 	$(man_MANS)

all:
	@echo -n "Creating Postscript..."; \
	groff -mandoc -e -t -Tps $(man_MANS) > $(man_MANS).ps; \
	echo " done";
	@echo -n "Creating Pdf..."; \
	ps2pdf $(man_MANS).ps $(man_MANS).pdf; \
	echo " done"; \
	gzip -9 -f $(man_MANS).ps
	@echo -n "Creating HTML..."; \
	groff -mandoc -e -t -Thtml $(man_MANS) > $(man_MANS).html; \
	echo " done";

clean:
	@echo -n "Cleaning...";\
	rm $(man_MANS).ps.gz $(man_MANS).pdf $(man_MANS).html;\
	echo " done"

distclean: clean
maintainer-clean: clean

FENICE_DOCS_DIR = $(DESTDIR)@fenice_docs_dir@

install-data-local:
	
	$(INSTALL) -m 755 -d $(FENICE_DOCS_DIR)
	$(INSTALL_DATA) $(man_MANS).ps.gz $(FENICE_DOCS_DIR)
	$(INSTALL_DATA) $(man_MANS).pdf $(FENICE_DOCS_DIR)
	$(INSTALL_DATA) $(man_MANS).html $(FENICE_DOCS_DIR)

uninstall-local:
	rm -rf $(FENICE_DOCS_DIR)

.PHONY: distclean
