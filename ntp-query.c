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

#define NTP_LEAP_NONE   0
#define NTP_LEAP_61S    1
#define NTP_LEAP_59S    2
#define NTP_LEAP_UNSYNC 3
#define NTP_VERS_4      4
#define NTP_VERS_3      3
#define NTP_MODE_RSVD   0
#define NTP_MODE_SYMACT 1
#define NTP_MODE_SYMPAS 2
#define NTP_MODE_CLIENT 3
#define NTP_MODE_SERVER 4
#define NTP_MODE_BROADC 5
#define NTP_MODE_CTRL   6
#define NTP_MODE_PRIV   7
struct ntp_msg {
  uint8_t mode : 3;
  uint8_t version : 3;
  uint8_t leap : 2;
  uint8_t stratum;
  uint8_t poll;
  int8_t precision;
  uint16_t delay;
  uint16_t delay_fb;
  uint16_t dispersion;
  uint16_t dispersion_fb;
  uint32_t ident;
  uint32_t ref_time;
  uint32_t ref_time_fb;
  uint32_t org_time;
  uint32_t org_time_fb;
  uint32_t recv_time;
  uint32_t recv_time_fb;
  uint32_t trans_time;
  uint32_t trans_time_fb;
};

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

void print_t(const struct timespec *t) {
  printf("%ld.%09ld", t->tv_sec, t->tv_nsec);
}

void print_diff_ts(const struct timespec *t1, const struct timespec *t2) {
  int32_t diff_s = t1->tv_sec - t2->tv_sec;
  int32_t diff_ns = t1->tv_nsec - t2->tv_nsec;
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
    printf("%d.%09d", diff_s, diff_ns*-1);
  } else {
    printf("%d.%09d", diff_s, diff_ns);
  }
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

void perror_exit(const char *msg) {
  perror_exit(msg);
  exit(1);
}

void print_addr(const struct sockaddr_in *a) {
  printf("%s:%u\n", inet_ntoa(a->sin_addr), ntohs(a->sin_port));
}

void print_pktinfo(const struct in_pktinfo *p) {
  printf("interface %u\n", p->ipi_ifindex);
  printf("spec_dst %s\n", inet_ntoa(p->ipi_spec_dst));
  printf("dst %s\n", inet_ntoa(p->ipi_addr));
}

#define NUM_MESSAGES 1
#define TIMEOUT 1
int main(int argc, char *argv[]) {
  struct ntp_msg bufs;
  struct timespec t[3];
  int status[5];
  int sock, opt;
  struct sockaddr_in addr;
  struct mmsghdr msgs[NUM_MESSAGES];
  struct iovec iovecs[NUM_MESSAGES];
  struct timespec timeout;
  uint8_t cmsgbuf[256];

  if(argc < 3) {
    printf("%s [ip] [port]\n", argv[0]);
    exit(1);
  }
  if(inet_aton(argv[1], &addr.sin_addr) == INADDR_NONE) {
    printf("invalid ip %s\n", argv[1]);
    exit(1);
  }
  addr.sin_port = htons(strtoul(argv[2], NULL, 10));
  addr.sin_family = AF_INET;

  memset(&bufs, 0, sizeof(bufs));
  bufs.version = NTP_VERS_4;
  bufs.mode = NTP_MODE_CLIENT;
  clock_gettime(CLOCK_REALTIME, &t[0]);
  bufs.trans_time = unixtime_to_ntp_s(t[0].tv_sec);
  bufs.trans_time_fb = ns_to_ntp_frac(t[0].tv_nsec);
  printf("=== request ===\n");
  print_ntp(&bufs, NULL, NULL);

  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock == -1) {
    perror_exit("socket");
  }
  opt = 1;
  if(setsockopt(sock, IPPROTO_IP, IP_PKTINFO, &opt, sizeof(opt)) < 0) {
    perror_exit("setsockopt(IP_PKTINFO)");
  }

  memset(msgs, 0, sizeof(msgs));
  iovecs[0].iov_base          = &bufs;
  iovecs[0].iov_len           = sizeof(bufs);
  msgs[0].msg_hdr.msg_iov     = &iovecs[0];
  msgs[0].msg_hdr.msg_iovlen  = 1;
  msgs[0].msg_hdr.msg_name    = &addr;
  msgs[0].msg_hdr.msg_namelen = sizeof(addr);
  msgs[0].msg_hdr.msg_control = cmsgbuf;
  msgs[0].msg_hdr.msg_controllen = sizeof(cmsgbuf);

  timeout.tv_sec = TIMEOUT;
  timeout.tv_nsec = 0;

  status[0] = clock_gettime(CLOCK_REALTIME, &t[0]);
  status[1] = sendto(sock, &bufs, sizeof(bufs), 0, &addr, sizeof(addr));
  status[2] = clock_gettime(CLOCK_REALTIME, &t[1]);
  status[3] = recvmmsg(sock, msgs, NUM_MESSAGES, MSG_WAITFORONE, &timeout);
  status[4] = clock_gettime(CLOCK_REALTIME, &t[2]);
  if(status[4] != 0) {
    perror_exit("clock_gettime #3");
  }
  if(status[3] < 0) {
    perror_exit("recvmmsg");
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

  printf("=== %d messages received ===\n=== response ===\n", status[3]);
  if(status[3] > 0) {
    printf("rtt = ");
    print_diff_ts(&t[2], &t[1]);
    printf("\n");

    printf("source = ");
    print_addr(&addr);
    for(struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msgs[0].msg_hdr); cmsg != NULL; cmsg = CMSG_NXTHDR(&msgs[0].msg_hdr, cmsg)) {
      if(cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_PKTINFO) {
        print_pktinfo((struct in_pktinfo *)CMSG_DATA(cmsg));
      }
    }
    printf("len = %u\n", msgs[0].msg_len);
    print_ntp(&bufs, &t[1], &t[2]);
  }

  printf("before sendto = ");
  print_t(&t[0]);
  printf("\n");
  printf("after sendto = ");
  print_t(&t[1]);
  printf(" ");
  print_diff_ts(&t[1], &t[0]);
  printf("\n");
  printf("after recvmmsg = ");
  print_t(&t[2]);
  printf(" ");
  print_diff_ts(&t[2], &t[1]);
  printf("\n");
}
