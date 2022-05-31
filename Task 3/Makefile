CC=gcc
AR=ar
FLAGS= -Wall -g

all: shell
shell: shell.o
	$(CC) $(FLAGS) -o shell shell.o
shell.o: shell.c
	$(CC) $(FLAGS) -c shell.c
.PHONY: clean all
clean:
	rm -f *.o *.a shell