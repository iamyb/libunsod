/**
********************************************************************************
Copyright (C) 2017 b20yang 
---
This program is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free
Software Foundation; either version 3 of the License, or (at your option) any
later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program. If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************/
#include<stdint.h>
#include<string.h>
#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/mman.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<errno.h>
#include<time.h>

#include <uinet_api_errno.h>
#include "ud_error.h"
#include "ud_file.h"
#include "uinet_api.h"


/*bsd2linux*/
static inline int map_flags(int flags)
{
	int ret = 0;
	if(flags & MSG_DONTWAIT){
		flags &= (~MSG_DONTWAIT);
		ret |= UINET_MSG_DONTWAIT;
	}
#if 0
	if(flags & MSG_EOF){
		flags &= (~MSG_EOF);
		ret |= UINET_MSG_EOF;
	}

	if(flags & MSG_NBIO){
		flags &= (~MSG_NBIO);		
		ret |= UINET_MSG_NBIO;
	}

	if(flags & MSG_HOLE_BREAK){
		flags &= (~MSG_HOLE_BREAK);			
		ret |= UINET_MSG_HOLE_BREAK;
	}
#endif
	// flag can't be supported 
	if(flags != 0){
		return -1;
	}

	return ret;
}

#if 0
#define MSG_OOB         1
#define MSG_PEEK        2
#define MSG_DONTROUTE   4
#define MSG_TRYHARD     4       /* Synonym for MSG_DONTROUTE for DECnet */
#define MSG_CTRUNC      8
#define MSG_PROBE       0x10    /* Do not send. Only probe path f.e. for MTU */
#define MSG_TRUNC       0x20
#define MSG_DONTWAIT    0x40    /* Nonblocking io                */
#define MSG_EOR         0x80    /* End of record */
#define MSG_WAITALL     0x100   /* Wait for a full request */
#define MSG_FIN         0x200
#define MSG_SYN         0x400
#define MSG_CONFIRM     0x800   /* Confirm path validity */
#define MSG_RST         0x1000
#define MSG_ERRQUEUE    0x2000  /* Fetch message from error queue */
#define MSG_NOSIGNAL    0x4000  /* Do not generate SIGPIPE */
#define MSG_MORE        0x8000  /* Sender will send more */
#define MSG_WAITFORONE  0x10000 /* recvmmsg(): block until 1+ packets avail */
#define MSG_SENDPAGE_NOTLAST 0x20000 /* sendpage() internal : not the last page */
#define MSG_BATCH       0x40000 /* sendmmsg(): more messages coming */
#define MSG_EOF         MSG_FIN
 
#define MSG_FASTOPEN    0x20000000      /* Send data in TCP SYN */
#define MSG_CMSG_CLOEXEC 0x40000000     /* Set close_on_exec for file
                                            descriptor received through
                                            SCM_RIGHTS */
#if defined(CONFIG_COMPAT)
#define MSG_CMSG_COMPAT 0x80000000      /* This message needs 32 bit fixups */
#else
#define MSG_CMSG_COMPAT 0               /* We never have 32 bit fixups */
#endif

#define	MSG_OOB		0x1		/* process out-of-band data */
#define	MSG_PEEK	0x2		/* peek at incoming message */
#define	MSG_DONTROUTE	0x4		/* send without using routing tables */
#define	MSG_EOR		0x8		/* data completes record */
#define	MSG_TRUNC	0x10		/* data discarded before delivery */
#define	MSG_CTRUNC	0x20		/* control data lost before delivery */
#define	MSG_WAITALL	0x40		/* wait for full request or error */
#define MSG_NOTIFICATION 0x2000         /* SCTP notification */
#if __BSD_VISIBLE
#define	MSG_DONTWAIT	0x80		/* this message should be nonblocking */
#define	MSG_EOF		0x100		/* data completes connection */
#define	MSG_NBIO	0x4000		/* FIONBIO mode, used by fifofs */
#define	MSG_COMPAT      0x8000		/* used in sendit() */
#endif
#ifdef _KERNEL
#define	MSG_SOCALLBCK   0x10000		/* for use by socket callbacks - soreceive (TCP) */
#endif
#if __BSD_VISIBLE
#define	MSG_NOSIGNAL	0x20000		/* do not generate SIGPIPE on EOF */
#endif
#ifdef _KERNEL
#define	MSG_HOLE_BREAK	0x40000		/* stop at and indicate hole boundary */
#endif

#endif


static int errno_map[] = {
    0,
	EPERM,				  /* Operation not permitted */
	ENOENT,				  /* No such file or directory */
	ESRCH,				  /* No such process */
	EINTR,				  /* Interrupted system call */
	EIO,				  /* Input/output error */
	ENXIO,				  /* Device not configured */
	E2BIG,				  /* Argument list too long */
	ENOEXEC,			  /* Exec format error */
	EBADF,				  /* Bad file descriptor */
	ECHILD,               /* No child processes */
	EDEADLK,              /* Resource deadlock avoided */
	ENOMEM,               /* Cannot allocate memory */
	EACCES,               /* Permission denied */
	EFAULT,               /* Bad address */
	ENOTBLK,              /* Block device required */
	EBUSY,                /* Device busy */
	EEXIST,               /* File exists */
	EXDEV,                /* Cross-device link */
	ENODEV,               /* Operation not supported by device */
	ENOTDIR,              /* Not a directory */
	EISDIR,               /* Is a directory */
	EINVAL,               /* Invalid argument */
	ENFILE,               /* Too many open files in system */
	EMFILE,               /* Too many open files */
	ENOTTY,               /* Inappropriate ioctl for device */
	ETXTBSY,              /* Text file busy */
	EFBIG,                /* File too large */
	ENOSPC,               /* No space left on device */
	ESPIPE,               /* Illegal seek */
	EROFS,                /* Read-only filesystem */
	EMLINK,               /* Too many links */
	EPIPE,                /* Broken pipe */
	
	/* math software */
	EDOM,                 /* Numerical argument out of domain */
	ERANGE,               /* Result too large */
	
	/* non-blocking and interrupt i/o */
	EAGAIN,               /* Resource temporarily unavailable */
	EINPROGRESS,		  /* Operation now in progress */
	EALREADY,	    	  /* Operation already in progress */
	
	/* ipc/network software -- argument errors */
	ENOTSOCK,             /* Socket operation on non-socket */
	EDESTADDRREQ,         /* Destination address required */
	EMSGSIZE,       	  /* Message too long */
	EPROTOTYPE,           /* Protocol wrong type for socket */
	ENOPROTOOPT ,		  /* Protocol not available */
	EPROTONOSUPPORT,      /* Protocol not supported */
	ESOCKTNOSUPPORT,      /* Socket type not supported */
	EOPNOTSUPP,           /* Operation not supported */
	EOPNOTSUPP,           /* Operation not supported */
	EPFNOSUPPORT,         /* Protocol family not supported */
	EAFNOSUPPORT,         /* Address family not supported by protocol family */
	EADDRINUSE,           /* Address already in use */
	EADDRNOTAVAIL,        /* Can't assign requested address */
	
	/* ipc/network software -- operational errors */
	ENETDOWN,             /* Network is down */
	ENETUNREACH,          /* Network is unreachable */
	ENETRESET,	          /* Network dropped connection on reset */
	ECONNABORTED,         /* Software caused connection abort */
	ECONNRESET,  		  /* Connection reset by peer */
	ENOBUFS,              /* No buffer space available */
	EISCONN,              /* Socket is already connected */
	ENOTCONN,             /* Socket is not connected */
	ESHUTDOWN,            /* Can't send after socket shutdown */
	ETOOMANYREFS,         /* Too many references: can't splice */
	ETIMEDOUT,            /* Operation timed out */
	ECONNREFUSED,         /* Connection refused */
	
	ELOOP,                /* Too many levels of symbolic links */
	ENAMETOOLONG          /* File name too long */
};


static void ud_set_errno(int error)
{
	if(error != 0 && error < sizeof(errno_map))
	{
		errno = errno_map[error];
#ifdef UD_DEBUG
		printf("BSD error: %d LINUX: error: %d\n");
#endif
	}
}
//extern uinet_if_t ud_uif;


/*----------------------------------------------------------------------------*/
int ud_socket(int domain, int type, int protocol)
{
	int error;
	struct uinet_socket *so = NULL;
	error = uinet_socreate(uinet_instance_default(), 
		domain, &so, type, 0);

	if(!error)
		return ud_fd_set_sock(so);
	return error;
}

int ud_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	struct uinet_socket *newso = NULL;
	struct uinet_socket *so = ud_fd_get_sock(sockfd);
	if(so == NULL)
	{
//		printf("ppppppp %d\n", sockfd);
		return -1;
	}
	
	int error = uinet_soaccept(so, (struct uinet_sockaddr **)&addr, &newso);
	if(!error)
		return ud_fd_set_sock(newso);
	return error;
}

/// to convert the sockaddr in POSIX to BSD version sockaddr_in
int ud_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
	struct uinet_socket *so = ud_fd_get_sock(sockfd);
	struct sockaddr_in *iaddr = (struct sockaddr_in*)addr;
	struct uinet_sockaddr_in uaddr;

	uaddr.sin_len = sizeof(struct uinet_sockaddr);
	uaddr.sin_family = iaddr->sin_family;
	uaddr.sin_port = iaddr->sin_port;
	memcpy((void*)&uaddr.sin_addr, (void*)&iaddr->sin_addr, sizeof(uaddr.sin_addr));
	
	if(so != NULL)
		return uinet_sobind(so, (struct uinet_sockaddr *)&uaddr);
	return -1;
}

int ud_connect(int sockfd, const struct sockaddr *addr, socklen_t address_len)
{
	struct uinet_socket *so = ud_fd_get_sock(sockfd);
	if(so == NULL){
		errno = EBADF;
		goto ERR;
	}

	struct sockaddr_in *iaddr = (struct sockaddr_in*)addr;
	struct uinet_sockaddr_in uaddr;

	uaddr.sin_len = sizeof(struct uinet_sockaddr);
	uaddr.sin_family = iaddr->sin_family;
	uaddr.sin_port = iaddr->sin_port;
	memcpy((void*)&uaddr.sin_addr, (void*)&iaddr->sin_addr, sizeof(uaddr.sin_addr));

	if(so != NULL)
		return uinet_soconnect(so, (struct uinet_sockaddr *)&uaddr);
	errno = 0;
	return 0;
ERR:
	return -1;
}

ssize_t ud_recvfrom(int sockfd, void *buf, size_t len, int flags,
    struct sockaddr * __restrict from, socklen_t * __restrict fromlen)
{
	struct uinet_iovec iov;
	struct uinet_uio uio;
	int error;
	struct uinet_sockaddr_in* uaddr;		
	struct sockaddr_in *iaddr = (struct sockaddr_in*)from;

	struct uinet_socket *so = ud_fd_get_sock(sockfd);
	if(so == NULL){
		errno = EBADF;
		goto ERR;
	}

	/* prepare the uio struct */
	uio.uio_iov = &iov;
	iov.iov_base = buf;
	iov.iov_len = len;
	uio.uio_iovcnt = 1;
	uio.uio_offset = 0;
	uio.uio_resid = len;

	flags = map_flags(flags);
	if(flags < 0){
		ud_set_errno(UINET_EINVAL);
		goto ERR;		
	}

	error = uinet_soreceive(so, (struct uinet_sockaddr **)&uaddr, &uio, &flags);
	if(error != 0){
		/* need adapt between LINUX and FreeBsd*/
		ud_set_errno(error);
		goto ERR;
	}	

	if(from != NULL && uaddr != NULL){
		iaddr->sin_family = uaddr->sin_family;
		iaddr->sin_port = uaddr->sin_port;
		memcpy((void*)&iaddr->sin_addr, (void*)&uaddr->sin_addr, sizeof(uaddr->sin_addr));
		*fromlen = sizeof(struct sockaddr_in);

		// need to free this memory allocated in soreceive
		free(uaddr); 
	}

	errno = 0;
//	printf("fd, %d , read: %d \n", sockfd, len - uio.uio_resid);
	return (len - uio.uio_resid);	
ERR:
	return -1;
	
}

ssize_t ud_recv(int sockfd, void *buf, size_t len, int flags)
{
	return ud_recvfrom(sockfd, buf, len, flags, NULL, NULL);
}


ssize_t ud_recvmsg(int sockfd, struct msghdr *msg, int flags)
{
	
}


int ud_listen(int sockfd, int backlog)
{
	struct uinet_socket *so = ud_fd_get_sock(sockfd);
	if(so == NULL){
		errno = EBADF;
		return -1;
	}

	return uinet_solisten(so, backlog);
}

ssize_t ud_send(int sockfd, void *buf, size_t len, int flags)
{
	struct uinet_iovec iov;
	struct uinet_uio uio;
	
	uio.uio_iov = &iov;
	iov.iov_base = buf;
	iov.iov_len = len;
	uio.uio_iovcnt = 1;
	uio.uio_offset = 0;
	uio.uio_resid = len;	

	flags = map_flags(flags);
	if(flags < 0){
		ud_set_errno(UINET_EINVAL);
		goto ERR;		
	}
	
	struct uinet_socket *so = ud_fd_get_sock(sockfd);
	int error = uinet_sosend(so, NULL, &uio, flags);
	if(error != 0){
		/* need adapt between LINUX and FreeBsd*/
		ud_set_errno(error);
		goto ERR;
	}		
	errno = 0;
	return (len - uio.uio_resid);		
ERR:
	return -1;
}

ssize_t ud_sendto(int sockfd, const void *buf, size_t len, int flags,
			  const struct sockaddr *addr, socklen_t addrlen)
{
	struct uinet_iovec iov;
	struct uinet_uio uio;

	struct sockaddr_in *iaddr = (struct sockaddr_in*)addr;
	struct uinet_sockaddr_in uaddr;

	uaddr.sin_len = sizeof(struct uinet_sockaddr);
	uaddr.sin_family = iaddr->sin_family;
	uaddr.sin_port = iaddr->sin_port;
	memcpy((void*)&uaddr.sin_addr, (void*)&iaddr->sin_addr, sizeof(uaddr.sin_addr));

	
	uio.uio_iov = &iov;
	iov.iov_base = buf;
	iov.iov_len = len;
	uio.uio_iovcnt = 1;
	uio.uio_offset = 0;
	uio.uio_resid = len;	

	flags = map_flags(flags);
	if(flags < 0){
		ud_set_errno(UINET_EINVAL);
		goto ERR;		
	}
	
	struct uinet_socket *so = ud_fd_get_sock(sockfd);
	int error = uinet_sosend(so, (struct uinet_sockaddr *)&uaddr, &uio, flags);
	if(error != 0){
		/* need adapt between LINUX and FreeBsd*/
		printf("error %d \n", error);
		ud_set_errno(error);
		goto ERR;
	}		
	errno = 0;
	return (len - uio.uio_resid);		
ERR:
	return -1;	
	
}

ssize_t ud_sendmsg(int sockfd, const struct msghdr *msg, int flags)
{
}

#if 0
#define	LINUX_SO_DEBUG		1
#define	LINUX_SO_REUSEADDR	2
#define	LINUX_SO_TYPE		3
#define	LINUX_SO_ERROR		4
#define	LINUX_SO_DONTROUTE	5
#define	LINUX_SO_BROADCAST	6
#define	LINUX_SO_SNDBUF		7
#define	LINUX_SO_RCVBUF		8
#define	LINUX_SO_KEEPALIVE	9
#define	LINUX_SO_OOBINLINE	10
#define	LINUX_SO_NO_CHECK	11
#define	LINUX_SO_PRIORITY	12
#define	LINUX_SO_LINGER		13
#define	LINUX_SO_PEERCRED	17
#define	LINUX_SO_RCVLOWAT	18
#define	LINUX_SO_SNDLOWAT	19
#define	LINUX_SO_RCVTIMEO	20
#define	LINUX_SO_SNDTIMEO	21
#define	LINUX_SO_TIMESTAMP	29
#define	LINUX_SO_ACCEPTCONN	30

#define	UINET_SO_DEBUG		0x00000001	/* turn on debugging info recording */
#define	UINET_SO_ACCEPTCONN	0x00000002	/* socket has had listen() */
#define	UINET_SO_REUSEADDR	0x00000004	/* allow local address reuse */
#define	UINET_SO_KEEPALIVE	0x00000008	/* keep connections alive */
#define	UINET_SO_DONTROUTE	0x00000010	/* just use interface addresses */
#define	UINET_SO_BROADCAST	0x00000020	/* permit sending of broadcast msgs */
#define	UINET_SO_LINGER		0x00000080	/* linger on close if data present */
#define	UINET_SO_OOBINLINE	0x00000100	/* leave received OOB data in line */
#define	UINET_SO_REUSEPORT	0x00000200	/* allow local address & port reuse */
#define	UINET_SO_TIMESTAMP	0x00000400	/* timestamp received dgram traffic */
#define	UINET_SO_NOSIGPIPE	0x00000800	/* no SIGPIPE from EPIPE */
#define	UINET_SO_ACCEPTFILTER	0x00001000	/* there is an accept filter */
#define	UINET_SO_BINTIME	0x00002000	/* timestamp received dgram traffic */
#define	UINET_SO_NO_OFFLOAD	0x00004000	/* socket cannot be offloaded */
#define	UINET_SO_NO_DDP		0x00008000	/* disable direct data placement */
#define	UINET_SO_PROMISC	0x00010000	/* socket will be used for promiscuous listen */
#define	UINET_SO_PASSIVE	0x00020000	/* socket will be used for passive reassembly */

#define	UINET_SO_SNDBUF		0x1001		/* send buffer size */
#define	UINET_SO_RCVBUF		0x1002		/* receive buffer size */
#define	UINET_SO_SNDLOWAT	0x1003		/* send low-water mark */
#define	UINET_SO_RCVLOWAT	0x1004		/* receive low-water mark */
#define	UINET_SO_SNDTIMEO	0x1005		/* send timeout */
#define	UINET_SO_RCVTIMEO	0x1006		/* receive timeout */
#define	UINET_SO_ERROR		0x1007		/* get error status and clear */
#define	UINET_SO_TYPE		0x1008		/* get socket type */
#define	UINET_SO_LABEL		0x1009		/* socket's MAC label */
#define	UINET_SO_PEERLABEL	0x1010		/* socket's peer's MAC label */
#define	UINET_SO_LISTENQLIMIT	0x1011		/* socket's backlog limit */
#define	UINET_SO_LISTENQLEN	0x1012		/* socket's complete queue length */
#define	UINET_SO_LISTENINCQLEN	0x1013		/* socket's incomplete queue length */
#define	UINET_SO_SETFIB		0x1014		/* use this FIB to route */
#define	UINET_SO_USER_COOKIE	0x1015		/* user cookie (dummynet etc.) */
#define	UINET_SO_PROTOCOL	0x1016		/* get socket protocol (Linux name) */
#define	UINET_SO_PROTOTYPE	UINET_SO_PROTOCOL	/* alias for UINET_SO_PROTOCOL (SunOS name) */
#define UINET_SO_L2INFO		0x1017		/* PROMISCUOUS_INET MAC addrs and tags */


#endif

static unsigned int ud_soopts[30] = 
{
	0,
	UINET_SO_DEBUG,
	UINET_SO_REUSEADDR,
	UINET_SO_TYPE,
	UINET_SO_ERROR,
	UINET_SO_DONTROUTE,
	UINET_SO_BROADCAST,
	UINET_SO_SNDBUF,
	UINET_SO_RCVBUF,
	UINET_SO_KEEPALIVE,
	UINET_SO_OOBINLINE,
	0,
	0,
	UINET_SO_LINGER
	};

#if 0
#define TCP_NODELAY		1	    /* Turn off Nagle's algorithm. */
#define TCP_MAXSEG		2	    /* Limit MSS */
#define TCP_CORK		3	    /* Never send partially complete segments */
#define TCP_KEEPIDLE    4	/* Start keeplives after this period */
#define TCP_KEEPINTVL	5	/* Interval between keepalives */
#define TCP_KEEPCNT		6	/* Number of keepalives before death */
#define TCP_SYNCNT		7	/* Number of SYN retransmits */
#define TCP_LINGER2		8	/* Life time of orphaned FIN-WAIT-2 state */
#define TCP_DEFER_ACCEPT	9	/* Wake up listener only when data arrive */
#define TCP_WINDOW_CLAMP	10	/* Bound advertised window */
#define TCP_INFO		11	/* Information about this connection. */
#define TCP_QUICKACK		12	/* Block/reenable quick acks */
#define TCP_CONGESTION		13	/* Congestion control algorithm */
#define TCP_MD5SIG		    14	/* TCP MD5 Signature (RFC2385) */
#define TCP_THIN_LINEAR_TIMEOUTS 16      /* Use linear timeouts for thin streams*/
#define TCP_THIN_DUPACK         17      /* Fast retrans. after 1 dupack */
#define TCP_USER_TIMEOUT	18	/* How long for loss retry before timeout */
#define TCP_REPAIR		19	/* TCP sock is under repair right now */
#define TCP_REPAIR_QUEUE	20
#define TCP_QUEUE_SEQ		21
#define TCP_REPAIR_OPTIONS	22
#define TCP_FASTOPEN		23	/* Enable FastOpen on listeners */
#define TCP_TIMESTAMP		24
#define TCP_NOTSENT_LOWAT	25	/* limit number of unsent bytes in write queue */
#define TCP_CC_INFO		26	/* Get Congestion Control (optional) info */
#define TCP_SAVE_SYN		27	/* Record SYN headers for new connections */
#define TCP_SAVED_SYN		28	/* Get SYN headers recorded for connection */
#define TCP_REPAIR_WINDOW	29	/* Get/set window parameters */
#define TCP_FASTOPEN_CONNECT	30	/* Attempt FastOpen with connect */
#endif

static unsigned int ud_tcpopts[31] = 
{
	0,//0
	UINET_TCP_NODELAY,//1/
	UINET_TCP_MAXSEG,//2/
	0,//3/
	UINET_TCP_KEEPIDLE,//4/
	UINET_TCP_KEEPINTVL,//5/
	UINET_TCP_KEEPCNT,//6/
	0,//7/
	0,//8/
	0,//9/
	0,//10/
	UINET_TCP_INFO,//11
	0,//12
	UINET_TCP_CONGESTION,//13/
	UINET_TCP_MD5SIG,//14
    0,//15
    0,//16
	0,//18
	0,//19
	0,//20
    0,//21
    0,//22
    0,//23
    0,//24
    0,//25
    0,//26
    0,//27
    0,//28
    0,//29
    0//30
};

static int ud_maplevelopt(int *level, int *opt)
{
	if(*level == 1) /* SOCKET  */
	{
		*level = UINET_SOL_SOCKET;
		if(*opt > sizeof(ud_tcpopts) || ud_soopts[*opt] == 0){
			printf("invalid opt !\n");
			goto ERR;
		}

		*opt = ud_soopts[*opt];
	}
	else if(*level == 6) /* tcp*/
	{
		*level = UINET_SOL_SOCKET;
		if(*opt > sizeof(ud_tcpopts) || ud_tcpopts[*opt] == 0){
			printf("invalid opt !\n");
			goto ERR;
		}

		*opt = ud_tcpopts[*opt];
	}
	return 0;
ERR:
	return -1;
}

int ud_setsockopt(int sockfd, int level, int optname,
	const void *optval, socklen_t optlen)
{
	struct uinet_socket *so = ud_fd_get_sock(sockfd);
	if(so == NULL){
		errno = EBADF;
		goto ERR;
	}

	if((ud_maplevelopt(&level, &optname)) < 0)
	{
		printf("invalid level !\n");		
		goto ERR;
	}

	int error = uinet_sosetsockopt(so, level, optname, optval, optlen);
	if(error != 0){
		ud_set_errno(UINET_EINVAL);
		goto ERR;
	}

	return error;
ERR:
	return -1;
	
}

int ud_getsockopt(int sockfd, int level, int optname,
	void *optval, socklen_t *optlen)
{
	struct uinet_socket *so = ud_fd_get_sock(sockfd);
	if(so == NULL){
		errno = EBADF;
		goto ERR;
	}

	if((ud_maplevelopt(&level, &optname)) < 0)
	{
		printf("invalid level !\n");		
		goto ERR;
	}

	int error = uinet_sogetsockopt(so, level, optname, optval, optlen);
	if(error != 0){
		ud_set_errno(UINET_EINVAL);
		goto ERR;
	}

	return error;
ERR:
	return -1;
}

int ud_shutdown(int sockfd, int how)
{
	return -1;
}

int ud_getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	struct uinet_sockaddr_in* uaddr;		
	struct sockaddr_in *iaddr = (struct sockaddr_in*)addr;
	
	struct uinet_socket *so = ud_fd_get_sock(sockfd);
	if(so == NULL){
		errno = EBADF;
		goto ERR;
	}
	
	int error = uinet_sogetsockaddr(so,(struct uinet_sockaddr **)&uaddr);
	if(error != 0){
		goto ERR;
	}
	
	if(addr!= NULL && uaddr != NULL){
		iaddr->sin_family = uaddr->sin_family;
		iaddr->sin_port = uaddr->sin_port;
		memcpy((void*)&iaddr->sin_addr, (void*)&uaddr->sin_addr, sizeof(uaddr->sin_addr)); 
		addrlen = sizeof(struct sockaddr_in);
		 
		free(uaddr); 
	}

	return 0;
ERR:
	return -1;
}


int	ud_getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	struct uinet_sockaddr_in* uaddr;		
	struct sockaddr_in *iaddr = (struct sockaddr_in*)addr;
	
	struct uinet_socket *so = ud_fd_get_sock(sockfd);
	if(so == NULL){
		errno = EBADF;
		goto ERR;
	}
	
	int error = uinet_sogetsockaddr(so,(struct uinet_sockaddr **)&uaddr);
	if(error != 0){
		goto ERR;
	}
	
	if(addr!= NULL && uaddr != NULL){
		iaddr->sin_family = uaddr->sin_family;
		iaddr->sin_port = uaddr->sin_port;
		memcpy((void*)&iaddr->sin_addr, (void*)&uaddr->sin_addr, sizeof(uaddr->sin_addr)); 
		*addrlen = sizeof(struct sockaddr_in);
		 
		free(uaddr); 
	}

	return 0;
ERR:
	return -1;
}



