CFLAGS=-Wall -std=gnu11

all: ntp-query pretend-ntp ntp-loop ntp-exit ntp-flood ntp-flood2

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

ntp-flood2: ntp-flood2.o ntp_msg.o timestamp.o util.o
	gcc $(CFLAGS) -o $@ $^

.PHONY: all
