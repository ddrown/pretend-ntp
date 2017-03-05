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
#define SOCKETS 4

int main(int argc, char *argv[]) {
  int sock[SOCKETS];
  struct sockaddr_in addr;
  struct ntp_msg buf_in, buf_out;
  struct msghdr msg_out, msg_in;
  struct iovec iovec_in, iovec_out;
  uint32_t stop;
  struct timeval start,end,runtime;
  float runtime_f;
  struct pollfd readfd[SOCKETS];

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
  addr.sin_port = htons(strtoul(argv[2], NULL, 10));

  for(uint8_t i = 0; i < SOCKETS; i++) {
    socklen_t optlen;
    int optval;

    sock[i] = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock[i] == -1) {
      perror_exit("socket");
    }

    optval = 2 * 1024 * 1024; // 2MB
    if(setsockopt(sock[i], SOL_SOCKET, SO_RCVBUF, &optval, sizeof(optval)) < 0) {
      perror_exit("setsockopt(receive buffer)");
    }

    optlen = sizeof(optval);
    if(getsockopt(sock[i], SOL_SOCKET, SO_RCVBUF, &optval, &optlen) < 0) {
      perror_exit("getsockopt SO_RCVBUF");
    }
    printf("S%d socket size = %d\n", i, optval);

    readfd[i].fd = sock[i];
    readfd[i].events = POLLIN|POLLOUT;
  }

  memset(&msg_in, 0, sizeof(msg_in));
  iovec_in.iov_base = &buf_in;
  iovec_in.iov_len = sizeof(buf_in);
  msg_in.msg_iov = &iovec_in;
  msg_in.msg_iovlen = 1;

  memset(&msg_out, 0, sizeof(msg_out));
  iovec_out.iov_base = &buf_out;
  iovec_out.iov_len = sizeof(buf_out);
  msg_out.msg_iov = &iovec_out;
  msg_out.msg_iovlen = 1;
  msg_out.msg_name = &addr;
  msg_out.msg_namelen = sizeof(addr);

  gettimeofday(&start, NULL);
  uint64_t sent = 0, received = 0;
  end.tv_sec = start.tv_sec + stop;

  ntp_request(&buf_out);

  while(end.tv_sec > time(NULL)) {
    poll(readfd, SOCKETS, 500);
    for(uint8_t i = 0; i < SOCKETS; i++) {
      if(readfd[i].revents & POLLIN) {
	if(recvmsg(sock[i], &msg_in, MSG_DONTWAIT) > 0) {
	  received++;
	}
      }  
      if(readfd[i].revents & POLLOUT) {
	if(sendmsg(sock[i], &msg_out, MSG_DONTWAIT) > 0) {
	  sent++;
	}
      } 
    }
  }
  gettimeofday(&end, NULL);

  timersub(&end, &start, &runtime);
  runtime_f = runtime.tv_sec + (runtime.tv_usec / 1000000.0);

  printf("sent %" UINT64_FMT " received %" UINT64_FMT " %" UINT64_FMT "%% (tx %.2f/s rx %.2f/s)\n", sent, received, received*100/sent,
      sent/runtime_f, received/runtime_f);
}
