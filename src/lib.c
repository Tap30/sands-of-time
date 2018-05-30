#include <signal.h>

#include <sys/types.h>
#include <time.h>
#include <sys/timeb.h>

static time_t       (*real_time)            (time_t *);
static int          (*real_ftime)           (struct timeb *);
static int          (*real_gettimeofday)    (struct timeval *, void *);
static int          (*real_clock_gettime)   (clockid_t clk_id, struct timespec *tp);

static int initialized = 0;
static int specifiedtime = 1600000;

void usr_signal_handler(int signum) {
    printf("Caught signal %d\n",signum);
    // Cleanup and close up stuff here
    int * a;
    a = 0;
    *a = 12313;
}

void init() {
    printf("init");
    signal(SIGUSR2, usr_signal_handler);
    initialized = 1;
}

int clock_gettime(clockid_t clk_id, struct timespec *tp) {
    if (!initialized) {
        init();
    }
    // TODO call real time and do timeshift
    tp->tv_nsec = 0;
    tp->tv_sec = specifiedtime;
    return 0;
}

int gettimeofday(struct timeval *tp, void *tz) {
    if (!initialized) {
        init();
    }
    // TODO call real time and do timeshift
    tp->tv_sec = specifiedtime;
    tp->tv_usec = 0;
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
