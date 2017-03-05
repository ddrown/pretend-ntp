CFLAGS=-Wall -std=gnu11

all: ntp-query pretend-ntp ntp-loop ntp-exit ntp-flood

ntp-query: ntp-query.o ntp_msg.o timestamp.o util.o
	gcc $(CFLAGS) -o $@ $^

pretend-ntp: pretend-ntp.o ntp_msg.o timestamp.o util.o
	gcc $(CFLAGS) -o $@ $^ -pthread

ntp-loop: ntp-loop.o ntp_msg.o timestamp.o util.o
	gcc $(CFLAGS) -o $@ $^

ntp-exit: ntp-exit.o ntp_msg.o timestamp.o util.o
	gcc $(CFLAGS) -o $@ $^

ntp-flood: ntp-flood.o ntp_msg.o timestamp.o util.o
	gcc $(CFLAGS) -o $@ $^

.PHONY: all
