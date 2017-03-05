#ifndef TIMESTAMP_H
#define TIMESTAMP_H

void print_t(const struct timespec *t);
void diff_ts(const struct timespec *t1, const struct timespec *t2, struct timespec *diff);
void print_diff_ts(const struct timespec *t1, const struct timespec *t2);

#endif
