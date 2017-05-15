#ifndef UINET_HOST_NETSTAT_API_H
#define UINET_HOST_NETSTAT_API_H

#define NS_TCPSTAT 0xA001
#define NS_UDPSTAT 0xA002
#define NS_IFSTAT  0xA003
#define NS_IPSTAT  0xA004

struct ns_netstat_req {
    unsigned int type;    // type of the request
    unsigned int data[1]; // payload of this req
};


#define	UINET_SYSCTL_MAXPATHLEN		1024

struct uinet_host_netstat_cfg {
    char netstat_sock_path[UINET_SYSCTL_MAXPATHLEN];
};

void *uinet_host_netstat_listener_thread(void *arg);
#endif