#define _GNU_SOURCE

#include <signal.h>
#include <dlfcn.h>

#include <sys/types.h>
#include <time.h>
#include <sys/timeb.h>
#include <sys/time.h>
#include <inttypes.h>

#include <stdio.h>

#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/mach_time.h>
#endif


struct fake_time {
	int seconds; // posix time, from epoch
	int microseconds;
};

void init();

static time_t       (*real_time)                (time_t *);
static int          (*real_ftime)               (struct timeb *);
static int          (*real_gettimeofday)        (struct timeval *, void *);
static int          (*real_clock_gettime)       (clockid_t clk_id, struct timespec *tp);
static int          (*real_sigaction)           (int signum, const struct sigaction *act, struct sigaction *oldact);
static uint64_t     (*real_mach_absolute_time)  ();

void                (*real_usr_signal_handler)  (int);
static int          initialized = 0;
static int          specifiedtime = 1600000;

static int			fake_time_alpha = 0;
static int			fake_time_beta = 0;

int 				get_fake_time				(struct fake_time *time);


int get_fake_time(struct fake_time *time)
{
	time.seconds = 0;
	time.microseconds = 0;
	return 0;
}

void usr_signal_handler(int signum)
{
	printf("mia handles request %d\n", signum);
	if (real_usr_signal_handler) {
		(*real_usr_signal_handler)(signum);
	}
	printf("mia done request %d\n", signum);
}

int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact)
{
	if (!initialized) {
		init();
	}

	if (SIGUSR2 == signum && act != NULL) {
		real_usr_signal_handler = act->sa_handler;
		return 0;
	} else {
		return (*real_sigaction)(signum, act, oldact);
	}
}

void submit_usr_handler()
{
	/* Start listening on sig usr2 */
	struct sigaction new_action;
	new_action.sa_handler = usr_signal_handler;
	sigemptyset (&new_action.sa_mask);
	new_action.sa_flags = 0;
	(*real_sigaction)(SIGUSR2, &new_action, NULL);
}

void init()
{
	real_sigaction = dlsym(RTLD_NEXT, "sigaction");
	real_gettimeofday = dlsym(RTLD_NEXT, "gettimeofday");
	real_clock_gettime = dlsym(RTLD_NEXT, "clock_gettime");
	real_time = dlsym(RTLD_NEXT, "time");
	real_ftime = dlsym(RTLD_NEXT, "ftime");
	real_mach_absolute_time = dlsym(RTLD_NEXT, "mach_absolute_time");
	submit_usr_handler();
	initialized = 1;
}

int clock_gettime(clockid_t clk_id, struct timespec *tp) 
{
	if (!initialized) {
		init();
	}
	struct timespec real_tp;
	if ((*real_clock_gettime)(clk_id, &real_tp) != 0) {
		// TODO put sth here
		return 1;
	}

	// TODO  do timeshift
	tp->tv_nsec = 0;
	tp->tv_sec = specifiedtime;
	return 0;
}

uint64_t mach_absolute_time()
{
	uint64_t real_mach_time = real_mach_absolute_time();
	// TODO time shift
	return real_mach_time - 150000000;
}

int gettimeofday(struct timeval *__restrict tp, __timezone_ptr_t tz)
{
	if (!initialized) {
		init();
	}
	struct timeval real_tp;
	if ((*real_gettimeofday)(&real_tp, tz) != 0) {
		// TODO put sth here
		return 1;
	}
	tp->tv_sec = specifiedtime;
	tp->tv_usec = 0;
	return 0;
}

time_t time(time_t *tloc)
{
	if (!initialized) {
		init();
	}
	time_t real_tloc;
	if ((*real_time)(&real_tloc) != 0) {
		// TODO put sth here
		return 1;
	}
	if (tloc != NULL) {
		*tloc = specifiedtime;
	}
	return specifiedtime;
}

int ftime(struct timeb * s)
{
	if (!initialized) {
		init();
	}
	struct timeb real_s;
	if ((*real_ftime)(&real_s) != 0) {
		// TODO put sth here
		return 1;
	}
	s->time = specifiedtime;
	s->millitm = 0;

	return 0;
}