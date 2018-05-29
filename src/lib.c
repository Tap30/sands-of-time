#include <sys/types.h>
#include <time.h>
#include <sys/timeb.h>
// static boolean initialized = false;


void init() {

}

int clock_gettime(clockid_t clk_id, struct timespec *tp) {
    tp->tv_nsec = 0;
    tp->tv_sec = 1500000;
    return 0;
}

int gettimeofday(struct timeval *restrict tp, void *restrict tzp) {
    return 1500000;
}



// time_t time(time_t *tloc) {
//     return 1500000;
// }

// int ftime(struct timeb * s) {
//     return 1500000;
// }
