This code is a proof of concept and is missing many useful things

example load test (~250kpps, ~184Mbit, i3-540 w/e1000e NIC as server, 4 server threads):

```
03:47:36 PM     IFACE   rxpck/s   txpck/s    rxkB/s    txkB/s   rxcmp/s   txcmp/s  rxmcst/s   %ifutil
03:47:37 PM       em1 251907.00 251904.00  23124.10  23125.47      0.00      0.00      1.00     18.94
03:47:38 PM       em1 250613.00 250591.00  23005.19  23005.07      0.00      0.00      0.00     18.85
03:47:39 PM       em1 254297.00 254314.00  23343.40  23346.73      0.00      0.00      0.00     19.13

average RTT at this load is 0.000128749 seconds
max RTT at this load is 0.008097817 seconds
packet loss under 0.01%
```

First missing thing: this code currently makes things up about the current fields in the NTP response:
  * stratum
  * ident
  * root dispersion
  * root distance
  * leap second status
  * precision

Second missing thing: this code currently 100% trusts the CLOCK\_REALTIME clock value

Third missing thing: everything else

Files:
  * ntp-loop.c - query a server once per second and print out timing information
  * ntp-query.c - query a server and print the request/response
  * pretend-ntp.c - NTP server


See also:
  * https://github.com/mlichvar/rsntp
