#ifndef DPDK_HELPER_H
#define DPDK_HELPER_H

#define MAX_BURST_SIZE 512

typedef struct dh_rte_mbuf_desc
{
	void*        rm_base;
	void*        rm_data;
	uint32_t     data_len;
	uint32_t     buf_len;	
	void*        ref_cnt;
	uint16_t*    rm_data_len;
	uint32_t*    rm_pkt_len;
	uint64_t*    debug_next;
}dh_rte_mbuf_desc;

int   dh_init_dpdk (const char* ifname);
int   dh_send_pkts (const uint8_t *buf, uint16_t num);
int   dh_recv_pkts (uint8_t *buf, uint16_t *len, dh_rte_mbuf_desc* desc);
void  dh_free_desc (void* ptr);
void* dh_alloc_desc(dh_rte_mbuf_desc* desc);
#endif
