CFLAGS=-Wall -std=gnu11

all: ntp-query pretend-ntp ntp-loop ntp-exit

ntp-query: ntp-query.o ntp_msg.o timestamp.o
	gcc $(CFLAGS) -o $@ $^

pretend-ntp: pretend-ntp.o ntp_msg.o timestamp.o
	gcc $(CFLAGS) -o $@ $^ -pthread

ntp-loop: ntp-loop.o ntp_msg.o timestamp.o
	gcc $(CFLAGS) -o $@ $^

ntp-exit: ntp-exit.o ntp_msg.o timestamp.o
	gcc $(CFLAGS) -o $@ $^

.PHONY: all
