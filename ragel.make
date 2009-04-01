# -*- Makefile -*-

SUFFIXES = .rl

.rl.c:
	$(mkdir_p) $(dir $@)
	$(RAGEL) $(RAGELFLAGS) -C $< -o $@
