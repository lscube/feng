SUBDIRS = src

ACLOCAL_AMFLAGS = -I m4

FENICE_AVROOT_DIR = $(fenice_avroot_dir)
FENICE_LOG_FILE = `basename $(fenice_log_file)`

avrootdir = $(fenice_avroot_dir)

dist_avroot_DATA = \
	avroot/h264.ds \
	avroot/h264_scramble.ds \
	avroot/test.mov

feng_confdir = $(fenice_conf_dir)
dist_feng_conf_DATA = etc/feng.conf

dist_man_MANS = docs/feng.1

dist_doc_DATA = AUTHORS ChangeLog NEWS README RELEASE TODO
MAINTAINERCLEANFILES = ChangeLog

if CHANGELOG_REGEN
ChangeLog:
	GIT_DIR="${srcdir}/.git" git log --pretty > $@

# Always regenerate!
.PHONY: ChangeLog
endif
