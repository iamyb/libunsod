/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2010-2015 Intel Corporation. All rights reserved.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <inttypes.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include<pthread.h>
#include <arpa/inet.h>
#include <sys/timeb.h>

#define RX_RING_SIZE 128
#define TX_RING_SIZE 512

#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32

static const struct rte_eth_conf port_conf_default = {
	.rxmode = { .max_rx_pkt_len = ETHER_MAX_LEN, },
};

static unsigned nb_ports;

static struct {
	uint64_t total_cycles;
	uint64_t total_pkts;
} latency_numbers;


static uint16_t
add_timestamps(uint8_t port __rte_unused, uint16_t qidx __rte_unused,
		struct rte_mbuf **pkts, uint16_t nb_pkts,
		uint16_t max_pkts __rte_unused, void *_ __rte_unused)
{
	unsigned i;
	uint64_t now = rte_rdtsc();

	for (i = 0; i < nb_pkts; i++)
		pkts[i]->udata64 = now;
	return nb_pkts;
}

static uint16_t
calc_latency(uint8_t port __rte_unused, uint16_t qidx __rte_unused,
		struct rte_mbuf **pkts, uint16_t nb_pkts, void *_ __rte_unused)
{
	uint64_t cycles = 0;
	uint64_t now = rte_rdtsc();
	unsigned i;

	for (i = 0; i < nb_pkts; i++)
		cycles += now - pkts[i]->udata64;
	latency_numbers.total_cycles += cycles;
	latency_numbers.total_pkts += nb_pkts;

	if (latency_numbers.total_pkts > (100 * 1000 * 1000ULL)) {
		printf("Latency = %"PRIu64" cycles\n",
		latency_numbers.total_cycles / latency_numbers.total_pkts);
		latency_numbers.total_cycles = latency_numbers.total_pkts = 0;
	}
	return nb_pkts;
}

/*
 * Initialises a given port using global settings and with the rx buffers
 * coming from the mbuf_pool passed as parameter
 */
static inline int
port_init(uint8_t port, struct rte_mempool *mbuf_pool)
{
	struct rte_eth_conf port_conf = port_conf_default;
	const uint16_t rx_rings = 1, tx_rings = 1;
	int retval;
	uint16_t q;

	if (port >= rte_eth_dev_count())
		return -1;

	retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
	if (retval != 0)
		return retval;

	for (q = 0; q < rx_rings; q++) {
		retval = rte_eth_rx_queue_setup(port, q, RX_RING_SIZE,
				rte_eth_dev_socket_id(port), NULL, mbuf_pool);
		if (retval < 0)
			return retval;
	}

	for (q = 0; q < tx_rings; q++) {
		retval = rte_eth_tx_queue_setup(port, q, TX_RING_SIZE,
				rte_eth_dev_socket_id(port), NULL);
		if (retval < 0)
			return retval;
	}

	retval  = rte_eth_dev_start(port);
	if (retval < 0)
		return retval;

	struct ether_addr addr;

	rte_eth_macaddr_get(port, &addr);
	printf("Port %u MAC: %02"PRIx8" %02"PRIx8" %02"PRIx8
			" %02"PRIx8" %02"PRIx8" %02"PRIx8"\n",
			(unsigned)port,
			addr.addr_bytes[0], addr.addr_bytes[1],
			addr.addr_bytes[2], addr.addr_bytes[3],
			addr.addr_bytes[4], addr.addr_bytes[5]);

	rte_eth_promiscuous_enable(port);
	rte_eth_add_rx_callback(port, 0, add_timestamps, NULL);
	rte_eth_add_tx_callback(port, 0, calc_latency, NULL);

	return 0;
}

//-----------------------------------------------------------------
#include<fcntl.h>
#include<string.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>

#define MAX_SIZE 1600

typedef struct s_fifo_data{
	int size;
	char data[MAX_SIZE];
}s_fifo_data;

static int in_fifo, out_fifo;
static char* in_fifo_path = "/tmp/in_fifo";
static char* out_fifo_path = "/tmp/out_fifo";
static char* sync_path = "/tmp/sync_file";
int init_fifo(void)
{
	if( access( in_fifo_path, F_OK ) != -1 )
	{
		int ret = unlink(in_fifo_path);
		if(ret != 0) perror("^^^^^^^^^^^^^^^^^^^");
	}
	if( access( out_fifo_path, F_OK ) != -1 )
	{
		int ret = unlink(out_fifo_path);
		if(ret != 0) perror("1^^^^^^^^^^^^^^^^^^^");
	}
	if( access( sync_path, F_OK ) != -1 )
	{
		int ret = unlink(sync_path);
		if(ret != 0) perror("2^^^^^^^^^^^^^^^^^^^");
	}

	mkfifo(in_fifo_path, 0666);
	mkfifo(out_fifo_path, 0666);

	out_fifo = open(out_fifo_path, O_RDONLY);
	if(out_fifo < 0)
	{
		printf("open out_fifo error!!!!!!!!!!!!!!!\n");
	}
	printf("open in fifo path\n");
	in_fifo = open(in_fifo_path, O_WRONLY);
	if(in_fifo < 0)
	{
		printf("open out_fifo error!!!!!!!!!!!!!!!\n");
	}

	while( access( sync_path, F_OK) == -1)
	{
		sleep(1);
		printf("waiting...\n");
	}
}

int free_fifo(void)
{
	close(in_fifo);
	close(out_fifo);
	unlink(in_fifo_path);
	unlink(out_fifo_path);
}

struct rte_mempool *mbuf_pool;
/*
 * Main thread that does the work, reading from INPUT_PORT
 * and writing to OUTPUT_PORT
 */


void send_to_eth(void)
{
			uint16_t nb_tx = 0;
			while(1)
			{
				s_fifo_data out_buf;

				int count = read(out_fifo, &out_buf, sizeof(out_buf));
//				printf("==============================================blockd\n");
				if(count == sizeof(out_buf))
				{
					struct timeb tp;
					ftime(&tp);
			    	struct rte_mbuf* out_pkts = rte_pktmbuf_alloc(mbuf_pool);
					out_pkts->data_len = out_buf.size;
					out_pkts->pkt_len = out_buf.size;

					memcpy((uint8_t*)out_pkts->buf_addr+out_pkts->data_off, out_buf.data, out_buf.size);

					#if 1
					const uint16_t nb_tx = rte_eth_tx_burst(0, 0, &out_pkts, 1);
					#endif

#if 0
//					printf("target: send packet.......%lx\n", tp.time); 

					int i = 0;
					printf("-----send-----\n");
					for(; i < out_buf.size; i++)
					{
						printf("%x ", (unsigned char)out_buf.data[i]);
					}
					printf("--------------\n");
#endif
				}
			}
}



static  __attribute__((noreturn)) void
lcore_main(void)
{
	uint8_t port;
	char name[100];
	rte_eth_dev_get_name_by_port(0, name);
	printf("=====================port by name %s \n", name);
	//init_fifo();

	for (port = 0; port < nb_ports; port++)
		if (rte_eth_dev_socket_id(port) > 0 &&
				rte_eth_dev_socket_id(port) !=
						(int)rte_socket_id())
			printf("WARNING, port %u is on remote NUMA node to "
					"polling thread.\n\tPerformance will "
					"not be optimal.\n", port);

	printf("\nCore %u forwarding packets. [Ctrl+C to quit]\n",
			rte_lcore_id());

	for (;;) {
		for (port = 0; port < nb_ports; port++) {
			struct rte_mbuf *bufs[BURST_SIZE];
			const uint16_t nb_rx = rte_eth_rx_burst(port, 0,
					bufs, BURST_SIZE);
			if (unlikely(nb_rx == 0))
				continue;
			uint16_t nb_tx = 0;

//				printf("----------%d, \n", nb_rx);
			if(nb_rx > 0)
			{
				s_fifo_data fbuf;
				int ii = 0;
				
				for(;ii < nb_rx; ii++)
				{
                	struct rte_mbuf *mb = bufs[ii];
					fbuf.size = mb->data_len;
					memcpy(fbuf.data,&((uint8_t*)mb->buf_addr+mb->data_off)[0],mb->data_len);
					//if(((uint8_t*)mb->buf_addr+mb->data_off)[33]==62)
					{
						write(in_fifo, &fbuf, sizeof(s_fifo_data));
					}
#if 1
#if 0
                	printf("%s:dst mac: %lx:%lx:%lx:%lx:%lx:%lx\n", __FUNCTION__, ((uint8_t*)mb->buf_addr+mb->data_off)[0],
                	((uint8_t*)mb->buf_addr+mb->data_off)[1]
                	,((uint8_t*)mb->buf_addr+mb->data_off)[2]
                	,((uint8_t*)mb->buf_addr+mb->data_off)[3]
                	,((uint8_t*)mb->buf_addr+mb->data_off)[4]
                	,((uint8_t*)mb->buf_addr+mb->data_off)[5]);
                	printf("%s:src mac: %lx:%lx:%lx:%lx:%lx:%lx\n", __FUNCTION__, ((uint8_t*)mb->buf_addr+mb->data_off)[6],
                	((uint8_t*)mb->buf_addr+mb->data_off)[7]
                	,((uint8_t*)mb->buf_addr+mb->data_off)[8]
                	,((uint8_t*)mb->buf_addr+mb->data_off)[9]
                	,((uint8_t*)mb->buf_addr+mb->data_off)[10]
                	,((uint8_t*)mb->buf_addr+mb->data_off)[11]);
					printf("%x %x\n", 
                	((uint8_t*)mb->buf_addr+mb->data_off)[12],
                	((uint8_t*)mb->buf_addr+mb->data_off)[13]);
#endif

					if	(((uint8_t*)mb->buf_addr+mb->data_off)[12]==8&&
	((uint8_t*)mb->buf_addr+mb->data_off)[13]==0)
					{
						//14,15,16,17//18,19.20,21//22,23,24,25
					
					if(
                	((uint8_t*)mb->buf_addr+mb->data_off)[33]==103)
					{
						struct in_addr ip_addr1, ip_addr2;
						uint16_t src_port = 0;
						uint16_t dst_port = 0;
                		int ihl = ((uint8_t*)mb->buf_addr+mb->data_off)[14] & 0xF;
					
						if(ihl <= 5)
						{
						
              		 		src_port = *(uint16_t*)((uint8_t*)mb->buf_addr+mb->data_off+34);
//					 		printf("src_port, %d\n",ntohs(src_port));
              		 		dst_port = *(uint16_t*)((uint8_t*)mb->buf_addr+mb->data_off+35);
//					 		printf("src_port, %d\n",ntohs(dst_port));
						}
						
#if 0
              		    uint32_t src_ip = *(uint32_t*)((uint8_t*)mb->buf_addr+mb->data_off+26);
						uint32_t dst_ip = *(uint32_t*)((uint8_t*)mb->buf_addr+mb->data_off+30);

						ip_addr1.s_addr = src_ip;
						ip_addr2.s_addr = dst_ip;

						char src_str[100];
						char dst_str[100];
						strcpy(src_str, inet_ntoa(ip_addr1));
						strcpy(dst_str, inet_ntoa(ip_addr2));

						struct timeb tp;
						ftime(&tp);
                		printf("%lx, %s:src ip: %s:%d, dst ip: %s:%d\n", tp.time, __FUNCTION__, src_str,src_port,dst_str,dst_port);
					int i = 0;
					printf("-----receive-----\n");
					for(; i < fbuf.size; i++)
					{
						printf("%x ", (unsigned char)fbuf.data[i]);
					}
					printf("--------------\n");
#endif
					}
					
					}
					
#endif
				}
				uint16_t buf;
				for (buf = nb_tx; buf < nb_rx; buf++)
					rte_pktmbuf_free(bufs[buf]);
			}
#if 0
			while(1)
			{
				s_fifo_data out_buf;
				printf("read....\n");

				int count = read(out_fifo, &out_buf, sizeof(out_buf));
				if(count == sizeof(out_buf))
				{
					printf("target: send packet.......\n");
			    	struct rte_mbuf* out_pkts = rte_pktmbuf_alloc(mbuf_pool);
					out_pkts->data_len = out_buf.size;
					out_pkts->pkt_len = out_buf.size;

					memcpy((uint8_t*)out_pkts->buf_addr+out_pkts->data_off, out_buf.data, out_buf.size);

					#if 1
					const uint16_t nb_tx = rte_eth_tx_burst(port, 0, out_pkts, 1);
					#endif
				}
				else
				{
					break;
				}
			}
			printf("one loop\n");
#endif
		}
	}
	free_fifo();
}

/* Main function, does initialisation and calls the per-lcore functions */
int
main(int argc, char *argv[])
{
	uint8_t portid;

	/* init EAL */
	int ret = rte_eal_init(argc, argv);

	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");
	argc -= ret;
	argv += ret;

	mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL",
		NUM_MBUFS * 1, MBUF_CACHE_SIZE, 0,
		RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
	if (mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

	nb_ports = rte_eth_dev_count();
	if (nb_ports < 1)
		rte_exit(EXIT_FAILURE, "Error: number of ports must be even %d\n", nb_ports);


	/* initialize all ports */
	for (portid = 0; portid < nb_ports; portid++)
		if (port_init(portid, mbuf_pool) != 0)
			rte_exit(EXIT_FAILURE, "Cannot init port %"PRIu8"\n",
					portid);

	if (rte_lcore_count() > 1)
		printf("\nWARNING: Too much enabled lcores - "
			"App uses only 1 lcore\n");

	init_fifo();
	pthread_t tid;
	pthread_create(&tid, NULL, send_to_eth, NULL);
	/* call lcore_main on master core only */
	lcore_main();
	return 0;
}
