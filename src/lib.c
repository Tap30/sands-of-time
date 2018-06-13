#define _GNU_SOURCE

#include <signal.h>
#include <dlfcn.h>

#include <sys/types.h>
#include <time.h>
#include <sys/timeb.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <inttypes.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/mach_time.h>
#endif

#define GIGA 1000000000
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

static double				fake_time_alpha = 1;
static struct timespec		fake_time_beta = {0, 0};
static struct timespec		fake_time_t0 = {0, 0};
static time_t				fake_time_delta = 0;


static int	    	fd = 0;
static FILE*	    fd_stream = NULL;



void bend_time(struct timespec *result, struct timespec *t0, struct timespec *t1, double alpha)
{
	double seconds_reminder = alpha * (double) t1->tv_sec;
	time_t seconds = seconds_reminder;
	seconds_reminder -= seconds;

	result->tv_sec = t0->tv_sec + seconds;
	result->tv_nsec = t0->tv_nsec + alpha * t1->tv_nsec + seconds_reminder * GIGA;

	if (result->tv_nsec > 0) {
		result->tv_sec += result->tv_nsec / GIGA;
		result->tv_nsec %= GIGA;
	} else {
		int rem = (-result->tv_nsec) / GIGA + 1;
		result->tv_sec -= rem;
		result->tv_nsec += rem * GIGA;
	}
}

int get_real_time(struct timespec *time)
{
#ifdef __APPLE__
	// TODO @rajabzz
	return 0;
#else
	int status = (*real_clock_gettime)(CLOCK_MONOTONIC, time);

	time->tv_sec += fake_time_delta;

	return status;
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

	// time = t' + (t-t0) * alpha
	// t => real
	// t'=> beta
	// t0 => t0

	real.tv_sec -= fake_time_t0.tv_sec;
	real.tv_nsec -= fake_time_t0.tv_nsec;

	bend_time(time, &fake_time_beta, &real, fake_time_alpha);
	
	return 0;
}

void usr_signal_handler(int signum)
{
	printf("mia handles request %d\n", signum);
	if (real_usr_signal_handler) {
		(*real_usr_signal_handler)(signum);
	}

	// let's read socket

	fscanf(fd_stream, "%lf:%ld:%ld", &fake_time_alpha, &(fake_time_beta.tv_sec), &(fake_time_beta.tv_nsec));
	get_real_time(&fake_time_t0);
	
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

	fd= socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd< 0) {
		perror("opening stream socket");
		exit(1);
	}


#ifndef __APPLE__
	struct timespec tp;
	struct timeval tv;
	(*real_clock_gettime)(CLOCK_MONOTONIC, &tp);
	(*real_gettimeofday)(&tv, NULL);

	fake_time_delta = tv.tv_sec - tp.tv_sec;
#endif
	
	struct sockaddr_un server;

	server.sun_family = AF_UNIX;
	strcpy(server.sun_path, getenv("SANDS_SUN"));
	
	if (connect(fd, (struct sockaddr *) &server, sizeof(struct sockaddr_un)) < 0) {
		close(fd);
		perror("connecting stream socket");
		exit(1);
	}

	fd_stream = fdopen(fd, "r");

	if (fd_stream == NULL) {
		close(fd);
		perror("connecting stream socket");
		exit(1);
	}


	initialized = 1;
}

int clock_gettime(clockid_t clk_id, struct timespec *tp) 
{
	if (!initialized) {
		init();
	}

	if (clk_id != CLOCK_MONOTONIC) {
		// TODO implement something here
	}
	
	return get_fake_time(tp);
}

uint64_t mach_absolute_time()
{
	// TODO @rajabzz
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


	if (tz != NULL) {
		// TODO
	}

	return status;
}

time_t time(time_t *tloc)
{
	if (!initialized) {
		init();
	}

	struct timespec fake_time;
	int status = get_fake_time(&fake_time);
	if (tloc != NULL) {
		(*tloc) = fake_time.tv_sec;
	}

	return status;
}

int ftime(struct timeb * s)
{
	if (!initialized) {
		init();
	}

	struct timespec fake_time;
	int status = get_fake_time(&fake_time);
	s->time = fake_time.tv_sec;
	s->millitm = fake_time.tv_nsec / MEGA;

	return status;
}
