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
#include <signal.h>
#include <poll.h>

#include "ntp_msg.h"
#include "timestamp.h"
#include "util.h"

#define DEFAULT_TIMEOUT_MS 500

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

    status = ntp_xchange(sock,&bufs,(struct sockaddr *)&addr,&t,&msgs,DEFAULT_TIMEOUT_MS);

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
