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

#define MEGA 1000000
#define KILO 1000


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

static double			fake_time_alpha = 1;
static struct timespec		fake_time_beta = {0, 0};



int get_real_time(struct timespec *time)
{
#ifdef __APPLE__
	// TODO @rajabzz
	return 0;
#else
	return (*real_clock_gettime)(CLOCK_MONOTONIC, time);
#endif // __APPLE__	
}

int get_fake_time(struct timespec *time)
{
	int status;
	struct timespec real;

	status = get_real_time(&real);

	if (status < 0) {
		return status;
	}

	time->tv_sec = fake_time_alpha * real.tv_sec + fake_time_beta.tv_sec;
	time->tv_nsec = fake_time_alpha * real.tv_nsec + fake_time_beta.tv_nsec;

	time->tv_sec += time->tv_nsec / MEGA;
	time->tv_nsec %= MEGA;

	return 0;
}

void usr_signal_handler(int signum)
{
	printf("mia handles request %d\n", signum);
	if (real_usr_signal_handler) {
		(*real_usr_signal_handler)(signum);
	}
	// handle socket?
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

	if(clk_id != CLOCK_MONOTONIC) {
		// TODO implement something here
	}
	
	return get_fake_time(tp);
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
	struct timespec real_tp;
	int status = get_fake_time(&real_tp);
	
	tp->tv_sec = real_tp.tv_sec;
	tp->tv_usec = real_tp.tv_nsec / KILO;

	if(tz != NULL) {
		// TODO
	}

	return status;
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
