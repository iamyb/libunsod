TOPDIR?=${CURDIR}/../..
include ${TOPDIR}/lib.mk
include ${TOPDIR}/cflags.mk

CFLAGS+=	${DEBUG_FLAGS} -I../libuinet/api_include -I../../../lib/include

SRCS=	ud_ifconfig.c
OBJS=	ud_ifconfig.o

all: libudif.a

libudif.a: $(OBJS)
	$(AR) -c -r libudif.a $(OBJS)

clean:
	$(RM) $(OBJS) libudif.a