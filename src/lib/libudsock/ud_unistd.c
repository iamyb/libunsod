
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
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "ud_socket.h"
#include "ud_file.h"

ssize_t ud_write(int fd, void *buf, size_t count)
{
    return ud_send(fd, buf, count, 0);
}

ssize_t ud_read(int fd, void *buf, size_t count)
{
    return ud_recvfrom(fd, buf, count, 0, NULL, NULL);
}

int ud_fcntl(int fd, int cmd, ... /* arg */ )
{
    struct uinet_socket *so = ud_fd_get_sock(fd);
    if(so == NULL) {
        errno = EBADF;
        goto ERR;
    }

    uinet_sosetnonblocking(so, 1);
ERR:
    return -1;
}

int ud_close(int sockfd)
{
    struct uinet_socket *so = ud_fd_get_sock(sockfd);
    ud_fd_free(sockfd);

    return uinet_soclose(so);
}

