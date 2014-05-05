/* sercommd.c - (C) 2014, Timo Buhrmester
 * immectl - ...
 * See README for contact-, COPYING for license information. */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <signal.h>
#include <sys/resource.h>
#include <sys/select.h>

#include <getopt.h>

#include <libsrsbsns/misc.h>
#include <libsrsbsns/addr.h>

#include "sercomm.h"
#include "eval.h"
#include "dbglog.h"

#define DEF_DEV "/dev/dtyU0"
#define DEF_BAUD 38400

static char *s_dev;
static int s_baud;
static bool s_nofork;
static int s_sck; //listening socket
static int s_scfd; //serial port


static int read_one(int fd);
static bool write_one(int fd, uint8_t b);

static void process_args(int *argc, char ***argv);
static void init(int *argc, char ***argv);
static void usage(FILE *str, const char *a0, int ec);


void
handle(int csck)
{
	int maxfd = csck > s_scfd ? csck : s_scfd;
	for (;;) {
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(csck, &fds);
		FD_SET(s_scfd, &fds);


		struct timeval tv = { 1, 0 };

		int r = select(maxfd+1, &fds, NULL, NULL, &tv);

		if (r == 0)
			continue;

		if (r == -1)
			CE("select");

		if (FD_ISSET(s_scfd, &fds)) {
			int x = read_one(s_scfd);
			if (x == -1)
				C("failed to read a byte from serial");
			if (!write_one(csck, x)) {
				E("failed to write a byte to client");
				break; //ditch this shitclient
			}
		}

		if (FD_ISSET(csck, &fds)) {
			int x = read_one(csck);
			if (x == -1) {
				E("failed to read a byte from client");
				break; //ditch this shitclient
			} else if (x == -2) { //EOF
				I("EOF from client");
				break;
			}

			if (!write_one(s_scfd, x)) {
				C("failed to write a byte to serial");
			}
		}
	}
}

static int
read_one(int fd)
{
	uint8_t x = 0;
	ssize_t r = read(fd, &x, 1);
	if (r == -1) {
		EE("read from %d", fd);
		return -1;
	}
	
	if (r == 0) {
		I("EOF from %d", fd);
		return -2;
	}

	D("read from %d: %#2.2hhx ('%c')", fd, x, x);

	return x;
		
}

static bool
write_one(int fd, uint8_t b)
{
	ssize_t r = write(fd, &b, 1);
	if (r != 1) {
		EE("write to %d: %d", fd, r);
		return false;
	}
	
	D("write to %d: %#2.2hhx ('%c')", fd, b, b);

	return true;
}



static void
process_args(int *argc, char ***argv)
{
	char *a0 = (*argv)[0];

	for(int ch; (ch = getopt(*argc, *argv, "d:b:envqch")) != -1;) {
		switch (ch) {
		case 'd':
			s_dev = strdup(optarg);
			D("set device to '%s'", s_dev);
			break;
		case 'b':
			s_baud = (int)strtol(optarg, NULL, 0);
			D("set baud rate to '%d'", s_baud);
			break;
		case 'n':
			s_nofork = true;
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
}


static void
daemonize(void)
{
	int r;
	struct rlimit rlim;
	int fdmax;

	/* ignore signals associated with job control */
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);

	if ((r = fork()) == -1)
		CE("failed to fork, exiting!");
	else if (r > 0)
		_exit(EXIT_SUCCESS);

	if (setsid() == -1) /* create a new session, become session leader */
		CE("setsid");

	if (getrlimit(RLIMIT_NOFILE, &rlim) != 0) {
		E("getrlimit() failed when asked for RLIMIT_NOFILE");
		fdmax = 2; /* assume only std{in,out,err} are open */
	} else
		fdmax = rlim.rlim_max;

	/* close all file descriptors */
	while(fdmax)
		close(fdmax--);

	//umask(0);
	if (chdir("/") != 0)
		E("couldn't chdir to /");

	if ((r = fork()) == -1)
		CE("2nd fork failed, exiting!");
	else if (r > 0)
		_exit(EXIT_SUCCESS);
}


static void
init(int *argc, char ***argv)
{
	process_args(argc, argv);

	if (!s_nofork) {
		I("forking.. subsequent logging will go to syslog");
		dbg_syslog((*argv[0]), LOG_USER);
		daemonize();
		D("daemonized");
	}
		

	if (!s_dev)
		s_dev = strdup(DEF_DEV);

	if (!s_baud)
		s_baud = DEF_BAUD;

	s_scfd = sc_init(s_dev, s_baud);

	D("initialized sercomm (dev: '%s', baud: %d)", s_dev, s_baud);
	
	s_sck = addr_bind_socket_p("localhost", 34321, NULL, 0, 0, 0);
	if (s_sck == -1)
		CE("addr_bind_socket_p");
	
	if (listen(s_sck, 16) == -1)
		D("listen");
}


static void
usage(FILE *str, const char *a0, int ec)
{
	#define H(STR) fputs(STR "\n", str)
	H("==============");
	H("== sercommd ==");
	H("==============");
	fprintf(str, "usage: %s [-envqch -d <dev> -b <baud>]\n", a0);
	H("");
	H("\t-d <str>: Specify serial device to use (def: /dev/dtyU0)");
	H("\t-b <int>: Baud rate to use (def: 38400)");
	H("\t-n: Don't fork");
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

	I("initializad");

	int csck;

	while ((csck = accept(s_sck, NULL, NULL)) != -1) {
		D("accepted");
		handle(csck);
		close(csck);
	}

	CE("accept");
}
