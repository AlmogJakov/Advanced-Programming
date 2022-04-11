CC=gcc
AR=ar
FLAGS= -Wall -g

all: ftw1 ftw2
ftw1: ftw1.o
	$(CC) $(FLAGS) -o ftw1 ftw1.o
ftw1.o: ftw1.c
	$(CC) $(FLAGS) -c ftw1.c
ftw2: ftw2.o
	$(CC) $(FLAGS) -o ftw2 ftw2.o
ftw2.o: ftw2.c
	$(CC) $(FLAGS) -c ftw2.c
.PHONY: clean all
clean:
	rm -f *.o *.a