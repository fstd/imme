/* eval.c - (C) 2014, Timo Buhrmester
 * immectl - ...
 * See README for contact-, COPYING for license information. */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#include "dbglog.h"

#include "sercomm.h"
#include "eval.h"

#define NOOPTS(CMDNAME)                                    \
	for(int ch; (ch = getopt(ac, av, "h")) != -1;) {   \
		switch (ch) {                              \
		case 'h':                                  \
			usage_##CMDNAME(stdout, #CMDNAME,   \
			    EXIT_SUCCESS);                 \
			break;                             \
		case '?':                                  \
		default:                                   \
			usage_##CMDNAME(stderr, #CMDNAME,   \
			    EXIT_FAILURE);                 \
		}                                          \
	}                                                  \
                                                           \
	ac -= optind;                                      \
	av += optind;                                      \
                                                           \
	optreset = 1;                                      \
	optind = 1;

static smap_t s_dspmap;


static bool ev_DBGDUMP(int ac, char **av);
static bool ev_PING(int ac, char **av);
static bool ev_MSTRST(int ac, char **av);
static bool ev_REINIT(int ac, char **av);
static bool ev_NOP(int ac, char **av);
static bool ev_STATUS(int ac, char **av);
static bool ev_CHIPID(int ac, char **av);
static bool ev_GETPC(int ac, char **av);
static bool ev_HALT(int ac, char **av);
static bool ev_RESUME(int ac, char **av);
static bool ev_CHIPERASE(int ac, char **av);
static bool ev_RCFG(int ac, char **av);
static bool ev_WCFG(int ac, char **av);
static bool ev_REPLINSTR(int ac, char **av);
static bool ev_RUNINSTR(int ac, char **av);
static bool ev_NEXTINSTR(int ac, char **av);
static bool ev_BRK(int ac, char **av);
static bool ev_WRAW(int ac, char **av);
static bool ev_RRAW(int ac, char **av);
static bool ev_DELAY(int ac, char **av);
static bool ev_WAITHALT(int ac, char **av);

static void usage_DBGDUMP(FILE *str, const char *a0, int ec);
static void usage_PING(FILE *str, const char *a0, int ec);
static void usage_MSTRST(FILE *str, const char *a0, int ec);
static void usage_REINIT(FILE *str, const char *a0, int ec);
static void usage_NOP(FILE *str, const char *a0, int ec);
static void usage_STATUS(FILE *str, const char *a0, int ec);
static void usage_CHIPID(FILE *str, const char *a0, int ec);
static void usage_GETPC(FILE *str, const char *a0, int ec);
static void usage_HALT(FILE *str, const char *a0, int ec);
static void usage_RESUME(FILE *str, const char *a0, int ec);
static void usage_CHIPERASE(FILE *str, const char *a0, int ec);
static void usage_RCFG(FILE *str, const char *a0, int ec);
static void usage_WCFG(FILE *str, const char *a0, int ec);
static void usage_REPLINSTR(FILE *str, const char *a0, int ec);
static void usage_RUNINSTR(FILE *str, const char *a0, int ec);
static void usage_NEXTINSTR(FILE *str, const char *a0, int ec);
static void usage_BRK(FILE *str, const char *a0, int ec);
static void usage_WRAW(FILE *str, const char *a0, int ec);
static void usage_RRAW(FILE *str, const char *a0, int ec);
static void usage_DELAY(FILE *str, const char *a0, int ec);
static void usage_WAITHALT(FILE *str, const char *a0, int ec);

bool
ev_init(void)
{
	s_dspmap = smap_init(16);

	if (!s_dspmap)
		return false;

	smap_put(s_dspmap, "DBGDUMP", ev_DBGDUMP);
	smap_put(s_dspmap, "PING", ev_PING);
	smap_put(s_dspmap, "MSTRST", ev_MSTRST);
	smap_put(s_dspmap, "REINIT", ev_REINIT);
	smap_put(s_dspmap, "NOP", ev_NOP);
	smap_put(s_dspmap, "STATUS", ev_STATUS);
	smap_put(s_dspmap, "CHIPID", ev_CHIPID);
	smap_put(s_dspmap, "GETPC", ev_GETPC);
	smap_put(s_dspmap, "HALT", ev_HALT);
	smap_put(s_dspmap, "RESUME", ev_RESUME);
	smap_put(s_dspmap, "CHIPERASE", ev_CHIPERASE);
	smap_put(s_dspmap, "RCFG", ev_RCFG);
	smap_put(s_dspmap, "WCFG", ev_WCFG);
	smap_put(s_dspmap, "REPLINSTR", ev_REPLINSTR);
	smap_put(s_dspmap, "RUNINSTR", ev_RUNINSTR);
	smap_put(s_dspmap, "NEXTINSTR", ev_NEXTINSTR);
	smap_put(s_dspmap, "BRK", ev_BRK);
	smap_put(s_dspmap, "WRAW", ev_WRAW);
	smap_put(s_dspmap, "RRAW", ev_RRAW);
	smap_put(s_dspmap, "DELAY", ev_DELAY);
	smap_put(s_dspmap, "WAITHALT", ev_WAITHALT);

	return true;
}

bool
ev_dispatch(int ac, char **av)
{
	ev_fn fnc = smap_get(s_dspmap, av[0]);
	if (!fnc) {
		E("illegal dispatch: '%s'", av[0]);
		return false;
	}
	I("dispatching %s (%d args)", av[0], ac);

	return fnc(ac, av);
}

static bool
ev_DBGDUMP(int ac, char **av)
{
	if (ac > 1)
		usage_DBGDUMP(stderr, av[0], EXIT_FAILURE);

	sc_put('!');
	uint8_t b1 = sc_get();
	printf("%#2.2hhx %#2.2hhx\n", b1, sc_get());
	return true;
}

static bool
ev_PING(int ac, char **av)
{
	if (ac > 1)
		usage_PING(stderr, av[0], EXIT_FAILURE);

	sc_put('?');
	return sc_get() == '!';
}

// reset the atmega16, not the target!
static bool
ev_MSTRST(int ac, char **av)
{
	if (ac > 1)
		usage_MSTRST(stderr, av[0], EXIT_FAILURE);

	sc_put('R');
	usleep(500000); //give it a (generous) while to come back
	return true;
}

static bool
ev_REINIT(int ac, char **av)
{
	if (ac > 1)
		usage_REINIT(stderr, av[0], EXIT_FAILURE);

	sc_put('D');
	if (sc_get() != 'D')
		return false;
	/* after a reinit, the LOCK bit is set, for some reason, even if
	 * the interface isn't actually locked.
	 * issuing a NOP ``fixes'' this, and does not affect PC */

	sc_put('I');
	sc_put(1);
	sc_put(0); //NOP
	sc_get(); //discard
	return true;
}

static bool
ev_NOP(int ac, char **av)
{
	if (ac > 1)
		usage_NOP(stderr, av[0], EXIT_FAILURE);

	return true;
}

static bool
ev_STATUS(int ac, char **av)
{
	if (ac > 1)
		usage_STATUS(stderr, av[0], EXIT_FAILURE);

	sc_put('s');
	printf("%#2.2hhx\n", sc_get());
	return true;
}

static bool
ev_CHIPID(int ac, char **av)
{
	if (ac > 1)
		usage_CHIPID(stderr, av[0], EXIT_FAILURE);

	sc_put('i');
	uint8_t b1 = sc_get();
	printf("%#2.2hhx %#2.2hhx\n", b1, sc_get());
	return true;
}

static bool
ev_GETPC(int ac, char **av)
{
	if (ac > 1)
		usage_GETPC(stderr, av[0], EXIT_FAILURE);

	sc_put('p');
	uint8_t b1 = sc_get();
	printf("%#2.2hhx %#2.2hhx\n", b1, sc_get());
	return true;
}

static bool
ev_HALT(int ac, char **av)
{
	if (ac > 1)
		usage_HALT(stderr, av[0], EXIT_FAILURE);

	sc_put('H');
	return sc_get() == 'H';
}

static bool
ev_RESUME(int ac, char **av)
{
	if (ac > 1)
		usage_RESUME(stderr, av[0], EXIT_FAILURE);

	sc_put('h');
	return sc_get() == 'h';
}

static bool
ev_CHIPERASE(int ac, char **av)
{
	if (ac > 1)
		usage_CHIPERASE(stderr, av[0], EXIT_FAILURE);

	sc_put('E');
	return sc_get() == 'E';
}

static bool
ev_RCFG(int ac, char **av)
{
	if (ac > 1)
		usage_RCFG(stderr, av[0], EXIT_FAILURE);

	sc_put('c');
	printf("%#2.2hhx\n", sc_get());
	return true;
}

static bool
ev_WCFG(int ac, char **av)
{
	const char *a0 = av[0];
	NOOPTS(WCFG)

	if (ac != 1)
		usage_WCFG(stderr, a0, EXIT_FAILURE);

	uint8_t b = (uint8_t)strtoul(av[0], NULL, 0);
	D("writing cfg byte %#2.2hhx", b);

	sc_put('C');
	sc_put(b);

	return sc_get() == b;
}

static bool
ev_REPLINSTR(int ac, char **av)
{
	const char *a0 = av[0];
	NOOPTS(REPLINSTR)

	if (ac < 1 || ac > 3)
		usage_REPLINSTR(stderr, a0, EXIT_FAILURE);

	sc_put('N');
	sc_put(ac);
	while(ac--) {
		uint8_t b = (uint8_t)strtoul(*av++, NULL, 0);
		D("writing instr byte %#2.2hhx", b);
		sc_put(b);
	}

	printf("%#2.2hhx\n", sc_get());
	return true;
}

static bool
ev_RUNINSTR(int ac, char **av)
{
	const char *a0 = av[0];
	NOOPTS(RUNINSTR)

	if (ac < 1 || ac > 3) {
		N("huh");
		usage_RUNINSTR(stderr, a0, EXIT_FAILURE);
	} else
		N("x");

	sc_put('I');
	sc_put(ac);
	while(ac--) {
		uint8_t b = (uint8_t)strtoul(*av++, NULL, 0);
		D("writing instr byte %#2.2hhx", b);
		sc_put(b);
	}

	printf("%#2.2hhx\n", sc_get());
	return true;
	sc_put('I');
}

static bool
ev_NEXTINSTR(int ac, char **av)
{
	if (ac > 1)
		usage_NEXTINSTR(stderr, av[0], EXIT_FAILURE);

	sc_put('n');
	printf("%#2.2hhx\n", sc_get());
	return true;
}

static bool
ev_BRK(int ac, char **av)
{
	const char *a0 = av[0];
	NOOPTS(BRK)

	if (ac != 3)
		usage_BRK(stderr, a0, EXIT_FAILURE);

	sc_put('B');
	while(ac--) {
		uint8_t b = (uint8_t)strtoul(*av++, NULL, 0);
		D("writing brk byte %#2.2hhx", b);
		sc_put(b);
	}

	printf("%#2.2hhx\n", sc_get());
	return true;
}

static bool
ev_WRAW(int ac, char **av)
{
	const char *a0 = av[0];
	NOOPTS(WRAW)

	if (ac != 1)
		usage_WRAW(stderr, a0, EXIT_FAILURE);

	sc_put('>');
	uint8_t b = (uint8_t)strtoul(*av++, NULL, 0);
	sc_put(b);
	return sc_get() == b;
}

static bool
ev_RRAW(int ac, char **av)
{
	if (ac > 1)
		usage_RRAW(stderr, av[0], EXIT_FAILURE);

	sc_put('<');
	printf("%#02hhx\n", sc_get());
	return true;
}

static bool
ev_DELAY(int ac, char **av)
{
	const char *a0 = av[0];
	NOOPTS(DELAY)

	if (ac != 1)
		usage_DELAY(stderr, a0, EXIT_FAILURE);

	unsigned int d = (unsigned int)strtoul(*av++, NULL, 0);
	unsigned int s = d / 1000000U;
	unsigned int us = d % 1000000U;

	if (s)
		sleep(s);
	
	if (us)
		usleep(us);

	return true;
}


static bool
ev_WAITHALT(int ac, char **av)
{
	if (ac > 1)
		usage_WAITHALT(stderr, av[0], EXIT_FAILURE);

	N("waiting for CPU to halt...");
	do {
		usleep(50000); //generous..
		sc_put('s');
	} while (!(sc_get() & 0x20));
	N("CPU halted...");

	return true;
}

#define USG(STR) fputs(STR "\n", str)

static void
usage_DBGDUMP(FILE *str, const char *a0, int ec)
{
	fprintf(str, "usage: "PACKAGE_NAME" [...] %s [-h]\n", a0);
	USG("");
	USG("\t(no arguments)");
	USG("\t(no output)");
	exit(ec);
}

static void
usage_PING(FILE *str, const char *a0, int ec)
{
	fprintf(str, "usage: "PACKAGE_NAME" [...] %s [-h]\n", a0);
	USG("");
	USG("\t(no arguments)");
	USG("\t(no output, but we exit with EXIT_FAILURE unless we");
	USG("\t  get a proper pong");
	exit(ec);
}

static void
usage_MSTRST(FILE *str, const char *a0, int ec)
{
	fprintf(str, "usage: "PACKAGE_NAME" [...] %s [-h]\n", a0);
	USG("");
	USG("\t(no arguments)");
	USG("\t(no output)");
	exit(ec);
}

static void
usage_REINIT(FILE *str, const char *a0, int ec)
{
	fprintf(str, "usage: "PACKAGE_NAME" [...] %s [-h]\n", a0);
	USG("");
	USG("\t(no arguments)");
	USG("\t(no output)");
	exit(ec);
}

static void
usage_NOP(FILE *str, const char *a0, int ec)
{
	fprintf(str, "usage: "PACKAGE_NAME" [...] %s [-h]\n", a0);
	USG("");
	USG("\t(no arguments)");
	USG("\t(no output)");
	exit(ec);
}

static void
usage_STATUS(FILE *str, const char *a0, int ec)
{
	fprintf(str, "usage: "PACKAGE_NAME" [...] %s [-h]\n", a0);
	USG("");
	USG("\t(no arguments)");
	USG("\toutputs the status byte");
	exit(ec);
}

static void
usage_CHIPID(FILE *str, const char *a0, int ec)
{
	fprintf(str, "usage: "PACKAGE_NAME" [...] %s [-h]\n", a0);
	USG("");
	USG("\t(no arguments)");
	exit(ec);
}

static void
usage_GETPC(FILE *str, const char *a0, int ec)
{
	fprintf(str, "usage: "PACKAGE_NAME" [...] %s [-h]\n", a0);
	USG("");
	USG("\t(no arguments)");
	exit(ec);
}

static void
usage_HALT(FILE *str, const char *a0, int ec)
{
	fprintf(str, "usage: "PACKAGE_NAME" [...] %s [-h]\n", a0);
	USG("");
	USG("\t(no arguments)");
	USG("\t(no output)");
	exit(ec);
}

static void
usage_RESUME(FILE *str, const char *a0, int ec)
{
	fprintf(str, "usage: "PACKAGE_NAME" [...] %s [-h]\n", a0);
	USG("");
	USG("\t(no arguments)");
	USG("\t(no output)");
	exit(ec);
}

static void
usage_CHIPERASE(FILE *str, const char *a0, int ec)
{
	fprintf(str, "usage: "PACKAGE_NAME" [...] %s [-h]\n", a0);
	USG("");
	USG("\t(no arguments)");
	USG("\t(no output)");
	exit(ec);
}

static void
usage_RCFG(FILE *str, const char *a0, int ec)
{
	fprintf(str, "usage: "PACKAGE_NAME" [...] %s [-h]\n", a0);
	USG("");
	USG("\t(no arguments)");
	exit(ec);
}

static void
usage_WCFG(FILE *str, const char *a0, int ec)
{
	fprintf(str, "usage: "PACKAGE_NAME" [...] %s [-h] <cfg byte>\n", a0);
	USG("");
	USG("\t<cfg byte> can be specified in either decimal (no prefix),");
	USG("\t octal (0-prefix) or hex (0x-prefix)");
	USG("");
	USG("\t(no output)");
	exit(ec);
}

static void
usage_REPLINSTR(FILE *str, const char *a0, int ec)
{
	fprintf(str, "usage: "PACKAGE_NAME" [...] %s [-h] <b1> [<b2> [<b3>]]\n", a0);
	USG("");
	USG("\treplace current instruction with b1..b3, advance PC");
	USG("");
	USG("\toutput is whatever the device responds (not yet sure if meaningful)");
	exit(ec);
}

static void
usage_RUNINSTR(FILE *str, const char *a0, int ec)
{
	fprintf(str, "usage: "PACKAGE_NAME" [...] %s [-h] <b1> [<b2> [<b3>]]\n", a0);
	USG("");
	USG("\texecute given instruction, do /not/ advance PC");
	USG("\toutput is whatever the device responds (not yet sure if meaningful)");
	exit(ec);
}

static void
usage_NEXTINSTR(FILE *str, const char *a0, int ec)
{
	fprintf(str, "usage: "PACKAGE_NAME" [...] %s [-h]\n", a0);
	USG("");
	USG("\t(no arguments)");
	USG("\toutput is whatever the device responds (not yet sure if meaningful)");
	exit(ec);
}

static void
usage_BRK(FILE *str, const char *a0, int ec)
{
	fprintf(str, "usage: "PACKAGE_NAME" [...] %s [-h] <b1> <b2> <b3>\n", a0);
	USG("");
	USG("\tset hardware breakpoint, defined by three bytes.");
	USG("\toutput is whatever the device responds (not yet sure if meaningful)");
	exit(ec);
}

static void
usage_WRAW(FILE *str, const char *a0, int ec)
{
	fprintf(str, "usage: "PACKAGE_NAME" [...] %s [-h] <raw byte>\n", a0);
	USG("");
	USG("\twrite a byte to the target as-is.  be careful.");
	USG("\t(no output)");
	exit(ec);
}

static void
usage_RRAW(FILE *str, const char *a0, int ec)
{
	fprintf(str, "usage: "PACKAGE_NAME" [...] %s [-h]\n", a0);
	USG("");
	USG("\t(no arguments)");
	USG("\toutput is the byte read");
	exit(ec);
}

static void
usage_DELAY(FILE *str, const char *a0, int ec)
{
	fprintf(str, "usage: "PACKAGE_NAME" [...] %s [-h] <microseconds>\n", a0);
	USG("");
	USG("\tintroduce an artificial delay.  don't expect accuracy.");
	exit(ec);
}

static void
usage_WAITHALT(FILE *str, const char *a0, int ec)
{
	fprintf(str, "usage: "PACKAGE_NAME" [...] %s [-h]\n", a0);
	USG("");
	USG("\twait until the halted-bit is set in the debug status byte");
	exit(ec);
}


#undef USG

