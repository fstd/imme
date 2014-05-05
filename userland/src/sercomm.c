/* sercomm.c - (C) 2014, Timo Buhrmester
 * immectl - ...
 * See README for contact-, COPYING for license information. */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <libsrsbsns/addr.h>
#include "dbglog.h"

#include "sercomm.h"

static int s_fd = -1;
static int s_wc = 0;
static int s_rc = 0;

static void eat_buffer(void);

void sc_cleanup(void);

int
sc_init_tcp(const char *host, uint16_t port)
{
	int fd = addr_connect_socket_p(host, port, NULL, NULL, 0, 0);

	if (fd == -1)
		CE("addr_connect_socket_p");

	atexit(sc_cleanup);
	
	return s_fd = fd;
}

int
sc_init(const char *device, int baud)
{
	struct termios	cntrl;
	int fd;

	fd = open(device, O_RDWR);

	if (fd == -1)
		CE("open '%s'", device);

	atexit(sc_cleanup);

	//if (flock(fd, LOCK_EX|LOCK_NB) != 0)
		//err(1, "flock");

	//if (ioctl(fd, TIOCEXCL, 0) != 0)
		//err(1, "ioctl TIOEXCL");

	if (tcgetattr(fd, &cntrl) != 0)
		CE("tcgetattr");

	if (cfsetospeed(&cntrl, baud) != 0)
		CE("cfsetospeed");

	if (cfsetispeed(&cntrl, baud) != 0)
		CE("cfsetispeed");

	cntrl.c_cflag &= ~(CSIZE|PARENB|HUPCL);
	cntrl.c_cflag |= CS8;
	cntrl.c_cflag |= CLOCAL;
	cntrl.c_iflag &= ~(ISTRIP|ICRNL);
	cntrl.c_oflag &= ~OPOST;
	cntrl.c_lflag &= ~(ICANON|ISIG|IEXTEN|ECHO);
	cntrl.c_cc[VMIN] = 1;
	cntrl.c_cc[VTIME] = 0;

	if (tcsetattr(fd, TCSADRAIN, &cntrl) != 0)
		CE("tcsetattr");

	return s_fd = fd;
}

void
sc_dumpstats(void)
{
	I("read: %d, wrote: %d", s_rc, s_wc);
}

void
sc_resetstats(void)
{
	s_rc = s_wc = 0;
}

uint8_t
sc_get(void)
{
	uint8_t c = 0;
	D("reading from %d...", s_fd);
	ssize_t r = read(s_fd, &c, 1);
	D("read: %zd: %#2.2hhx ('%c')", r, c, c);
	if (r != 1) {
		CE("serial comm failed on read (%zd)", r);
		return -1;
	}
	s_rc++;
	return c;
}

void
sc_put(uint8_t c)
{
	D("writing to %d: %#2.2hhx ('%c')", s_fd, c, c);
	ssize_t r = write(s_fd, &c, 1);
	D("write: %zd", r);
	if (r == -1)
		CE("serial comm failed on write");
	s_wc++;
}

static void
eat_buffer(void)
{
	D("eating buffer");
	int seenempty = 0;
	bool fresh = true;
	for(;;) {
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(s_fd,&fds);
		struct timeval to;
		to.tv_sec = 0;
		to.tv_usec = 0;

		int r = select(s_fd+1, &fds, 0, 0, &to);
		if (r == 0) {
			if (fresh) {
				D("buffer apparently empty");
				break;
			}
			fresh = false;

			if (!seenempty) {
				usleep(999999);
				seenempty = 1;
				continue;
			} else {
				D("buffer apparently empty");
				break;
			}
		} else if (r == -1) {
			CE("select failed");
		} else {
			fresh = false;
			if (FD_ISSET(s_fd, &fds)) {
				seenempty = 0;
				D("discarded a byte");
				sc_get(); //discard
			} else
				C("wat?");
		}
	}
}

void
sc_cleanup(void)
{
	//if (flock(s_fd, LOCK_UN) != 0)
		//warn("flock");
	close(s_fd);
}

