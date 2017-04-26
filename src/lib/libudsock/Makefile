TOPDIR?=${CURDIR}/../..
include ${TOPDIR}/lib.mk
include ${TOPDIR}/cflags.mk

CFLAGS+= ${DEBUG_FLAGS} -I../libuinet/api_include -I../../../lib/include

SRCS=ud_socket.c ud_select.c ud_unistd.c ud_file.c
OBJS=ud_socket.o ud_select.o ud_unistd.o ud_file.o

all: libudsock.a

libudsock.a: $(OBJS)
	$(AR) -c -r libudsock.a $(OBJS)

clean:
	$(RM) $(OBJS) libudsock.a