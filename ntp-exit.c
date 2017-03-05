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

#include "ntp_msg.h"
#include "util.h"

#define NUM_MESSAGES 1
#define TIMEOUT 1
int main(int argc, char *argv[]) {
  struct ntp_msg buf;
  int sock;
  struct sockaddr_in addr;

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

  memset(&buf, 0, sizeof(buf));
  buf.version = NTP_VERS_4;
  buf.mode = NTP_MODE_PRIV;

  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock == -1) {
    perror_exit("socket");
  }
  sendto(sock, &buf, sizeof(buf), 0, &addr, sizeof(addr));
}
