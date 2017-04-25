#ifndef _UD_SOCK_H
#define _UD_SOCK_H
#include <netinet/in.h>
#include "ud_unistd.h" 

/* Create a new socket of type TYPE in domain DOMAIN, using
   protocol PROTOCOL.  If PROTOCOL is zero, one is chosen automatically.
   Returns a file descriptor for the new socket, or -1 for errors.  */
int ud_socket(int domain, int type, int protocol);

/* Give the socket FD the local address ADDR (which is LEN bytes long).  */
int ud_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

/* Open a connection on socket FD to peer at ADDR (which LEN bytes long).
   For connectionless socket types, just set the default address to send to
   and the only address from which to accept transmissions.
   Return 0 on success, -1 for errors.*/
int ud_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

/* Send N bytes of BUF to socket FD.  Returns the number sent or -1.*/
ssize_t ud_send(int sockfd, void *buf, size_t len, int flags);

/* Send N bytes of BUF to socket FD.  Returns the number sent or -1.*/
ssize_t ud_recv(int sockfd, void *buf, size_t len, int flags);

/* Send N bytes of BUF on socket FD to peer at address ADDR (which is
   ADDR_LEN bytes long).  Returns the number sent, or -1 for errors. */
ssize_t ud_sendto(int sockfd, const void *buf, size_t len, int flags,
			  const struct sockaddr *addr, socklen_t addrlen);

/* Read N bytes into BUF through socket FD.
   If ADDR is not NULL, fill in *ADDR_LEN bytes of it with tha address of
   the sender, and store the actual size of the address in *ADDR_LEN.
   Returns the number of bytes read or -1 for errors. */
ssize_t ud_recvfrom(int sockfd, void *buf, size_t len, int flags,
    struct sockaddr * __restrict from, socklen_t * __restrict fromlen);

/* Send a message described MESSAGE on socket FD.
   Returns the number of bytes sent, or -1 for errors. */
ssize_t ud_sendmsg(int sockfd, const struct msghdr *msg, int flags);

/* Receive a message as described by MESSAGE from socket FD.
   Returns the number of bytes read or -1 for errors.*/
ssize_t ud_recvmsg(int sockfd, struct msghdr *msg, int flags);

/* Put the current value for socket FD's option OPTNAME at protocol level LEVEL
   into OPTVAL (which is *OPTLEN bytes long), and set *OPTLEN to the value's
   actual length.  Returns 0 on success, -1 for errors.  */
int ud_getsockopt(int sockfd, int level, int optname,
	void *optval, socklen_t *optlen);

/* Set socket FD's option OPTNAME at protocol level LEVEL
   to *OPTVAL (which is OPTLEN bytes long).
   Returns 0 on success, -1 for errors.  */
int ud_setsockopt(int sockfd, int level, int optname,
	const void *optval, socklen_t optlen);

/* Prepare to accept connections on socket FD.
   N connection requests will be queued before further requests are refused.
   Returns 0 on success, -1 for errors.  */
int ud_listen(int sockfd, int backlog);

/* Await a connection on socket FD.
   When a connection arrives, open a new socket to communicate with it,
   set *ADDR (which is *ADDR_LEN bytes long) to the address of the connecting
   peer and *ADDR_LEN to the address's actual length, and return the
   new socket's descriptor, or -1 for errors.*/
int ud_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

/* Shut down all or part of the connection open on socket FD.
   HOW determines what to shut down:
      SHUT_RD   = No more receptions;
      SHUT_WR   = No more transmissions;
      SHUT_RDWR = No more receptions or transmissions.
    Returns 0 on success, -1 for errors.  */
int shutdown(int sockfd, int how);
#endif
