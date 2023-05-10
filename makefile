CC=gcc
CFLAGS= -std=gnu99 -Wall -g
make: prog_s prog_tcp
prog_s: prog_s.c
		$(CC) $(CFLAGS) -fsanitize=address,undefined -o prog_s prog_s.c
prog_tcp: server.c
		$(CC) $(CFLAGS) -fsanitize=address,undefined -o prog_tcp prog_tcp.c
.PHONY: clean
clean: 
		rm -f prog_s prog_tcp
