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
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include "ud_file.h"

int ud_select(int nfds, fd_set *readfds, fd_set *writefds,
		   fd_set *exceptfds, struct timeval *timeout)
{
	int retval = 0;
	int restarted = 0;
	int i = 0;
	fd_set res_read, res_write;
	FD_ZERO(&res_read);
	FD_ZERO(&res_write);	
	
RESTART:
	for(i = 0; i < nfds; i++)
	{
		if(readfds!=0 && FD_ISSET(i, readfds))
		{
			struct uinet_socket *so = ud_fd_get_sock(i);
			if(so == NULL){
				errno = EBADF;
				goto ERR;
			}			

			if(uinet_soreadable(so, 0) > 0)
			{
				FD_SET(i, &res_read);
				retval++;
			}
		}

		if(writefds!=0 && FD_ISSET(i, writefds))
		{
			struct uinet_socket *so = ud_fd_get_sock(i);
			if(so == NULL){
				errno = EBADF;
				goto ERR;
			}			

			if(uinet_sowritable(so, 0) > 0)
			{
				retval++;
				FD_SET(i, &res_write);
			}
		}
	}

	// LOOP inifinitely if timeout == NULL
	if(!retval && timeout == NULL)
		goto RESTART;	

	if(!restarted && !retval && (timeout->tv_sec != 0 || timeout->tv_usec != 0))
	{
		//clock_nanosleep
		if(clock_nanosleep(CLOCK_REALTIME, 0, timeout, NULL) != 0)
		{
			// ERRNO
			goto ERR;
		}
		
		restarted = 1;
		goto RESTART;	
	}

	if(retval > 0)
	{
		memcpy(readfds, &res_read, sizeof(fd_set));
		memcpy(writefds, &res_write, sizeof(fd_set));
		//FD_COPY(&res_read, readfds);
		//FD_COPY(&res_write, writefds);	
	}

	//printf("ud_selcet %d\n", retval);
	return retval;
ERR:
	return -1;
}