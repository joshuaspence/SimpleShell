CC=gcc
CFLAGS=-g -O2 -Wall
LD=gcc
LDFLAGS=
OBJS=list.o ringbuf.o
LIBS=

all: stripcc

stripcc: stripcc.o ${OBJS}
	${LD} ${LDFLAGS} -o stripcc stripcc.o ${OBJS} ${LIBS}

stripcc.o: stripcc.c list.h ringbuf.h
	${CC} ${CFLAGS} -c stripcc.c

list.o: list.c list.h
	${CC} ${CFLAGS} -c list.c

ringbuf.o: ringbuf.c ringbuf.h
	${CC} ${CFLAGS} -c ringbuf.c

install: stripcc
	-cp stripcc /usr/local/bin

uninstall:
	-rm -f /usr/local/bin/stripcc

clean: 
	rm -f *.o stripcc
