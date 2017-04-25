#
# Derived from FreeBSD src/share/mk/bsd.prog.mk
#

ifdef DEBUG_FLAGS
CFLAGS+=${DEBUG_FLAGS}
CXXFLAGS+=${DEBUG_FLAGS}
endif

ifdef NO_SHARED
ifneq (${NO_SHARED},no)
ifneq (${NO_SHARED},NO)
LDFLAGS+= -static
endif
endif
endif

ifdef PROG_CXX
PROG=	${PROG_CXX}
endif

ifndef PROG
$(error  PROG or PROG_CXX must be defined.)
endif

ifndef SRCS
ifdef PROG_CXX
SRCS=	${PROG}.cc
else
SRCS=	${PROG}.c
endif
endif

OBJS+= $(patsubst %.cc,%.o,$(patsubst %.c,%.o,${SRCS}))
#UINET_LIBS+=dpdk_helper

#
# Include Makefile.inc from each UINET library that is being used and
# set up the compiler and linker options for finding and linking to
# each one.
#
UINET_LIB_PATHS:= $(foreach lib,${UINET_LIBS},${TOPDIR}/lib/lib$(lib))
UINET_LIB_INCS:= $(foreach libpath,${UINET_LIB_PATHS},$(libpath)/Makefile.inc)
UINET_CFLAGS:= $(foreach lib,${UINET_LIBS}, -I${TOPDIR}/lib/lib$(lib)$(if $(wildcard ${TOPDIR}/lib/lib$(lib)/api_include),/api_include))


-include ${UINET_LIB_INCS}

#
# Including UINET_LIB_INCS may have added to UINET_LIBS in order to
# pick up additional link flags, so rebuild UINET_LIB_PATHS before
# building UINET_LDADD.
#
UINET_LIB_PATHS:= $(foreach lib,${UINET_LIBS},${TOPDIR}/lib/lib$(lib))
UINET_LDADD:= $(foreach lib,${UINET_LIBS}, ${TOPDIR}/lib/lib$(lib)$(if $(wildcard ${TOPDIR}/lib/lib$(lib)/.libs),/.libs)/lib$(lib).a)

#========================================================================
# to support dpdk
#========================================================================
UINET_LDADD+=-L${RTE_SDK}/${RTE_TARGET}/lib
UINET_LDADD+=-L${TOPDIR}/lib/libdpdk_helper/build/lib
UINET_LDADD+=-L${TOPDIR}/lib/liblatprof
UINET_LDADD+=-Wl,--whole-archive
UINET_LDADD+=${TOPDIR}/lib/libdpdk_helper/libdpdk_helper.a
UINET_LDADD+=${TOPDIR}/lib/liblatprof/liblatprof.a
UINET_LDADD+=${RTE_SDK}/${RTE_TARGET}/lib/librte_mbuf.a
UINET_LDADD+=${RTE_SDK}/${RTE_TARGET}/lib/libethdev.a
UINET_LDADD+=${RTE_SDK}/${RTE_TARGET}/lib/librte_mempool.a 
UINET_LDADD+=${RTE_SDK}/${RTE_TARGET}/lib/librte_ring.a 
UINET_LDADD+=${RTE_SDK}/${RTE_TARGET}/lib/librte_eal.a

#UINET_LDADD+=${RTE_SDK}/${RTE_TARGET}/lib/librte_pmd_af_packet.a
#UINET_LDADD+=${RTE_SDK}/${RTE_TARGET}/lib/librte_pmd_bnx2x.a -lz
UINET_LDADD+=${RTE_SDK}/${RTE_TARGET}/lib/librte_pmd_bnxt.a
UINET_LDADD+=${RTE_SDK}/${RTE_TARGET}/lib/librte_pmd_cxgbe.a
UINET_LDADD+=${RTE_SDK}/${RTE_TARGET}/lib/librte_pmd_e1000.a
UINET_LDADD+=${RTE_SDK}/${RTE_TARGET}/lib/librte_pmd_ena.a
#UINET_LDADD+=${RTE_SDK}/${RTE_TARGET}/lib/librte_pmd_enic.a
#UINET_LDADD+=${RTE_SDK}/${RTE_TARGET}/lib/librte_pmd_fm10k.a
#UINET_LDADD+=${RTE_SDK}/${RTE_TARGET}/lib/librte_pmd_i40e.a
UINET_LDADD+=${RTE_SDK}/${RTE_TARGET}/lib/librte_pmd_ixgbe.a
#UINET_LDADD+=${RTE_SDK}/${RTE_TARGET}/lib/librte_pmd_ring.a
#UINET_LDADD+=${RTE_SDK}/${RTE_TARGET}/lib/librte_pmd_virtio.a
#UINET_LDADD+=${RTE_SDK}/${RTE_TARGET}/lib/librte_pmd_vhost.a

UINET_LDADD+=-Wl,--no-whole-archive

UINET_LDADD+=-ldl
#UINET_LDADD+=-llibdpdk.a
#UINET_LDADD+=-llibrte_eal.a
#UINET_LDADD+=-llibrte_ether.a
#UINET_LDADD+=-llibrte_mbuf.a
#UINET_LDADD+=-llibrte_mempool.a
UINET_LIB_PATHS+=${RTE_SDK}/${RTE_TARGET}/lib

#========================================================================
CFLAGS+= ${UINET_CFLAGS}
CXXFLAGS+= ${UINET_CFLAGS}

${PROG}: ${OBJS}
ifdef PROG_CXX
	${CXX} ${CXXFLAGS} ${LDFLAGS} -o $@ ${OBJS} ${UINET_LDADD} ${LDADD} 
else
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ ${OBJS} ${UINET_LDADD} ${LDADD}
endif


clean:
	@rm -f ${PROG} ${OBJS}

all: ${PROG}

install:
	${UINET_INSTALL_DIR} -d ${UINET_DESTDIR}/bin
	${UINET_INSTALL_BIN} ${PROG} ${UINET_DESTDIR}/bin

config:
