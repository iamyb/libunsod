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
#ifndef DPDK_HELPER_H
#define DPDK_HELPER_H

#define MAX_BURST_SIZE 512

typedef struct dh_rte_mbuf_desc {
    void*        rm_base;
    void*        rm_data;
    uint32_t     data_len;
    uint32_t     buf_len;
    void*        ref_cnt;
    uint16_t*    rm_data_len;
    uint32_t*    rm_pkt_len;
    uint64_t*    debug_next;
} dh_rte_mbuf_desc;

int   dh_init_dpdk (const char* ifname, uint8_t* mac_addr);
int   dh_send_pkts (const uint8_t *buf, uint16_t num);
int   dh_recv_pkts (uint8_t *buf, uint16_t *len, dh_rte_mbuf_desc* desc);
void  dh_free_desc (void* ptr);
void* dh_alloc_desc(dh_rte_mbuf_desc* desc);
#endif
