/* eval.h - (C) 2014, Timo Buhrmester
* immectl - ...
* See README for contact-, COPYING for license information. */

#ifndef EVAL_H
#define EVAL_H 1

#include <libsrsbsns/strmap.h>

typedef bool (*ev_fn)(int ac, char **av);

bool ev_init(void);
bool ev_dispatch(int ac, char **av);

#endif /* EVAL_H */
