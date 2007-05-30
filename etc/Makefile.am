FENICE_CONF_FILE = feng.conf
FENICE_CONF_DIR = $(DESTDIR)@fenice_conf_dir@

EXTRA_DIST = $(FENICE_CONF_FILE).in

all: feng.conf

feng.conf: feng.conf.in ../config.h
	cpp -I.. -P $(srcdir)/$(FENICE_CONF_FILE).in > $(FENICE_CONF_FILE)

clean:
	rm -f $(FENICE_CONF_FILE)

install-data-local:
	@if test -f $(FENICE_CONF_DIR)/$(FENICE_CONF_FILE); then \
		echo "Found $(FENICE_CONF_DIR)/$(FENICE_CONF_FILE). Saving new as $(FENICE_CONF_DIR)/$(FENICE_CONF_FILE).new"; \
		cp feng.conf $(FENICE_CONF_DIR)/$(FENICE_CONF_FILE).new; \
	else \
		if test ! -d $(FENICE_CONF_DIR); then \
			mkdir $(FENICE_CONF_DIR); \
		fi; \
		cp feng.conf $(FENICE_CONF_DIR); \
	fi
		    
