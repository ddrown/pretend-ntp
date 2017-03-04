#define _GNU_SOURCE
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>

void print_diff_ts(const struct timespec *t1, const struct timespec *t2) {
  int32_t diff_s = t1->tv_sec - t2->tv_sec;
  int32_t diff_ns = t1->tv_nsec - t2->tv_nsec;
  printf("s = %d ns = %d\n", diff_s, diff_ns);
  if(diff_ns < -1000000000 || (diff_ns < 0 && diff_s > 0)) {
    diff_s--;
    diff_ns += 1000000000;
  }
  if(diff_ns > 0 && diff_s < 0) {
    diff_s++;
    diff_ns -= 1000000000;
  }
  if(diff_ns < 0 && diff_s == 0) {
    printf("-0.%09d", diff_ns*-1);
  } else if(diff_ns < 0) {
    printf("%2d.%09d", diff_s, diff_ns*-1);
  } else {
    printf("%2d.%09d", diff_s, diff_ns);
  }
}

int main(int argc, char *argv[]) {
  struct timespec t1 = {.tv_sec = 1488497998, .tv_nsec = 1032486};
  struct timespec t2 = {.tv_sec = 1488497997, .tv_nsec = 999975064};
  struct timespec t3 = {.tv_sec = 1488497998, .tv_nsec = 343574};
  struct timespec t4 = {.tv_sec = 1488497998, .tv_nsec = 343573};
  struct timespec t5 = {.tv_sec = 1488497998, .tv_nsec = 343575};

  print_diff_ts(&t1, &t2);
  printf("\n");
  print_diff_ts(&t3, &t2);
  printf("\n");
  print_diff_ts(&t3, &t4);
  printf("\n");
  print_diff_ts(&t3, &t5);
  printf("\n");
}
