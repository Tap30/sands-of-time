#define _GNU_SOURCE

#include <signal.h>
#include <dlfcn.h>

#include <sys/types.h>
#include <time.h>
#include <sys/timeb.h>
#include <sys/time.h>

#include <mach/mach.h>
#include <mach/mach_time.h>

void init();

static time_t       (*real_time)            (time_t *);
static int          (*real_ftime)           (struct timeb *);
static int          (*real_gettimeofday)    (struct timeval *, void *);
static int          (*real_clock_gettime)   (clockid_t clk_id, struct timespec *tp);
static int          (*real_sigaction)       (int signum, const struct sigaction *act, struct sigaction *oldact);

void                (*real_usr_signal_handler)         (int);
static int          initialized = 0;
static int          specifiedtime = 1600000;

int usr_signal_handler(int signum) {
    printf("mia handles request %d\n", signum);
    if (real_usr_signal_handler) {
        (*real_usr_signal_handler)(signum);
    }
    printf("mia done request %d\n", signum);
    return 0;
}

uint64_t mach_absolute_time() {
    return 150000000;
}

int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact) {
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

void submit_usr_handler() {
    /* Start listening on sig usr2 */
    struct sigaction new_action;
    new_action.sa_handler = usr_signal_handler;
    sigemptyset (&new_action.sa_mask);
    new_action.sa_flags = 0;

    (*real_sigaction)(SIGUSR2, &new_action, NULL);
    // sigaction(SIGUSR2, usr_signal_handler);
}

void init() {
    real_sigaction = dlsym(RTLD_NEXT, "sigaction");
    real_gettimeofday = dlsym(RTLD_NEXT, "gettimeofday");
    submit_usr_handler();
    initialized = 1;
}

int clock_gettime(clockid_t clk_id, struct timespec *tp) {
    printf("clock_gettime \n");
    if (!initialized) {
        init();
    }
    // TODO call real time and do timeshift
    tp->tv_nsec = 0;
    tp->tv_sec = specifiedtime;
    return 0;
}

int gettimeofday(struct timeval *tp, void *tz) {
    printf("gettimeofday :D\n");
    if (!initialized) {
        init();
    }
    // TODO call real time and do timeshift
    tp->tv_sec = specifiedtime;
    tp->tv_usec = 0;
    //(*real_gettimeofday)(tp, tz);
    return 0;
}

time_t time(time_t *tloc) {
    if (!initialized) {
        init();
    }
    if (tloc != NULL) {
        *tloc = specifiedtime;
    }
    return specifiedtime;
}

int ftime(struct timeb * s) {
    if (!initialized) {
        init();
    }
    s->time = specifiedtime;
    s->millitm = 0;

    return 0;
}