CC=gcc
AR=ar
OBJECTS_MAIN = main.o utmplib.o
FLAGS= -Wall -g

all: main
main: main.o utmplib.o
	$(CC) $(FLAGS) -o main $(OBJECTS_MAIN)
main.o: main.c
	$(CC) $(FLAGS) -c main.c
utmplib.o: utmplib.c
	$(CC) $(FLAGS) -c utmplib.c
.PHONY: clean all
clean:
	rm -f *.o *.a