CC = gcc
CFLAGS = -Wall -std=c99 -g

all: words

words: words.c
	$(CC) $(CFLAGS) words.c -o words

clean:
	rm -f words *.o