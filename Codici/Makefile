CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lm

OBJFILES = rngs.o rvgs.o ssq4.o

all: $(OBJFILES)

rngs.o: rngs.c rngs.h
	$(CC) -c $<

rvgs.o: rvgs.c rvgs.h rngs.c rngs.h
	$(CC) -c $<

ssq4.o: ssq4.c rngs.o rvgs.o
	$(CC) $^ -o $@ $(LDFLAGS)

clean:
	/bin/rm -f $(OBJFILES) core*
