CC=gcc
CFLAGS= -std=gnu99 -Wall -g
LDFLAGS=-fsanitize=address,undefined
make: prog_s
make_tcp: prog_s
make_pipe: War_card_game
make_msq: client server
prog_s: prog_s.c
		$(CC) $(CFLAGS) $(LDFLAGS) -o prog_s prog_s.c
client: client.c
		$(CC) $(CFLAGS) $(LDFLAGS) -o client client.c
server: server.c
		$(CC) $(CFLAGS) $(LDFLAGS) -o server server.c
War_card_game: War_card_game.c
		$(CC) $(CFLAGS) $(LDFLAGS) -o War_card_game War_card_game.c
.PHONY: clean
clean: 
		rm -f prog_s client server War_card_game
