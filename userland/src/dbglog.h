/* dbglog.h - (C) 2014, Timo Buhrmester
* immectl - ...
* See README for contact-, COPYING for license information. */

#ifndef DBGLOG_H
#define DBGLOG_H 1

#include <stdbool.h>
#include <errno.h>

#include <syslog.h>

//[DINWE](): log with Debug, Info, Notice, Warn, Error severity.
//[DINWE]E(): similar, but also append ``errno'' message
 
// ----- logging interface -----

#define D(F,A...)                                               \
    dbg_log(LOG_DEBUG,-1,__FILE__,__LINE__,__func__,F,##A)

#define DE(F,A...)                                              \
    dbg_log(LOG_DEBUG,errno,__FILE__,__LINE__,__func__,F,##A)

#define I(F,A...)                                               \
    dbg_log(LOG_INFO,-1,__FILE__,__LINE__,__func__,F,##A)

#define IE(F,A...)                                              \
    dbg_log(LOG_INFO,errno,__FILE__,__LINE__,__func__,F,##A)

#define N(F,A...)                                               \
    dbg_log(LOG_NOTICE,-1,__FILE__,__LINE__,__func__,F,##A)

#define NE(F,A...)                                              \
    dbg_log(LOG_NOTICE,errno,__FILE__,__LINE__,__func__,F,##A)

#define W(F,A...)                                               \
    dbg_log(LOG_WARNING,-1,__FILE__,__LINE__,__func__,F,##A)

#define WE(F,A...)                                              \
    dbg_log(LOG_WARNING,errno,__FILE__,__LINE__,__func__,F,##A)

#define E(F,A...)                                               \
    dbg_log(LOG_ERR,-1,__FILE__,__LINE__,__func__,F,##A)

#define EE(F,A...)                                              \
    dbg_log(LOG_ERR,errno,__FILE__,__LINE__,__func__,F,##A)

#define C(F,A...) do{                                           \
    dbg_log(LOG_CRIT,-1,__FILE__,__LINE__,__func__,F,##A);      \
    exit(EXIT_FAILURE); }while(0)

#define CE(F,A...) do{                                          \
    dbg_log(LOG_CRIT,errno,__FILE__,__LINE__,__func__,F,##A);   \
    exit(EXIT_FAILURE); }while(0)

// ----- logger control interface -----

void dbg_stderr(void);
void dbg_syslog(const char *ident, int facility);

void dbg_setlvl(int lvl);
int dbg_getlvl(void);

void dbg_setfancy(bool fancy);
bool dbg_getfancy(void);


// ----- backend -----
void dbg_log(int lvl, int errn, const char *file, int line,
    const char *func, const char *fmt, ...);

#endif /* DBGLOG_H */
