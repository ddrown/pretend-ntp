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

void perror_exit(const char *msg) {
  perror(msg);
  exit(1);
}

int main(int argc, char *argv[]) {
  int sock;
  struct ntp_msg in, out;
  struct sockaddr_in addr;
  struct timespec t;
  struct msghdr msgs;
  struct iovec iovecs;
  int status;

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

  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock == -1) {
    perror_exit("socket");
  }
  status = bind(sock, &addr, sizeof(addr));
  if(status < 0) {
    perror_exit("bind");
  }

  memset(&msgs, 0, sizeof(msgs));
  iovecs.iov_base     = &out;
  iovecs.iov_len      = sizeof(out);
  msgs.msg_iov        = &iovecs;
  msgs.msg_iovlen     = 1;
  msgs.msg_name       = &addr;
  msgs.msg_namelen    = sizeof(addr);

  memset(&out, 0, sizeof(out));
  out.version = NTP_VERS_4;
  out.mode = NTP_MODE_SERVER;
  out.stratum = 1;
  out.ident = htonl(0x50505300); // PPS
  out.precision = -20;

  while(1) {
    int len;
    socklen_t addrlen = sizeof(addr); 

    len = recvfrom(sock, &in, sizeof(in), 0, &addr, &addrlen);
    clock_gettime(CLOCK_REALTIME, &t);
    if(len < 0) 
      perror_exit("recvfrom");
    out.org_time = in.trans_time;
    out.org_time_fb = in.trans_time_fb;
    out.ref_time = out.recv_time = unixtime_to_ntp_s(t.tv_sec);
    out.ref_time_fb = out.recv_time_fb = ns_to_ntp_frac(t.tv_nsec);
    clock_gettime(CLOCK_REALTIME, &t);
    out.trans_time_fb = ns_to_ntp_frac(t.tv_nsec);
    out.trans_time = unixtime_to_ntp_s(t.tv_sec);
    sendto(sock, &out, sizeof(out), 0, &addr, sizeof(addr));
  }
}
