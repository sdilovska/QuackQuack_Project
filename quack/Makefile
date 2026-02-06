CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -O2

all: quack

quack: quack.c
	$(CC) $(CFLAGS) -o quack quack.c

memtest: lab1_test.c
	rm -rf test
	$(CC) -include quack.c -Wl,--defsym=main=test_mem -o test lab1_test.c
	./test

clean:
	rm -f quack

.PHONY: all clean
