#ifndef _UD_SELECT_H 
#define _UD_SELECT_H

/* POSIX like APIs for select */

/* Check the first NFDS descriptors each in READFDS (if not NULL) for read
   readiness, in WRITEFDS (if not NULL) for write readiness, and in EXCEPTFDS
   (if not NULL) for exceptional conditions.  If TIMEOUT is not NULL, time out
   after waiting the interval specified therein.  Returns the number of ready
   descriptors, or -1 for errors. */
int ud_select(int nfds, fd_set *readfds, fd_set *writefds,
		      fd_set *exceptfds, struct timeval *timeout);

#endif