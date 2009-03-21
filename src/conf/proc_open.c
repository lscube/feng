/**
 * Copyright (c) 2004, Jan Kneschke, incremental
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of the 'incremental' nor the names of its contributors
 *   may be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include "proc_open.h"

#include <sys/wait.h>
#include <unistd.h>

/* {{{ unix way */
# define SHELLENV "SHELL"
# define SECURITY_DC
# define SECURITY_CC
# define close_descriptor(fd) close(fd)
static void pipe_close_parent(pipe_t *p) {
	/* don't close stdin */
	close_descriptor(p->parent);
	if (dup2(p->child, p->fd) != p->fd) {
		perror("pipe_child dup2");
	} else {
		close_descriptor(p->child);
		p->child = p->fd;
	}
}
static void pipe_close_child(pipe_t *p) {
	close_descriptor(p->child);
	p->fd = p->parent;
}
/* }}} */

/* {{{ pipe_close */
static void pipe_close(pipe_t *p) {
	close_descriptor(p->parent);
	close_descriptor(p->child);
}
/* }}} */
/* {{{ pipe_open */
static int pipe_open(pipe_t *p, int fd SECURITY_DC) {
	descriptor_t newpipe[2];

	if (0 != pipe(newpipe)) {
		fprintf(stderr, "can't open pipe");
		return -1;
	}
	if (0 == fd) {
		p->parent = newpipe[1]; /* write */
		p->child  = newpipe[0]; /* read */
	} else {
		p->parent = newpipe[0]; /* read */
		p->child  = newpipe[1]; /* write */
	}
	p->fd = fd;

	return 0;
}
/* }}} */

/* {{{ proc_open_pipes */
static int proc_open_pipes(proc_handler_t *proc SECURITY_DC) {
	if (pipe_open(&(proc->in), 0 SECURITY_CC) != 0) {
		return -1;
	}
	if (pipe_open(&(proc->out), 1 SECURITY_CC) != 0) {
		return -1;
	}
	if (pipe_open(&(proc->err), 2 SECURITY_CC) != 0) {
		return -1;
	}
	return 0;
}
/* }}} */
/* {{{ proc_close_pipes */
static void proc_close_pipes(proc_handler_t *proc) {
	pipe_close(&proc->in);
	pipe_close(&proc->out);
	pipe_close(&proc->err);
}
/* }}} */
/* {{{ proc_close_parents */
static void proc_close_parents(proc_handler_t *proc) {
	pipe_close_parent(&proc->in);
	pipe_close_parent(&proc->out);
	pipe_close_parent(&proc->err);
}
/* }}} */
/* {{{ proc_close_childs */
static void proc_close_childs(proc_handler_t *proc) {
	pipe_close_child(&proc->in);
	pipe_close_child(&proc->out);
	pipe_close_child(&proc->err);
}
/* }}} */

/* {{{ proc_close */
static int proc_close(proc_handler_t *proc) {
	pid_t child = proc->child;
	int wstatus;
	pid_t wait_pid;

	proc_close_pipes(proc);

	do {
		wait_pid = waitpid(child, &wstatus, 0);
	} while (wait_pid == -1 && errno == EINTR);

	if (wait_pid == -1) {
		return -1;
	} else {
		if (WIFEXITED(wstatus))
			wstatus = WEXITSTATUS(wstatus);
	}

	return wstatus;
}
/* }}} */
/* {{{ proc_open */
static int proc_open(proc_handler_t *proc, const char *command) {
	pid_t child;
	const char *shell;

	if (NULL == (shell = getenv(SHELLENV))) {
		shell = "/bin/sh";
	}

	if (proc_open_pipes(proc) != 0) {
		return -1;
	}

	/* the unix way */

	child = fork();

	if (child == 0) {
		/* this is the child process */

		/* close those descriptors that we just opened for the parent stuff,
		 * dup new descriptors into required descriptors and close the original
		 * cruft
		 */
		proc_close_parents(proc);

		execl(shell, shell, "-c", command, (char *)NULL);
		_exit(127);

	} else if (child < 0) {
		fprintf(stderr, "failed to forking");
		proc_close(proc);
		return -1;

	} else {
		proc->child = child;
		proc_close_childs(proc);
		return 0;
	}
}
/* }}} */

/* {{{ proc_read_fd_to_buffer */
static void proc_read_fd_to_buffer(int fd, conf_buffer *b) {
	ssize_t s;

	for (;;) {
		buffer_prepare_append(b, 512);
		if ((s = read(fd, (void *)(b->ptr + b->used), 512 - 1)) <= 0) {
			break;
		}
		b->used += s;
	}
	b->ptr[b->used] = '\0';
}
/* }}} */
/* {{{ proc_open_buffer */
int proc_open_buffer(proc_handler_t *proc, const char *command, conf_buffer *in, conf_buffer *out, conf_buffer *err) {

	UNUSED(err);

	if (proc_open(proc, command) != 0) {
		return -1;
	}

	if (in) {
		if (write(proc->in.fd, (void *)in->ptr, in->used) < 0) {
			perror("error writing pipe");
			return -1;
		}
	}
	pipe_close(&proc->in);

	if (out) {
		proc_read_fd_to_buffer(proc->out.fd, out);
	}
	pipe_close(&proc->out);

	if (err) {
		proc_read_fd_to_buffer(proc->err.fd, err);
	}
	pipe_close(&proc->err);

	return 0;
}
/* }}} */

/* {{{ test */
#ifdef DEBUG_PROC_OPEN
int main() {
	proc_handler_t proc;
	conf_buffer *in = buffer_init(), *out = buffer_init(), *err = buffer_init();
	int wstatus;

#define FREE() do { \
	buffer_free(in); \
	buffer_free(out); \
	buffer_free(err); \
} while (0)

#define RESET() do { \
	buffer_reset(in); \
	buffer_reset(out); \
	buffer_reset(err); \
	wstatus = proc_close(&proc); \
	if (0&&wstatus != 0) { \
		fprintf(stdout, "exitstatus %d\n", wstatus); \
		return __LINE__ - 200; \
	} \
} while (0)

#define ERROR_OUT() do { \
	fprintf(stdout, "failed opening proc\n"); \
	wstatus = proc_close(&proc); \
	fprintf(stdout, "exitstatus %d\n", wstatus); \
	FREE(); \
	return __LINE__ - 300; \
} while (0)

#define CMD_CAT "cat"

	do {
		fprintf(stdout, "test: echo 123 without read\n");
		if (proc_open(&proc, "echo 321") != 0) {
			ERROR_OUT();
		}
		close_descriptor(proc.in.parent);
		close_descriptor(proc.out.parent);
		close_descriptor(proc.err.parent);
		RESET();

		fprintf(stdout, "test: echo 321 with read\n"); fflush(stdout);
		if (proc_open_buffer(&proc, "echo 321", NULL, out, err) != 0) {
			ERROR_OUT();
		}
		fprintf(stdout, "result: ->%s<-\n\n", out->ptr); fflush(stdout);
		RESET();

		fprintf(stdout, "test: echo 123 | " CMD_CAT "\n"); fflush(stdout);
		buffer_copy_string_len(in, CONST_STR_LEN("123\n"));
		if (proc_open_buffer(&proc, CMD_CAT, in, out, err) != 0) {
			ERROR_OUT();
		}
		fprintf(stdout, "result: ->%s<-\n\n", out->ptr); fflush(stdout);
		RESET();
	} while (0);

#undef RESET
#undef ERROR_OUT

	fprintf(stdout, "ok\n");

	FREE();
	return 0;
}
#endif /* DEBUG_PROC_OPEN */
/* }}} */

