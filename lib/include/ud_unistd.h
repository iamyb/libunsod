/**
********************************************************************************
Copyright (C) 2016 b20yang
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
#ifndef _UD_UNISTD_H
#define _UD_UNISTD_H

/* POSIX like APIs for write/read/close */

/* write() writes up to count bytes from the buffer pointed buf
   to the file referred to by the file descriptor fd. */
ssize_t ud_write(int sockfd, void *buf, size_t count);

/* read() attempts to read up to count bytes from file descriptor
   fd into the buffer starting at buf */
ssize_t ud_read(int sockfd, void *buf, size_t count);

/* close()  closes  a  file  descriptor,  so that it no longer
   refers to any file and may be reused. */
int ud_close(int sockfd);

/* fcntl() performs one of the operations described below on the
   open file descriptor fd.  The operation is determined by cmd. */
int ud_fcntl(int sockfd, int cmd, ... /* arg */ );

#endif