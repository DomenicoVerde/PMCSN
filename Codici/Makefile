CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lm

OBJFILES = rngs.o rvgs.o nsssn.o

all: $(OBJFILES)

rngs.o: rngs.c rngs.h
	$(CC) -c $<

rvgs.o: rvgs.c rvgs.h rngs.c rngs.h
	$(CC) -c $<

nsssn.o: nsssn.c rngs.o rvgs.o
	$(CC) $^ -o $@ $(LDFLAGS)

clean:
	/bin/rm -f $(OBJFILES) core*
