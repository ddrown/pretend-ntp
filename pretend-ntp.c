#define _GNU_SOURCE
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include "ntp_msg.h"
#include "util.h"

#define THREADS 4

struct thread_info {
  pthread_t thread_id;
  int thread_num;
  uint64_t thread_queries;
  uint64_t unknown_version;
  uint64_t unknown_mode;
  struct sockaddr_in *addr;
};

void *ntp_server(void *arg) {
  struct thread_info *tinfo = arg;
  struct ntp_msg in, out;
  int sock;
  int true_int = 1;

  tinfo->thread_queries = tinfo->unknown_version = tinfo->unknown_mode = 0;

  if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror_exit("socket");
  }

  if(setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &true_int, sizeof(true_int)) < 0) {
    perror_exit("setsockopt SO_REUSEADDR");
  }

  if(bind(sock, tinfo->addr, sizeof(*tinfo->addr)) < 0) {
    perror_exit("bind");
  }

  memset(&out, 0, sizeof(out));
  out.version = NTP_VERS_4;
  out.mode = NTP_MODE_SERVER;
  out.stratum = 1;
  out.ident = htonl(0x50505300); // PPS
  out.precision = -20;

  while(1) {
    int len;
    struct sockaddr_in source_addr;
    socklen_t addrlen = sizeof(source_addr); 
    struct timespec t;

    len = recvfrom(sock, &in, sizeof(in), 0, &source_addr, &addrlen);
    clock_gettime(CLOCK_REALTIME, &t);

    if(len < 0) 
      perror_exit("recvfrom");

    if(in.version < 2 || in.version > 4) {
      tinfo->unknown_version++;
      continue;
    }
    if(in.mode == NTP_MODE_PRIV) { // TODO: better exit
      printf("thread %d exiting\n", tinfo->thread_num);
      close(sock);
      return NULL;
    }
    if(in.mode != NTP_MODE_CLIENT) {
      tinfo->unknown_mode++;
      continue;
    }

    tinfo->thread_queries++;

    out.org_time = in.trans_time;
    out.org_time_fb = in.trans_time_fb;

    out.ref_time = out.recv_time = unixtime_to_ntp_s(t.tv_sec);
    out.ref_time_fb = out.recv_time_fb = ns_to_ntp_frac(t.tv_nsec);

    clock_gettime(CLOCK_REALTIME, &t);
    out.trans_time_fb = ns_to_ntp_frac(t.tv_nsec);
    out.trans_time = unixtime_to_ntp_s(t.tv_sec);

    sendto(sock, &out, sizeof(out), 0, &source_addr, sizeof(source_addr));
  }
}

int main(int argc, char *argv[]) {
  struct sockaddr_in addr;
  struct thread_info tinfo[THREADS];
  int status;

  if(argc < 3) {
    printf("%s [ip] [port]\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  if(inet_aton(argv[1], &addr.sin_addr) == INADDR_NONE) {
    printf("invalid ip %s\n", argv[1]);
    exit(EXIT_FAILURE);
  }
  addr.sin_port = htons(strtoul(argv[2], NULL, 10));
  addr.sin_family = AF_INET;

  memset(&tinfo, 0, sizeof(tinfo));

  for (uint32_t i = 0; i < THREADS; i++) {
    tinfo[i].thread_num = i;
    tinfo[i].addr = &addr;
    status = pthread_create(&tinfo[i].thread_id, NULL, &ntp_server, &tinfo[i]);
    if (status != 0) {
      errno = status;
      perror("pthread_create(ntp_server) failed");
      exit(EXIT_FAILURE);
    }
  }

  printf("threads created\n");

  for(uint32_t i = 0; i < THREADS; i++) {
    void *res;
    status = pthread_join(tinfo[i].thread_id, &res);
    if (status != 0) {
      errno = status;
      perror("pthread_join(ntp_server) failed");
      exit(EXIT_FAILURE);
    }

    printf("reader thread %d done q=%lu !v=%lu !m=%lu\n", tinfo[i].thread_num, tinfo[i].thread_queries,
        tinfo[i].unknown_version, tinfo[i].unknown_mode);
  }

  exit(EXIT_SUCCESS);
}
