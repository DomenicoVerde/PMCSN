CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lm

OBJFILES = rngs.o rvgs.o rvms.o nsssn_bp.o ver_and_val.o nsssn_bp_loss.o transiente.o transiente_loss.o stazionaria.o stazionaria_loss.o  

all: $(OBJFILES)

rngs.o: rngs.c rngs.h
	$(CC) -c $<

rvgs.o: rvgs.c rvgs.h rngs.c rngs.h
	$(CC) -c $<

rvms.o: rvms.c rvms.h
	$(CC) -c $<

ver_and_val.o: ver_and_val.c rngs.o rvgs.o
	$(CC) $^ -o $@ $(LDFLAGS)

nsssn_bp.o: nsssn_bp.c rngs.o rvgs.o
	$(CC) $^ -o $@ $(LDFLAGS)

nsssn_bp_loss.o: nsssn_bp_loss.c rngs.o rvgs.o
	$(CC) $^ -o $@ $(LDFLAGS)

transiente.o: transiente.c rngs.o rvgs.o
	$(CC) $^ -o $@ $(LDFLAGS)

transiente_loss.o: transiente_loss.c rngs.o rvgs.o
	$(CC) $^ -o $@ $(LDFLAGS)

stazionaria.o: stazionaria.c rngs.o rvgs.o rvms.o
	$(CC) $^ -o $@ $(LDFLAGS)

stazionaria_loss.o: stazionaria_loss.c rngs.o rvgs.o rvms.o
	$(CC) $^ -o $@ $(LDFLAGS)


clean:
	/bin/rm -f $(OBJFILES) core*
