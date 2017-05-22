#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "ud_ifconfig.h"
#include "ud_socket.h"

#define BUFFER_SIZE (2*1024)
static char buffer[BUFFER_SIZE];

/*---------------------------------------------------------------------------*/
static void* echo_cb(void* fd)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(4, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

    while(1) {
        int rcv_size = ud_recv(fd, buffer, BUFFER_SIZE, MSG_DONTWAIT);
        if(errno == EAGAIN)
            continue;

        if(rcv_size < 0) {
            printf("read error (%d), closing\n", rcv_size);
            goto done;
        }

        if(rcv_size > 0) {
            int snd_size = ud_send(fd, buffer, rcv_size, 0);
            if (snd_size < 0) {
                printf("write error (%d), closing\n", snd_size);
                goto done;
            }
        }
    }
done:
    ud_close(fd);
}

/*---------------------------------------------------------------------------*/
int main(int argc, char** argv)
{
    int error;
    struct ud_ifcfg
        ifcfg= {"eth1", "192.168.1.188", "255.255.255.0", "255.255.255.255"};

    if((error = ud_ifsetup(&ifcfg)) != 0) {
        printf("if create/up failed %d\n", error);
        goto done;
    }

    struct sockaddr_in sin;
    struct in_addr addr;

    char* ip_addr = "192.168.1.188";
    if (inet_pton(AF_INET, ip_addr, &addr) <= 0) {
        printf("Invalid address %s\n", ip_addr);
        goto done;
    }

    int sockfd = ud_socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("Listen socket creation failed (%d)\n", sockfd);
        goto done;
    }

    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_addr = addr;
    sin.sin_port = htons(11111);
    error = ud_bind(sockfd, (struct sockaddr *)&sin, sizeof(struct sockaddr));
    if (0 != error) {
        printf("bind failed %d\n", error);
        goto done;
    }

    error = ud_listen(sockfd, -1);
    if (0 != error)
        goto done;

    while(1) {
        int newfd = ud_accept(sockfd, NULL, 0);
        if (newfd < 0) {
            printf("accept failed (%d)\n", newfd);
            goto done;
        }

        pthread_t tid;
        pthread_attr_t tattr;
        pthread_attr_init(&tattr);
        pthread_attr_setschedpolicy(&tattr, SCHED_FIFO);

        if(pthread_create(&tid, &tattr, echo_cb, (void*)newfd)) {
            printf("phread create failed\n");
            goto done;
        }
    }

done:
    ud_ifclose("eth1");
    return 0;
}
