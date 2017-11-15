#define _GNU_SOURCE
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h>

#include "ntp_msg.h"
#include "timestamp.h"
#include "util.h"

#define DEFAULT_TIMEOUT_MS 2

// understand this: people don't like it when their servers or infrastructure are flooded. 
// Make sure you understand your impact and get permission from those you would impact
#ifdef I_KNOW_WHAT_IM_DOING
struct diff_stats {
  double min, max, total;
  uint32_t count;
};

void add_diff_stats(struct diff_stats *stat, const struct timespec *diff) {
  double new_ts = diff->tv_sec;

  new_ts = new_ts + diff->tv_nsec / 1000000000.0;

  if(stat->count == 0) {
    stat->min = stat->max = new_ts;
  } else {
    if(stat->min > new_ts)
      stat->min = new_ts;
    if(stat->max < new_ts)
      stat->max = new_ts;
  }
  stat->total += new_ts;
  stat->count++;
}

void print_stats(const struct diff_stats *stat) {
  printf("min/avg/max = %.9f/%.9f/%.9f (#%u)\n", stat->min, stat->total/stat->count, stat->max, stat->count);
}

int main(int argc, char *argv[]) {
  int sock;
  struct ntp_msg bufs;
  struct sockaddr_in addr;
  struct ntptimes t;
  struct msghdr msgs;
  int status;
  uint32_t stop;
  struct diff_stats rx_stats, tx_stats, proc_stats, rtt_stats;
  struct timeval start,end,runtime;
  float runtime_f;

  if(argc < 4) {
    printf("%s [ip] [port] [stop]\n", argv[0]);
    exit(1);
  }
  if(inet_aton(argv[1], &addr.sin_addr) == INADDR_NONE) {
    printf("invalid ip %s\n", argv[1]);
    exit(1);
  }
  stop = strtoul(argv[3], NULL, 10);
  addr.sin_family = AF_INET;

  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock == -1) {
    perror_exit("socket");
  }
  addr.sin_port = htons(strtoul(argv[2], NULL, 10));

  memset(&msgs, 0, sizeof(msgs));
  memset(&rx_stats, 0, sizeof(rx_stats));
  memset(&tx_stats, 0, sizeof(tx_stats));
  memset(&proc_stats, 0, sizeof(proc_stats));
  memset(&rtt_stats, 0, sizeof(rtt_stats));

  gettimeofday(&start, NULL);
  uint64_t success = 0, loss = 0;
  end.tv_sec = start.tv_sec + stop;
  while(end.tv_sec > time(NULL)) {
    ntp_request(&bufs);

    status = ntp_xchange(sock,&bufs,(struct sockaddr *)&addr,&t,&msgs,DEFAULT_TIMEOUT_MS);

    if(status > 0) {
      struct timespec diff, recv_time, trans_time;

      success++;

      recv_time.tv_sec = ntp_s_to_unixtime(bufs.recv_time);
      recv_time.tv_nsec = ntp_frac_to_ns(bufs.recv_time_fb);
      diff_ts(&t.before_sendto, &recv_time, &diff);
      add_diff_stats(&rx_stats, &diff);

      trans_time.tv_sec = ntp_s_to_unixtime(bufs.trans_time);
      trans_time.tv_nsec = ntp_frac_to_ns(bufs.trans_time_fb);
      diff_ts(&trans_time, &t.after_recvmsg, &diff);
      add_diff_stats(&tx_stats, &diff);

      diff_ts(&trans_time, &recv_time, &diff);
      add_diff_stats(&proc_stats, &diff);

      diff_ts(&t.after_recvmsg, &t.before_sendto, &diff);
      add_diff_stats(&rtt_stats, &diff);
    } else {
      loss++;
    }
  }
  gettimeofday(&end, NULL);

  timersub(&end, &start, &runtime);
  runtime_f = runtime.tv_sec + (runtime.tv_usec / 1000000.0);

  printf("good %" UINT64_FMT " lost %" UINT64_FMT " (%.2f/s)\n", success, loss, (success+loss)/runtime_f);
  printf("rx   ");
  print_stats(&rx_stats);
  printf("tx   ");
  print_stats(&tx_stats);
  printf("proc ");
  print_stats(&proc_stats);
  printf("rtt  ");
  print_stats(&rtt_stats);
}
#endif
