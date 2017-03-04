CFLAGS=-Wall -std=gnu11

all: ntp-query pretend-ntp ntp-loop

ntp-query: ntp-query.c
	gcc $(CFLAGS) -o ntp-query ntp-query.c

pretend-ntp: pretend-ntp.c
	gcc $(CFLAGS) -o pretend-ntp pretend-ntp.c

ntp-loop: ntp-loop.c
	gcc $(CFLAGS) -o ntp-loop ntp-loop.c

.PHONY: all
