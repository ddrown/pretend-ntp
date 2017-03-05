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

#include "ntp_msg.h"
#include "timestamp.h"

void perror_exit(const char *msg) {
  perror(msg);
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

int ntp_xchange(int sock, struct ntp_msg *bufs, struct sockaddr_in *addr, struct ntptimes *t, struct msghdr *msgs) {
  int status[5];
  struct iovec iovecs;

  iovecs.iov_base     = bufs;
  iovecs.iov_len      = sizeof(*bufs);
  msgs->msg_iov        = &iovecs;
  msgs->msg_iovlen     = 1;
  msgs->msg_name       = addr;
  msgs->msg_namelen    = sizeof(*addr);

  status[0] = clock_gettime(CLOCK_REALTIME, &t->before_sendto);
  status[1] = sendto(sock, bufs, sizeof(*bufs), 0, addr, sizeof(*addr));
  status[2] = clock_gettime(CLOCK_REALTIME, &t->after_sendto);
  status[3] = recvmsg(sock, msgs, 0);
  status[4] = clock_gettime(CLOCK_REALTIME, &t->after_recvmsg);
  if(status[4] != 0) {
    perror_exit("clock_gettime #3");
  }
  if(status[3] < 0) {
    perror_exit("recvmsg");
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

  return status[3];
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

int main(int argc, char *argv[]) {
  int sock, opt;
  struct ntp_msg bufs;
  struct sockaddr_in addr;
  struct ntptimes t;
  struct msghdr msgs;
  int status;
  uint8_t cmsgbuf[256];
  uint8_t lines = 0;
  uint32_t stop;

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
  opt = 1;
  if(setsockopt(sock, IPPROTO_IP, IP_PKTINFO, &opt, sizeof(opt)) < 0) {
    perror_exit("setsockopt(IP_PKTINFO)");
  }
  addr.sin_port = htons(strtoul(argv[2], NULL, 10));

  memset(&msgs, 0, sizeof(msgs));
  msgs.msg_control    = cmsgbuf;
  msgs.msg_controllen = sizeof(cmsgbuf);

  for(uint32_t i = 0; i < stop; i++) {
    if(!lines) {
      printf("%20s %8s %8s %12s %12s %12s %12s %12s\n","ts","r.delay", "disper", "send", "recv", "proc", "sendto", "recvmsg");
      lines = 80;
    }
    lines--;
    ntp_request(&bufs);

    status = ntp_xchange(sock,&bufs,&addr,&t,&msgs);

    if(status > 0) {
      print_t(&t.before_sendto);
      printf(" ");

      print_short_ntp(&bufs, &t);
    }

    printf(" ");
    print_diff_ts(&t.after_sendto, &t.before_sendto);
    printf(" ");
    print_diff_ts(&t.after_recvmsg, &t.before_sendto);
    printf("\n");
    fflush(stdout);
    sleep(1);
  }
}
