#include <config.h>
#ifndef _plan9
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>
#ifndef _MAC
#include <sys/types.h>
#endif
#include <fconfig.h>
#include <filter.h>
#include <fractal.h>
#include <string.h>
#include <ui_helper.h>
#include <ui.h>
#include <timers.h>
#include <xmenu.h>
#include "uiint.h"
#ifdef COMPILE_PIPE
static int pipefd = -1;
tl_timer *pipetimer;
static char pipecommand[256];
static int commandpos;
static int textmode = 0;
static int backslash = 0;
static int nest = -1;
#define add(cmd) (pipecommand[commandpos++]=cmd)
static void ui_pipe_handler(void *data, int q)
{
#ifdef O_NONBLOCK
    char buf[100];
    int n = (int) read(pipefd, buf, 100);
    int i;
    if (n > 0) {
	buf[n] = 0;
    }
    for (i = 0; i < n; i++) {
	if (backslash) {
	    add(buf[i]);
	    continue;
	}
	if (textmode) {
	    if (buf[i] == '\\') {
		add(buf[i]);
		backslash = 1;
		continue;
	    }
	    if (buf[i] == '"') {
		add(buf[i]);
		textmode = 0;
		continue;
	    }
	    add(buf[i]);
	} else {
	    add(buf[i]);
	    if (buf[i] == '"')
		textmode = 1;
	    if (buf[i] == ')')
		nest--;
	    if (buf[i] == '(') {
		if (nest == -1)
		    nest = 1;
		else
		    nest++;
	    }
	    if (!nest) {
		add(0);
		uih_command(uih, pipecommand);
		nest = -1;
		commandpos = 0;
	    }
	}
    }
#endif
}

void ui_pipe_init(const char *name)
{
#ifdef O_NONBLOCK
    if ((int) strlen(name) == 1 && name[0] == '-') {
	pipefd = 0;
	fcntl(pipefd, F_SETFL, O_NONBLOCK);
    } else {
	pipefd = open(name, O_NONBLOCK);
	if (pipefd == -1) {
	    perror(name);
	    exit(1);
	}
    }
    pipetimer = tl_create_timer();
    tl_set_multihandler(pipetimer, ui_pipe_handler, NULL);
    tl_reset_timer(pipetimer);
    tl_set_interval(pipetimer, 100000);
    tl_add_timer(syncgroup, pipetimer);
#else
    printf
	("Fatal error - constant O_NONBLOCK used for non-blocking IO in the pipe handling"
	 "is unknown at the moment of compilation. The non-blocking IO will not work. "
	 "Please ask authors...\n");
    exit(0);
#endif
}

static void ui_pipe_close(void)
{
#ifdef O_NONBLOCK
    if (pipefd == -1)
	return;
    close(pipefd);
    tl_remove_timer(pipetimer);
#endif
}
#endif
#endif
