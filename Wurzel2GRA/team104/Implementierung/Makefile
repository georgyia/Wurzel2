CFLAGS=-O3 -g -Wall -Wextra -Wpedantic

all: main
main: bignum.c add_sub.c mul.c div.c sqrt2.c main.c
	$(CC) $(CFLAGS) -o $@ $^
clean:
	rm -f main