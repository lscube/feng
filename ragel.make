# -*- Makefile -*-

SUFFIXES = .rl

.rl.c:
	$(mkdir_p) $(dir $@)
	$(RAGEL) $(RAGELFLAGS) $(AM_RAGELFLAGS) -C $< -o $@
