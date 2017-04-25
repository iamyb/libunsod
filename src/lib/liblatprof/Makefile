TOPDIR?=${CURDIR}/../..
include ${TOPDIR}/lib.mk
include ${TOPDIR}/cflags.mk

CFLAGS+=	${DEBUG_FLAGS}

SRCS=	latprof.c
OBJS=	latprof.o
INCS=	ytimestamp.h

all: liblatprof.a

liblatprof.a: $(OBJS)
	$(AR) -c -r liblatprof.a $(OBJS)

clean:
	$(RM) $(OBJS) liblatprof.a 