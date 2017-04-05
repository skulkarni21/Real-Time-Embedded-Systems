INCLUDE_DIRS =
LIB_DIRS =
CC=gcc

CDEFS=
CFLAGS= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS=

HFILES=
CFILES= Exercise_1.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	Exercise_1

clean:
	-rm -f *.o *.d
	-rm -f Exercise_1

Exercise_1: Exercise_1.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o -lpthread

depend:

.c.o:
	$(CC) $(CFLAGS) -c $<
