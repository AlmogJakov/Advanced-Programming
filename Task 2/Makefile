CC=gcc
AR=ar
FLAGS= -Wall -g

all: stree
stree: stree.o
	$(CC) $(FLAGS) -o stree stree.o
stree.o: stree.c
	$(CC) $(FLAGS) -c stree.c
.PHONY: clean all
clean:
	rm -f *.o *.a stree