#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <poll.h>
#include <errno.h>

#include "ntp_msg.h"
#include "timestamp.h"
#include "util.h"

char *ntp_leap_str[] = {
  "none", "61s", "59s", "unsync"
};

char *ntp_mode_str[] = {
  "reserved", "sym. active", "sym. passive", "client", "server", "broadcast", "control msg", "private"
};

// todo: NTP era
uint32_t ntp_s_to_unixtime(uint32_t ntp_s) {
  if(ntp_s == 0) {
    return 0;
  }
  return ntohl(ntp_s) - 2208988800;
}

uint32_t unixtime_to_ntp_s(uint32_t unix_ts) {
  return htonl(unix_ts + 2208988800);
}

uint32_t ns_to_ntp_frac(uint32_t ns) {
  uint64_t fractional = ns;
  fractional = fractional * 4294967296 / 1000000000;
  return htonl(fractional);
}

uint32_t ntp_frac_to_ns(uint32_t ntp_fb) {
  uint64_t fractional_s = ntohl(ntp_fb);
  fractional_s = fractional_s * 1000000000 / 4294967296;
  return fractional_s;
}

uint16_t ntp_frac_to_us(uint16_t ntp_fb) {
  uint32_t fractional_s = ntohs(ntp_fb) * 1000000 / 65536;
  return fractional_s;
}

void print_ntp(const struct ntp_msg *m, const struct timespec *send, const struct timespec *recv) {
  struct timespec recv_time, trans_time;

  if(m->leap < sizeof(ntp_leap_str)/sizeof(char *)) {
    printf("leap = %s\n", ntp_leap_str[m->leap]);
  } else {
    printf("leap = %u\n", m->leap);
  }
  printf("version = %u\n", m->version);
  if(m->mode < sizeof(ntp_mode_str)/sizeof(char *)) {
    printf("mode = %s\n", ntp_mode_str[m->mode]);
  } else {
    printf("mode = %u\n", m->mode);
  }
  printf("stratum = %u\n", m->stratum);
  printf("poll = %u\n", m->poll);
  printf("precision = %d\n", m->precision);
  printf("delay = %u.%06u\n", ntohs(m->delay), ntp_frac_to_us(m->delay_fb));
  printf("dispersion = %u.%06u\n", ntohs(m->dispersion), ntp_frac_to_us(m->dispersion_fb));
  printf("ident = %u\n", ntohl(m->ident));
  printf("ref_time = %u.%09u\n", ntp_s_to_unixtime(m->ref_time), ntp_frac_to_ns(m->ref_time_fb));
  printf("org_time = %u.%09u\n", ntp_s_to_unixtime(m->org_time), ntp_frac_to_ns(m->org_time_fb));

  recv_time.tv_sec = ntp_s_to_unixtime(m->recv_time);
  recv_time.tv_nsec = ntp_frac_to_ns(m->recv_time_fb);
  printf("recv_time = ");
  print_t(&recv_time);
  if(send != NULL) {
    printf(" ");
    print_diff_ts(&recv_time, send);
  }
  printf("\n");

  trans_time.tv_sec = ntp_s_to_unixtime(m->trans_time);
  trans_time.tv_nsec = ntp_frac_to_ns(m->trans_time_fb);
  printf("trans_time = ");
  print_t(&trans_time);
  if(recv != NULL) {
    printf(" ");
    print_diff_ts(&trans_time, recv);
  }
  printf("\n");

  printf("proc time = ");
  print_diff_ts(&trans_time, &recv_time);
  printf("\n");
}

void print_short_ntp(const struct ntp_msg *m, const struct ntptimes *t) {
  struct timespec recv_time, trans_time;

  printf(" %u.%06u", ntohs(m->delay), ntp_frac_to_us(m->delay_fb));
  printf(" %u.%06u", ntohs(m->dispersion), ntp_frac_to_us(m->dispersion_fb));

  recv_time.tv_sec = ntp_s_to_unixtime(m->recv_time);
  recv_time.tv_nsec = ntp_frac_to_ns(m->recv_time_fb);
  if(t != NULL) {
    printf(" ");
    print_diff_ts(&t->before_sendto, &recv_time);
  }

  trans_time.tv_sec = ntp_s_to_unixtime(m->trans_time);
  trans_time.tv_nsec = ntp_frac_to_ns(m->trans_time_fb);
  if(t != NULL) {
    printf(" ");
    print_diff_ts(&trans_time, &t->after_recvmsg);
  }

  printf(" ");
  print_diff_ts(&trans_time, &recv_time);
}

void ntp_request(struct ntp_msg *bufs) {
  struct timespec t;

  memset(bufs, 0, sizeof(*bufs));
  bufs->version = NTP_VERS_4;
  bufs->mode = NTP_MODE_CLIENT;
  clock_gettime(CLOCK_REALTIME, &t);
  bufs->trans_time = unixtime_to_ntp_s(t.tv_sec);
  bufs->trans_time_fb = ns_to_ntp_frac(t.tv_nsec);
}

int ntp_xchange(int sock, struct ntp_msg *bufs, const struct sockaddr *addr, struct ntptimes *t, struct msghdr *msgs, int timeout_ms) {
  int status[6];
  struct iovec iovecs;
  struct pollfd readfd;

  iovecs.iov_base     = bufs;
  iovecs.iov_len      = sizeof(*bufs);
  msgs->msg_iov        = &iovecs;
  msgs->msg_iovlen     = 1;

  readfd.fd = sock;
  readfd.events = POLLIN;

  status[0] = clock_gettime(CLOCK_REALTIME, &t->before_sendto);
  status[1] = sendto(sock, bufs, sizeof(*bufs), 0, addr, sizeof(*addr));
  status[2] = clock_gettime(CLOCK_REALTIME, &t->after_sendto);
  status[3] = poll(&readfd, 1, timeout_ms);
  status[4] = clock_gettime(CLOCK_REALTIME, &t->after_recvmsg);
  status[5] = recvmsg(sock, msgs, MSG_DONTWAIT);
  if(status[5] < 0) {
    if(errno == EAGAIN) {
      return 0;
    }
    perror_exit("recvmsg");
  }
  if(status[4] != 0) {
    perror_exit("clock_gettime #3");
  }
  if(status[3] < 0) {
    perror_exit("poll");
  }
  if(status[2] != 0) {
    perror_exit("clock_gettime #2");
  }
  if(status[1] < 0) {
    perror_exit("sendto");
  }
  if(status[0] != 0) {
    perror_exit("clock_gettime #1");
  }

  return status[5];
}
