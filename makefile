CFLAGS=-O -std=c89

OBJS = 	bin/generate.o \
	bin/tools.o \
	bin/loop.o \
	bin/move.o \
	bin/eval.o \
	bin/chess.o \
	bin/actions.o \
	bin/legal.o \
	bin/strike.o

chess : $(OBJS)
	cc -o $@ $^

bin/generate.o : src/generate.c src/actions.h src/chess.h
	cc -c $(CFLAGS) $< -o $@

bin/tools.o : src/tools.c src/chess.h
	cc -c $(CFLAGS) $< -o $@

bin/loop.o : src/loop.c src/chess.h
	cc -c $(CFLAGS) $< -o $@

bin/move.o : src/move.c src/chess.h src/hash.h
	cc -c $(CFLAGS) $< -o $@

bin/eval.o : src/eval.c src/actions.h src/chess.h
	cc -c $(CFLAGS) $< -o $@

bin/actions.o : src/actions.c src/actions.h src/chess.h
	cc -c $(CFLAGS) $< -o $@

bin/legal.o : src/legal.c src/actions.h src/chess.h
	cc -c $(CFLAGS) $< -o $@

bin/chess.o : src/chess.c src/chess.h
	cc -c $(CFLAGS) $< -o $@

bin/strike.o : src/strike.c src/chess.h
	cc -c $(CFLAGS) $< -o $@

clean :
	rm bin/*.o
	rm chess
