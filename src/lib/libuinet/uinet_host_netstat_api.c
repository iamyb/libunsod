/**
********************************************************************************
Copyright (C) 2017 billyang
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
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/mman.h>

#include "uinet_api.h"
#include "uinet_api_types.h"
#include "uinet_host_netstat_api.h"

/*---------------------------------------------------------------------------*/
int handle_netstat(int fd)
{
    struct ns_netstat_req req;
    struct uinet_tcpstat tcpstat;
    struct uinet_ifstat ifstat;
    struct uinet_ipstat ipstat;    
    int ret = 0;

    int n = recv(fd, &req, sizeof(req), MSG_PEEK);

    switch(req.type) {
    case NS_TCPSTAT:
        uinet_gettcpstat(uinet_instance_default(), &tcpstat);

        n = send(fd, &tcpstat, sizeof(tcpstat), 0);
        if(n != sizeof(tcpstat)) {
            printf("TCPSTAT incorrect number bytes sent\n");
            ret = -1;
        }
        break;
    case NS_IFSTAT:
        uinet_getifstat(udif_getuif(), &ifstat);
        n = send(fd, &ifstat, sizeof(ifstat), 0);
        if(n != sizeof(ifstat)) {
            printf("IFSTAT incorrect number bytes sent\n");
            ret = -1;
        }
        break;
    case NS_IPSTAT:
        uinet_getipstat(uinet_instance_default(), &ipstat);
        n = send(fd, &ipstat, sizeof(ipstat), 0);
        if(n != sizeof(ipstat)) {
            printf("IPSTAT incorrect number bytes sent\n");
            ret = -1;
        }
        break;        
    default:
        printf("unknow type received \n");
        ret = -1;
    }

    return ret;
}

/*---------------------------------------------------------------------------*/
void *
uinet_host_netstat_listener_thread(void *arg)
{
    int s, r;
    struct sockaddr_un sun;
    struct uinet_host_netstat_cfg *cfg = arg;
    char *path;

    path = "/tmp/sysctl.sock";
    if (cfg) {
        path = cfg->netstat_sock_path;
    }
    uinet_initialize_thread("sysctl");

    (void) unlink(path);

    bzero(&sun, sizeof(sun));
    strcpy(sun.sun_path, path);
    sun.sun_family = AF_UNIX;

    printf("libunsod: starting listener on %s\n", sun.sun_path);
    s = socket(AF_UNIX, SOCK_STREAM, 0);
    if (s < 0) {
        fprintf(stderr, "%s: socket failed: %d\n", __func__, errno);
        return NULL;
    }

    r = bind(s, (struct sockaddr *) &sun, sizeof(sun));
    if (r < 0) {
        fprintf(stderr, "%s: bind failed: %d\n", __func__, errno);
        return NULL;
    }

    r = listen(s, 10);
    if (r < 0) {
        fprintf(stderr, "%s: listen failed: %d\n", __func__, errno);
        return NULL;
    }

    for (;;) {
        struct sockaddr_un sun_n;
        socklen_t sl;
        int ns;

        ns = accept(s, (struct sockaddr *) &sun_n, &sl);
        if (ns < 0) {
            fprintf(stderr, "%s: accept failed: %d\n", __func__, errno);
            continue;
        }

        if(handle_netstat(ns) == -1) {
            fprintf(stderr, "%s: handle netstat failed: %d\n", __func__, errno);
        }
        /* Done; bail */
        close(ns);
    }

    return NULL;
}
