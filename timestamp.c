#include <stdio.h>
#include <time.h>

#include "timestamp.h"

void print_t(const struct timespec *t) {
  printf("%ld.%09ld", t->tv_sec, t->tv_nsec);
}

void diff_ts(const struct timespec *t1, const struct timespec *t2, struct timespec *diff) {
  diff->tv_sec = t1->tv_sec - t2->tv_sec;
  diff->tv_nsec = t1->tv_nsec - t2->tv_nsec;
  if(diff->tv_nsec < -1000000000 || (diff->tv_nsec < 0 && diff->tv_sec > 0)) {
    diff->tv_sec--;
    diff->tv_nsec += 1000000000;
  }
  if(diff->tv_nsec > 0 && diff->tv_sec < 0) {
    diff->tv_sec++;
    diff->tv_nsec -= 1000000000;
  }
}

void print_diff_ts(const struct timespec *t1, const struct timespec *t2) {
  struct timespec diff;

  diff_ts(t1, t2, &diff);

  if(diff.tv_nsec < 0 && diff.tv_sec == 0) {
    printf("-0.%09ld", diff.tv_nsec*-1);
  } else if(diff.tv_nsec < 0) {
    printf("%ld.%09ld", diff.tv_sec, diff.tv_nsec*-1);
  } else {
    printf("%ld.%09ld", diff.tv_sec, diff.tv_nsec);
  }
}

