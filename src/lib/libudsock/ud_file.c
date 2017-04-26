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
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>        
#include <fcntl.h>       
#include <uinet_api_errno.h>
#include "uinet_api.h"
#include "ud_error.h"

#define UD_SOCKET_DESC_MAX 65516
#define UDSOCK_SHM_NAME "/udsock_shm"
typedef struct ud_fds_table
{
	int num;
	struct uinet_socket* fds[UD_SOCKET_DESC_MAX];
}ud_fds_table;


static ud_fds_table *fds_table=NULL;


#define UDSOCK_SHM_SIZE (sizeof(ud_fds_table))

/*----------------------------------------------------------------------------*/
static void __attribute__((constructor))ud_fd_create_shm(void)
{
    int fd = shm_open(UDSOCK_SHM_NAME, O_CREAT|O_RDWR|O_EXCL,0666);
    if(fd == -1){
        if((fd = shm_open(UDSOCK_SHM_NAME, O_RDWR, 0666))==-1){
				("shm_open");
        }
    }

    if(ftruncate(fd, UDSOCK_SHM_SIZE) == -1)
        handle_error("ftruncate");

    uint64_t *ptr = 
        mmap(NULL, UDSOCK_SHM_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(ptr == MAP_FAILED)
        handle_error("mmap");

    memset(ptr, 0, UDSOCK_SHM_SIZE);
    fds_table = (ud_fds_table *)ptr;
}

static void __attribute__((destructor))ud_fd_destroy_shm(void)
{
    if(munmap((void*)fds_table, UDSOCK_SHM_SIZE) == -1)
        handle_error("munmap");
    fds_table = NULL;
    if(shm_unlink(UDSOCK_SHM_NAME) == -1);
}

int ud_fd_get_free(void)
{
	int i = 1;
	for(; i < UD_SOCKET_DESC_MAX; i++)
	{
		if(fds_table->fds[i]==NULL) break;
	}
	return i;
}

struct uinet_socket* ud_fd_get_sock(int fd)
{
	return fds_table->fds[fd];
}

int ud_fd_set_sock(struct uinet_socket* sock)
{
	int fd = ud_fd_get_free();
	if(fd != -1)
		fds_table->fds[fd] = sock;

	return fd;
}

void ud_fd_free(int fd)
{
	fds_table->fds[fd] = NULL;
}

