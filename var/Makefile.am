
if HAVE_SSL
FENICE_CERT_FILE = cert.pem
FENICE_CERT_DIR = $(DESTDIR)@fenice_state_dir@

EXTRA_DIST = 

all:	cert.pem 

cert.pem: 
	@if test -f $(FENICE_CERT_DIR)/$(FENICE_CERT_FILE); then \
		echo "Found $(FENICE_CERT_DIR)/$(FENICE_CERT_FILE)."; \
	else \
		echo "*************************************************************************************************************************************";\
		echo "	Please create a cert.pem file and put it in $(FENICE_CERT_DIR)/ ";\
		echo "*************************************************************************************************************************************";\
	fi

clean:

install-data-local:
	@if test -f $(FENICE_CERT_DIR)/$(FENICE_CERT_FILE); then \
		echo "Found $(FENICE_CERT_DIR)/$(FENICE_CERT_FILE)."; \
	else \
		if test ! -d $(FENICE_CERT_DIR); then \
			mkdir -p $(FENICE_CERT_DIR); \
		fi; \
		echo "*************************************************************************************************************************************";\
		echo "	Please create a cert.pem file and put it in $(FENICE_CERT_DIR)/ ";\
		echo "*************************************************************************************************************************************";\
	fi
endif
