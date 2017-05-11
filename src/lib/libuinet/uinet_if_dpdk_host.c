/*
 * Copyright (c) 2015 Patrick Kelsey. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(__linux__)
/*
 * To expose required facilities in net/if.h.
 */
#define _GNU_SOURCE
#endif /* __linux__ */
#include <netinet/in.h>
#include <errno.h>
#include <poll.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
//#include <pcap/pcap.h>
#include <netinet/if_ether.h>
#include "uinet_host_interface.h"
#include "uinet_if_dpdk_host.h"

struct if_dpdk_host_context {
//	pcap_t *rx_p;
	int rx_isfile;
//	pcap_t *tx_p;
	int tx_isfile;
	unsigned int tx_snaplen;
	unsigned int tx_file_per_flow;
	unsigned int tx_dirbits;
	uint32_t tx_epoch_no;
	uint32_t tx_instance_index;
	const char *rx_ifname;
	const char *tx_ifname;
	uint64_t last_packet_delivery;
	uint64_t last_packet_timestamp;		
#define PATH_BUFFER_SIZE 1024
	char path_buffer[PATH_BUFFER_SIZE];
//	char errbuf[PCAP_ERRBUF_SIZE];
};
#if 0
static pcap_t *
if_dpdk_configure_live_interface(const char *ifname, pcap_direction_t direction,
				 unsigned int isnonblock, int *fd, char *errbuf)
{
	pcap_t *new_p;
	//int dlt;
	
	new_p = pcap_create(ifname, errbuf);
	if (NULL == new_p)
		goto fail;

	if (-1 == pcap_setdirection(new_p, direction)) {
		printf("Could not restrict pcap capture to input on %s\n", ifname);
		goto fail;
	}

	pcap_set_timeout(new_p, 1);
	pcap_set_snaplen(new_p, 65535);
	pcap_set_promisc(new_p, 1);
	if (isnonblock)
		if (-1 == pcap_setnonblock(new_p, 1, errbuf)) {
			printf("Could not set non-blocking mode on %s\n", ifname);
			goto fail;
		}
	if (fd)
		if (-1 == (*fd = pcap_get_selectable_fd(new_p))) {
			printf("Could not get selectable fd for %s\n", ifname);
			goto fail;
		}
#if 0
	switch (pcap_activate(new_p)) {
	case 0:
		break;
	case PCAP_WARNING_PROMISC_NOTSUP:
		printf("Promiscuous mode not supported on %s: %s\n", ifname, pcap_geterr(new_p));
		break;
	case PCAP_WARNING:
		printf("Warning while activating pcap capture on %s: %s\n", ifname, pcap_geterr(new_p));
		break;
	case PCAP_ERROR_NO_SUCH_DEVICE:
	case PCAP_ERROR_PERM_DENIED:
		printf("Error activating pcap capture on %s: %s\n", ifname, pcap_geterr(new_p));
		/* FALLTHOUGH */
	default:
		goto fail;
		break;
	}

	dlt = pcap_datalink(new_p);
	if (DLT_EN10MB != dlt) {
		printf("Data link type on %s is %d, only %d supported\n", ifname, dlt, DLT_EN10MB);
		goto fail;
	}

#endif
	return (new_p);

fail:
	if (new_p)
		pcap_close(new_p);
	return (NULL);
}
//-----------------------------------------------------------------
#if 0
#include<fcntl.h>
#include<string.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>

#define MAX_SIZE 1600

typedef struct s_fifo_data{
	        int size;
			uint8_t data[MAX_SIZE];
}s_fifo_data;

static int in_fifo, out_fifo;
static char* in_fifo_path = "/tmp/in_fifo";
static char* out_fifo_path = "/tmp/out_fifo";
static char* sync_path = "/tmp/sync_file";
static int init_fifo(void)
{
#if 0
	if( access( in_fifo_path, F_OK ) != -1 )
		unlink(in_fifo_path);
	if( access( out_fifo_path, F_OK ) != -1 )
		unlink(out_fifo_path);
#endif

//	mkfifo(in_fifo_path, 0666);
    out_fifo = open(out_fifo_path, O_WRONLY);
//	mkfifo(out_fifo_path, 0666);
    in_fifo = open(in_fifo_path, O_RDONLY);
	FILE* f = fopen(sync_path, "a+");
	if(f != NULL)
	{
		fputs("dddd", f);	
		fflush(f);
	}
	else
	{
		perror("error......\n");
	}


	if(in_fifo < 0 || out_fifo < 0)
	{
		printf("failed to open in_fifo or out_fifo\n");
		exit(-1);
	}
	fclose(f);

	return 0;
}
#endif
#endif
//-----------------------------------------------------------------
struct if_dpdk_host_context *
if_dpdk_create_handle(const char *rx_ifname, unsigned int rx_isfile, int *rx_fd, unsigned int rx_isnonblock,
		      const char *tx_ifname, unsigned int tx_isfile, unsigned int tx_file_snaplen,
		      unsigned int tx_file_per_flow, uint8_t* mac_addr, unsigned int tx_file_dirbits,
		      uint32_t tx_file_epoch_no, uint32_t tx_file_instance_index)
{
	struct if_dpdk_host_context *ctx;
	int txisrx;

	if(dh_init_dpdk(rx_ifname, mac_addr))
	{
		printf("dpdk init failed....\n");
		goto fail;
	}

	ctx = calloc(1, sizeof(*ctx));
	if (NULL == ctx)
		goto fail;

	ctx->rx_isfile = rx_isfile;
	ctx->rx_ifname = rx_ifname;
	ctx->tx_isfile = tx_isfile;
	ctx->tx_file_per_flow = tx_file_per_flow;
	ctx->tx_snaplen = tx_file_snaplen;
	ctx->tx_ifname = tx_ifname;
	ctx->tx_dirbits = tx_file_dirbits;
	ctx->tx_epoch_no = tx_file_epoch_no;
	ctx->tx_instance_index = tx_file_instance_index;
#if 0
	txisrx = !tx_isfile && !rx_isfile && rx_ifname && tx_ifname && (strcmp(tx_ifname, rx_ifname) == 0);

	if (ctx->tx_ifname) {
		if (!ctx->tx_isfile) {
			ctx->tx_p = if_dpdk_configure_live_interface(tx_ifname,
								     txisrx ? PCAP_D_INOUT : PCAP_D_IN,
								     txisrx ? rx_isnonblock : 0,
								     (txisrx && rx_isnonblock) ? rx_fd : NULL,
								     ctx->errbuf);
			if (ctx->tx_p == NULL)
				goto fail;
		} else if (ctx->tx_file_per_flow) {
			//nothing, yangbiao
		}
	}

	if (txisrx)
		ctx->rx_p = ctx->tx_p;
	else
	{
		printf("yangbiao: error!\n");
		exit(-1);
	}
#endif
	return (ctx);

fail:
	if (ctx) {
#if 0		
		if (ctx->tx_p)
			pcap_close(ctx->tx_p);
		if (ctx->rx_p && (ctx->rx_p != ctx->tx_p))
			pcap_close(ctx->rx_p);
#endif		
		free(ctx);
	}

	return (NULL);
}

void
if_dpdk_destroy_handle(struct if_dpdk_host_context *ctx)
{
#if 0
	if (ctx->tx_ifname) {
		if (ctx->tx_isfile) {
			//NOTHING
		} else 
			pcap_close(ctx->tx_p);
	}
	if (ctx->rx_p && (ctx->rx_p != ctx->tx_p))
		pcap_close(ctx->rx_p);
#endif

	if(ctx)
		free(ctx);
}

int
if_dpdk_sendpacket(struct if_dpdk_host_context *ctx, const uint8_t *buf, unsigned int size,
		   uint64_t flowid, uint64_t ts_nsec, void* pkts, unsigned int num)
{
	return dh_send_pkts(pkts, num);
}

int
if_dpdk_getpacket(struct if_dpdk_host_context *ctx, uint64_t now,
		  uint32_t *buffer, uint16_t max_length, uint16_t *length, uint64_t *timestamp, uint64_t *wait_ns, dh_rte_mbuf_desc *info)
{
	*wait_ns = 0;
	return dh_recv_pkts((uint8_t*)buffer, length, info);
}




