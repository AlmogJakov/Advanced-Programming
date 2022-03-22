CC=gcc
AR=ar
OBJECTS_MAIN = slast.o
FLAGS= -Wall -g

all: slast
slast: slast.o
	$(CC) $(FLAGS) -o slast $(OBJECTS_MAIN)
slast.o: slast.c
	$(CC) $(FLAGS) -c slast.c
.PHONY: clean all
clean:
	rm -f *.o *.a