#CC = riscv64-linux-gnu-gcc
CC = gcc
CFLAGS = -Wall -Wextra -O2 -static

all: player1 player2

player1: player1.c common.h
	$(CC) $(CFLAGS) -o player1 player1.c

player2: player2.c common.h
	$(CC) $(CFLAGS) -o player2 player2.c

clean:
	rm -f player1 player2 

.PHONY: all clean
