SUBDIRS = howto

man_MANS = fenice.1

EXTRA_DIST = 	$(man_MANS)

all: $(man_MANS).ps.gz $(man_MANS).pdf $(man_MANS).html


$(man_MANS).ps.gz:
	@which groff >/dev/null 2>&1; \
	if test $$? == 0 ; then \
		echo -n "Creating Postscript..."; \
		groff -mandoc -e -t -Tps $(man_MANS) > $(man_MANS).ps; \
		gzip -9 -f $(man_MANS).ps; \
		echo " done"; \
	else \
		echo "ERROR! groff is not installed! I can't create PS file from man page!"; \
	fi

$(man_MANS).pdf:
	@which ps2pdf >/dev/null 2>&1; \
	if test $$? == 0 ; then \
		echo -n "Creating Pdf..."; \
		zcat $(man_MANS).ps.gz | ps2pdf - $(man_MANS).pdf; \
		echo " done"; \
	else \
		echo "ERROR! ps2pdf is not installed! I can't create PDF file from man page!"; \
	fi

$(man_MANS).html:
	@which groff >/dev/null 2>&1; \
	if test $$? == 0 ; then \
		echo -n "Creating HTML..."; \
		groff -mandoc -e -t -Thtml $(man_MANS) > $(man_MANS).html; \
		if test $$? == 3 ; then \
			echo "ERROR! groff does not support HTML output! I can't create HTML file from man page!"; \
			rm $(man_MANS).html; \
		fi; \
		echo " done"; \
	fi

CLEANFILES = $(man_MANS).ps.gz $(man_MANS).pdf $(man_MANS).html

FENICE_DOCS_DIR = $(DESTDIR)@fenice_docs_dir@

install-data-local:
	$(INSTALL) -m 755 -d $(FENICE_DOCS_DIR)
	test -f $(man_MANS).ps.gz && $(INSTALL_DATA) $(man_MANS).ps.gz $(FENICE_DOCS_DIR) || : 
	test -f $(man_MANS).pdf && $(INSTALL_DATA) $(man_MANS).pdf $(FENICE_DOCS_DIR) || :
	test -f $(man_MANS).html && $(INSTALL_DATA) $(man_MANS).html $(FENICE_DOCS_DIR) || :

uninstall-local:
	rm -rf $(FENICE_DOCS_DIR)
