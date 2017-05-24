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
#include <inttypes.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/timeb.h>
#include "dpdk_helper.h"

#define RX_RING_SIZE 4096 
#define TX_RING_SIZE 2048

#define NUM_MBUFS 16384 
#define MBUF_CACHE_SIZE 256
//#define BURST_SIZE 32

static const struct rte_eth_conf port_conf_default = {
    .rxmode = { .max_rx_pkt_len = ETHER_MAX_LEN, },
};

static unsigned nb_ports;

static struct {
    uint64_t total_cycles;
    uint64_t total_pkts;
} latency_numbers;

/* Private */
/*---------------------------------------------------------------------------*/
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


/*---------------------------------------------------------------------------*/
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

struct rte_mempool *mbuf_pool;
struct rte_mempool *mbuf_pool_tx;

static int port = 0;
/*---------------------------------------------------------------------------*/
static int 
dpdk_init(int argc, char *argv[], const char* ifname, uint8_t* mac_addr)
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
    mbuf_pool_tx = rte_pktmbuf_pool_create("MBUF_POOL_TX",
                                           NUM_MBUFS * 1, MBUF_CACHE_SIZE, 0,
                                           RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
    if (mbuf_pool == NULL || mbuf_pool_tx == NULL)
        rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

#if 1
    nb_ports = rte_eth_dev_count();
    if (nb_ports < 1)
        rte_exit(EXIT_FAILURE, "Error: number of ports must be even %d\n", nb_ports);


    /* initialize all ports */
    for (portid = 0; portid < nb_ports; portid++) {
        if (port_init(portid, mbuf_pool) != 0)
            rte_exit(EXIT_FAILURE, "Cannot init port %"PRIu8"\n",
                     portid);
    }
#endif

    //HARDCODE
    rte_eth_macaddr_get(0, mac_addr);
    if (rte_lcore_count() > 1)
        printf("\nWARNING: Too much enabled lcores - "
               "App uses only 1 lcore\n");

    return 0;
}

/* Public */
/*---------------------------------------------------------------------------*/
int dh_send_pkts(const uint8_t *buf, uint16_t num)
{
    if(buf != NULL && num > 0) {
#if 0
        int i = 0;
        struct rte_mbuf** tx_pkt=&buf[0];
        for(; i < num; i++, tx_pkt++) {
            printf("i%d, %lx %lx, \n", i, *tx_pkt, (*tx_pkt)->next);
        }
#endif
        return rte_eth_tx_burst(port, 0, &(buf[0]), num);
    }
    return 0;
}

/*---------------------------------------------------------------------------*/
int dh_recv_pkts(uint8_t *buf, uint16_t *len, dh_rte_mbuf_desc* desc)
{
    struct rte_mbuf* mbufs[MAX_BURST_SIZE];
    uint16_t i = 0;
    uint16_t nb = rte_eth_rx_burst(port, 0, mbufs, MAX_BURST_SIZE);

    for(; i < nb; i++) {
        struct rte_mbuf* mb = mbufs[i];

        desc[i].rm_base = (void*)mb;
        desc[i].rm_data = (void*)&((uint8_t*)mb->buf_addr+mb->data_off)[0];
        desc[i].data_len  = mb->data_len;
        desc[i].buf_len = mb->buf_len;
        desc[i].ref_cnt = &mb->refcnt;
    }

    return nb;
}

/*---------------------------------------------------------------------------*/
void dh_free_desc(void* ptr)
{
    rte_pktmbuf_free(ptr);
}

void* dh_alloc_desc(dh_rte_mbuf_desc* desc)
{
    struct rte_mbuf* mb = rte_pktmbuf_alloc(mbuf_pool);
    if(mb != NULL) {
        desc->rm_base = mb;
        desc->rm_data = (void*)((uint8_t*)mb->buf_addr+mb->data_off);
        desc->data_len  = mb->data_len;
        desc->buf_len = mb->buf_len;
        desc->ref_cnt = &mb->refcnt;
        desc->rm_data_len = &mb->data_len;
        desc->rm_pkt_len = &mb->pkt_len;
        desc->debug_next = &mb->next;
    } else {
        printf("dh_alloc_desc failed\n");
    }

    return (void*)mb;
}

/*---------------------------------------------------------------------------*/
int dh_init_dpdk(const char* ifname, uint8_t* mac_addr)
{
    char* argv[] = {"", "-c", "0x8", "-n", "4"};
    return dpdk_init(5, argv, ifname, mac_addr);
}
