#include <sys/types.h>
#include <time.h>
#include <sys/timeb.h>
// static boolean initialized = false;

static time_t       (*real_time)            (time_t *);
static int          (*real_ftime)           (struct timeb *);
static int          (*real_gettimeofday)    (struct timeval *, void *);
static int          (*real_clock_gettime)   (clockid_t clk_id, struct timespec *tp);

static int specifiedtime = 1600000;


void init() {

}

int clock_gettime(clockid_t clk_id, struct timespec *tp) {
    // TODO call real time and do timeshift
    tp->tv_nsec = 0;
    tp->tv_sec = specifiedtime;
    return 0;
}

int gettimeofday(struct timeval *tp, void *tz) {
    // TODO call real time and do timeshift
    tp->tv_sec = specifiedtime;
    tp->tv_usec = 0;
    return 0;
}

time_t time(time_t *tloc) {
    if (tloc != NULL) {
        *tloc = specifiedtime;
    }
    return specifiedtime;
}

int ftime(struct timeb * s) {
    s->time = specifiedtime;
    s->millitm = 0;

    return 0;
}
