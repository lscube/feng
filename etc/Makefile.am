FENICE_CONF_FILE = fenice.conf
FENICE_CONF_DIR = /etc

install-data-local:
	@if test -f $(FENICE_CONF_DIR)/$(FENICE_CONF_FILE); then \
		echo "Found $(FENICE_CONF_DIR)/$(FENICE_CONF_FILE). Saving new as $(FENICE_CONF_DIR)/$(FENICE_CONF_FILE).new"; \
		cp fenice.conf $(FENICE_CONF_DIR)/$(FENICE_CONF_FILE).new; \
	else \
		cp fenice.conf $(FENICE_CONF_DIR)/$(FENICE_CONF_FILE); \
	fi
		
