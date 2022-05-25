CC=gcc
AR=ar
FLAGS= -Wall -g

all: shell1 shell2 shell3
shell1: shell1.o
	$(CC) $(FLAGS) -o shell1 shell1.o
shell2: shell2.o
	$(CC) $(FLAGS) -o shell2 shell2.o
shell3: shell3.o
	$(CC) $(FLAGS) -o shell3 shell3.o
shell1.o: shell1.c
	$(CC) $(FLAGS) -c shell1.c
shell2.o: shell2.c
	$(CC) $(FLAGS) -c shell2.c
shell3.o: shell3.c
	$(CC) $(FLAGS) -c shell3.c
.PHONY: clean all
clean:
	rm -f *.o *.a shell1 shell2 shell3