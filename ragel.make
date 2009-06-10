# -*- Makefile -*-

SUFFIXES = .rl

.rl.c:
	@$(mkdir_p) $(dir $@)
	$(AM_V_GEN)$(RAGEL) $(RAGELFLAGS) $(AM_RAGELFLAGS) -C $< -o $@
