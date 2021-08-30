CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lm

OBJFILES = rngs.o rvgs.o nsssn_exp.o nsssn_bp.o nsssn_bp_loss.o

all: $(OBJFILES)

rngs.o: rngs.c rngs.h
	$(CC) -c $<

rvgs.o: rvgs.c rvgs.h rngs.c rngs.h
	$(CC) -c $<

nsssn_exp.o: nsssn_exp.c rngs.o rvgs.o
	$(CC) $^ -o $@ $(LDFLAGS)

nsssn_bp.o: nsssn_bp.c rngs.o rvgs.o
	$(CC) $^ -o $@ $(LDFLAGS)

nsssn_bp_loss.o: nsssn_bp_loss.c rngs.o rvgs.o
	$(CC) $^ -o $@ $(LDFLAGS)

clean:
	/bin/rm -f $(OBJFILES) core*
