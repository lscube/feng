SUBDIRS = src

noinst_HEADERS = \
	include/fenice/utils.h \
	include/fenice/fnc_log.h \
	include/fenice/multicast.h \
	include/fenice/prefs.h \
	include/fenice/schedule.h \
	include/fenice/server.h \
	include/fenice/debug.h \
	include/fenice/sdp_grammar.h

EXTRA_DIST = RELEASE

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
