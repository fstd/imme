/* sercomm.h - (C) 2014, Timo Buhrmester
* immectl - ...
* See README for contact-, COPYING for license information. */

#ifndef SERCOMM_H
#define SERCOMM_H 1

#include <stdint.h>

int sc_init(const char *device, int baud);
int sc_init_tcp(const char *host, uint16_t port);
uint8_t sc_get(void);
void sc_put(uint8_t c);
void sc_dumpstats(void);
void sc_resetstats(void);

int mkbaud(int actualrate); //we need this for linux compat......

#endif /* SERCOMM_H */
