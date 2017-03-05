#ifndef NTP_MSG_H
#define NTP_MSG_H

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

struct ntptimes {
  struct timespec before_sendto,after_sendto,after_recvmsg;
};

extern char *ntp_leap_str[];
extern char *ntp_mode_str[];

uint32_t ntp_s_to_unixtime(uint32_t ntp_s);
uint32_t unixtime_to_ntp_s(uint32_t unix_ts);
uint32_t ns_to_ntp_frac(uint32_t ns);
uint32_t ntp_frac_to_ns(uint32_t ntp_fb);
uint16_t ntp_frac_to_us(uint16_t ntp_fb);
void print_ntp(const struct ntp_msg *m, const struct timespec *send, const struct timespec *recv);
void print_short_ntp(const struct ntp_msg *m, const struct ntptimes *t);

#endif
