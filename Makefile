CC=$(CROSS)gcc
CFLAGS+=-Wall -I. -I../ -g
CFLAGS+=-rdynamic -DVTY_SIMPLE_NODE
LIBS=-lreadline -lcrypt -lncurses
STRIP=$(CROSS)strip
TARGETDIR=/usr/local/bin

all:mksh
OBJECT=${patsubst %.c, %.o, ${wildcard *.c cmd/*.c}}
mksh:${OBJECT}
	${CC} ${CFLAGS} -o $@ $^ ${LIBS}
	#${STRIP} $@

install:mksh
	#${STRIP} mksh
	rm -f ${TARGETDIR}/mksh
	cp mksh ${TARGETDIR}/
.c.o:
.c.h:
clean:
	rm -f *.o cmd/*.o mksh
