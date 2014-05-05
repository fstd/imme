/* immectl.c - (C) 2014, Timo Buhrmester
 * immectl - ...
 * See README for contact-, COPYING for license information. */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <getopt.h>

#include <libsrsbsns/misc.h>

#include "sercomm.h"
#include "eval.h"
#include "dbglog.h"

#define DEF_DEV "/dev/dtyU0"
#define DEF_BAUD 38400

//when using sercommd (-D):
#define DEF_TRG "localhost"
#define DEF_PORT 34321

static bool s_skipinit = true;
static char *s_dev;
static int s_baud;
static bool s_stdin = true;
static bool s_sercommd;
static bool s_failterm = false;


static void process_args(int *argc, char ***argv);
static void init(int *argc, char ***argv);
static void usage(FILE *str, const char *a0, int ec);


static void
process_args(int *argc, char ***argv)
{
	char *a0 = (*argv)[0];

	for(int ch; (ch = getopt(*argc, *argv, "d:b:Dervqch")) != -1;) {
		switch (ch) {
		case 'd':
			s_dev = strdup(optarg);
			D("set device to '%s'", s_dev);
			break;
		case 'b':
			s_baud = (int)strtol(optarg, NULL, 0);
			D("set baud rate to '%d'", s_baud);
			break;
		case 'D':
			s_sercommd = true;
			break;
		case 'e':
			s_failterm = true;
			D("will terminate on eval failure");
			break;
		case 'r':
			s_skipinit = false;
			D("will reinit target");
			break;
		case 'v':
			dbg_setlvl(dbg_getlvl()+1);
			break;
		case 'q':
			dbg_setlvl(dbg_getlvl()-1);
			break;
		case 'c':
			dbg_setfancy(true);
			break;
		case 'h':
			usage(stdout, a0, EXIT_SUCCESS);
			break;
		case '?':
		default:
			usage(stderr, a0, EXIT_FAILURE);
		}
	}

	*argc -= optind;
	*argv += optind;

	optreset = 1;
	optind = 1;
}


static void
init(int *argc, char ***argv)
{
	process_args(argc, argv);

	if (!ev_init())
		C("failed to initialize eval/dispatch");
	
	if (s_sercommd) {
		if (!s_dev)
			s_dev = strdup(DEF_TRG);

		if (!s_baud)
			s_baud = DEF_PORT;

		if (!sc_init_tcp(s_dev, (uint16_t)s_baud))
			C("failed to initialize sercomm (trg: '%s', port: %d)",
			    s_dev, s_baud);
	} else {
		if (!s_dev)
			s_dev = strdup(DEF_DEV);

		if (!s_baud)
			s_baud = DEF_BAUD;

		if (!sc_init(s_dev, s_baud))
			C("failed to initialize sercomm (dev: '%s', baud: %d)",
			    s_dev, s_baud);
	}

	if (*argc)
		s_stdin = false;
}


static void
usage(FILE *str, const char *a0, int ec)
{
	#define H(STR) fputs(STR "\n", str)
	H("=============");
	H("== immectl ==");
	H("=============");
	fprintf(str, "usage: %s [-envqch -d <dev> -b <baud>]\n", a0);
	H("");
	H("\t-d <str>: Specify serial device to use (def: /dev/dtyU0)");
	H("\t-b <int>: Baud rate to use (def: 38400)");
	H("\t-e: Abort when evaluation of a line fails (``set -e''-like)");
	H("\t-D: Use sercommd via TCP instead of directly opening the serport");
	H("\t      When used, -d and -b specify target host and port, respectively");
	H("\t-r: Reinit target (rst+dbgen)");
	H("\t-v: Increase verbosity on stderr");
	H("\t-q: Decrease verbosity on stderr");
	H("\t-c: Use fancy bash(1)-style color sequences on stderr");
	H("\t-h: Display brief usage statement and terminate");
	H("");
	H("(C) 2014, Timo Buhrmester (contact: #fstd on irc.freenode.org)");
	#undef H
	exit(ec);
}


int
main(int argc, char **argv)
{
	init(&argc, &argv);

	I("initializad (optind is %d, argc is now %d, argv[0]: '%s')",
	    optind, argc, argv[0]);

	if (!s_skipinit) {
		D("initializing target");
		char *s = "REINIT";
		if (!ev_dispatch(1, &s))
			C("failed to initialize target");
	} else
		N("will /not/ (re)initialize target...");

	if (!s_stdin)
		return ev_dispatch(argc, argv) ? EXIT_SUCCESS : EXIT_FAILURE;

	char *line = NULL;
	size_t linesize = 0;
	ssize_t linelen;

	size_t nargc = 8;
	char **nargv = malloc(nargc * sizeof *nargv); //grows

	bool fail = false;

	while ((linelen = getline(&line, &linesize, stdin)) != -1) {
		D("read line: '%s'", line);
		char *dup = strdup(line);
		while (isspace(*dup))
			dup++;
		D("skipped leading WS: '%s'", dup);

		char *p = strchr(dup, '#');
		if (p)
			*p = '\0';

		D("kill comments: '%s'", dup);

		p = dup + strlen(dup);

		while (p > dup) {
			if (isspace(*--p))
				*p = '\0';
			else
				break;
		}

		D("strip trailing WS: '%s'", dup);

		if (strlen(dup)) {
			int ac = splitquoted(dup, nargv, nargc);
			if (ac < 0)
				C("failed to split! (raw line: '%s')", line);

			bool resized = false;
			while ((size_t)ac > nargc) {
				nargc *= 2;
				char **na = malloc(nargc * sizeof *na);
				if (!na)
					CE("malloc");
				free(nargv);
				nargv = na;
				resized = true;
			}

			if (resized)
				ac = splitquoted(dup, nargv, nargc);


			if (!ev_dispatch(ac, nargv)) {
				fail = false;
				if (s_failterm)
					C("eval failed on (raw) line '%s', aborting",
					    line);
				else
					W("eval failed on (raw) line '%s'", line);
			}
		}

		free(dup);
	}

	if (ferror(stdin))
		CE("getline");

	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
