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
	man2html $(man_MANS) | sed '1,2d' > $(man_MANS).html; \
	echo " done";

clean:
	@echo -n "Cleaning...";\
	rm $(man_MANS).ps.gz $(man_MANS).pdf $(man_MANS).html;\
	echo " done"

distclean: clean
maintainer-clean: clean

NEMESI_DOCS_DIR = $(DESTDIR)@fenice_docs_dir@

install-data-local:
	
	$(INSTALL) -m 755 -d $(NEMESI_DOCS_DIR)
	$(INSTALL_DATA) $(man_MANS).ps.gz $(NEMESI_DOCS_DIR)
	$(INSTALL_DATA) $(man_MANS).pdf $(NEMESI_DOCS_DIR)
	$(INSTALL_DATA) $(man_MANS).html $(NEMESI_DOCS_DIR)

uninstall-local:
	rm -rf $(NEMESI_DOCS_DIR)

.PHONY: distclean
