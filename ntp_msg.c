#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>

#include "ntp_msg.h"
#include "timestamp.h"

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

