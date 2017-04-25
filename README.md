#libunsod (being developed...)
libunsod is an enhanced user space network stack based on libuinet and DPDK, short for Userspace Network Stack over DPDK(libunsod). 

### Inteface
libunsod provides POSIX compatible API. Application can be integrated with libunsod with quite small changes. Part of those API listed as below. For the detailed infomation, please refer to the header files inside ./lib/include

	int ud_socket(int domain, int type, int protocol);  
	int ud_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);  
	int ud_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);    
	size_t ud_send(int sockfd, void *buf, size_t len, int flags);    
	ssize_t ud_recv(int sockfd, void *buf, size_t len, int flags);  
	ssize_t ud_recvmsg(int sockfd, struct msghdr *msg, int flags);  
	int ud_listen(int sockfd, int backlog);


### License
>This program is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.  
>This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have received a copy of the GNU Lesser General Public License along with this program. If not, see http://www.gnu.org/licenses/.