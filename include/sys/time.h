#ifndef _SYS_TIME_H
#define _SYS_TIME_H
#ifdef __cplusplus
extern "C" {
#endif

/* All symbols from select.h except pselect are required anyway... */
#include <sys/select.h>

#define __NEED_time_t
#define __NEED_suseconds_t
#define __NEED_struct_timeval

#include <bits/alltypes.h>



int gettimeofday (struct timeval *, void *);

#if defined(_XOPEN_SOURCE) || defined(_GNU_SOURCE)

#define ITIMER_REAL    0
#define ITIMER_VIRTUAL 1
#define ITIMER_PROF    2

struct itimerval
{
	struct timeval it_interval;
	struct timeval it_value;
};

int getitimer (int, struct itimerval *);
int setitimer (int, const struct itimerval *, struct itimerval *);
int utimes (const char *, const struct timeval [2]);

#endif

#ifdef _GNU_SOURCE
int settimeofday (const struct timeval *, void *);
int adjtime (const struct timeval *, struct timeval *);
struct timezone {
	int tz_minuteswest;
	int tz_dsttime;
};
#define timerisset(t) ((t)->tv_sec || (t)->tv_usec)
#define timerclear(t) ((t)->tv_sec = (t)->tv_usec = 0)
#define timercmp(s,t,op) ((s)->tv_sec == (t)->tv_sec ? \
	(s)->tv_usec op (t)->tv_usec : (s)->tv_sec op (t)->tv_sec)
#define timeradd(s,t,a) ( (a)->tv_sec = (s)->tv_sec + (t)->tv_sec, \
	((a)->tv_usec = (s)->tv_usec + (t)->tv_usec) >= 1000000 && \
	((a)->tv_usec -= 1000000, (a)->tv_sec++) )
#define timersub(s,t,a) ( (a)->tv_sec = (s)->tv_sec - (t)->tv_sec, \
	((a)->tv_usec = (s)->tv_usec - (t)->tv_usec) < 0 && \
	((a)->tv_usec += 1000000, (a)->tv_sec--) )
#endif

#ifdef __cplusplus
}
#endif
#endif
