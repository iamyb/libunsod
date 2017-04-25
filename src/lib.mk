CPPFLAGS+=-I${TOPDIR}/network/uinet/lib/libuinet/api_include
CPPFLAGS+=-DHAVE_UINET=1

LDADD+=         -L${TOPDIR}/network/uinet/lib/libuinet
LDADD+=         -L${TOPDIR}/lib/libifdpdk/build/lib
LDADD+=         -L${TOPDIR}/lib/liblatprof/
LDADD+=		-luinet
LDADD+=		-lpcap
LDADD+=		-llatprof
LDADD+=     -Wl,--whole-archive -ldpdk_helper -Wl,--no-whole-archive
ifeq "${OSNAME}" "Linux"
LDADD+=		-lcrypto
else
LDADD+=		-lssl
endif
