CFLAGS=-O

OBJS = 	generate.o \
	tools.o \
	loop.o \
	move.o \
	eval.o \
	chess.o \
	actions.o \
	legal.o \
	strike.o

chess : $(OBJS)
	cc -o $@ $^

generate.o : generate.c actions.h chess.h
	cc -c $(CFLAGS) $<

tools.o : tools.c chess.h
	cc -c $(CFLAGS) $<

loop.o : loop.c chess.h
	cc -c $(CFLAGS) $<

move.o : move.c chess.h hash.h
	cc -c $(CFLAGS) $<

eval.o : eval.c actions.h chess.h
	cc -c $(CFLAGS) $<

actions.o : actions.c actions.h chess.h
	cc -c $(CFLAGS) $<

legal.o : legal.c actions.h chess.h
	cc -c $(CFLAGS) $<

chess.o : chess.c chess.h
	cc -c $(CFLAGS) $<

strike.o : strike.c chess.h
	cc -c $(CFLAGS) $<
