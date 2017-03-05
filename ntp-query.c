#define _GNU_SOURCE
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include "ntp_msg.h"
#include "timestamp.h"
#include "util.h"

#define DEFAULT_TIMEOUT_MS 500

void print_addr(const struct sockaddr_in *a) {
  printf("%s:%u\n", inet_ntoa(a->sin_addr), ntohs(a->sin_port));
}

void print_pktinfo(const struct in_pktinfo *p) {
  printf("interface %u\n", p->ipi_ifindex);
  printf("spec_dst %s\n", inet_ntoa(p->ipi_spec_dst));
  printf("dst %s\n", inet_ntoa(p->ipi_addr));
}

int main(int argc, char *argv[]) {
  struct ntp_msg bufs;
  struct timespec t[3];
  int status[6];
  int sock, opt;
  struct sockaddr_in addr;
  struct msghdr msg;
  struct iovec iovec;
  uint8_t cmsgbuf[256];
  struct pollfd readfd;

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

  memset(&msg, 0, sizeof(msg));
  iovec.iov_base          = &bufs;
  iovec.iov_len           = sizeof(bufs);
  msg.msg_iov     = &iovec;
  msg.msg_iovlen  = 1;
  msg.msg_name    = &addr;
  msg.msg_namelen = sizeof(addr);
  msg.msg_control = cmsgbuf;
  msg.msg_controllen = sizeof(cmsgbuf);

  readfd.fd = sock;
  readfd.events = POLLIN;

  status[0] = clock_gettime(CLOCK_REALTIME, &t[0]);
  status[1] = sendto(sock, &bufs, sizeof(bufs), 0, &addr, sizeof(addr));
  status[2] = clock_gettime(CLOCK_REALTIME, &t[1]);
  status[3] = poll(&readfd, 1, DEFAULT_TIMEOUT_MS);
  status[4] = clock_gettime(CLOCK_REALTIME, &t[2]);
  status[5] = recvmsg(sock, &msg, MSG_DONTWAIT);
  if((status[5] < 0) && (errno != EAGAIN)) {
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

  printf("=== %d packet(s) received ===\n=== response ===\n", status[3]);
  if(status[5] > 0) {
    printf("rtt = ");
    print_diff_ts(&t[2], &t[1]);
    printf("\n");

    printf("source = ");
    print_addr(&addr);
    for(struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
      if(cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_PKTINFO) {
        print_pktinfo((struct in_pktinfo *)CMSG_DATA(cmsg));
      }
    }
    printf("len = %u\n", status[5]);
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
