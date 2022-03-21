CC=gcc
AR=ar
OBJECTS_MAIN = main.o
FLAGS= -Wall -g

all: main
main: main.o
	$(CC) $(FLAGS) -o main $(OBJECTS_MAIN)
main.o: main.c
	$(CC) $(FLAGS) -c main.c
.PHONY: clean all
clean:
	rm -f *.o *.a