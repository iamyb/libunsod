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
#include<ytimestamp.h>
#include <sys/ctype.h>
#include <sys/dirent.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/kthread.h>
#include <sys/sched.h>
#include <sys/sockio.h>

#include <net/if.h>
#include <net/if_var.h>
#include <net/if_types.h>
#include <net/ethernet.h>
#include <net/if_arp.h>
#include <net/if_tap.h>
#include <net/if_dl.h>

#include <machine/atomic.h>

#include "uinet_internal.h"
#include "uinet_host_interface.h"
#include "uinet_if_dpdk.h"
#include "uinet_if_dpdk_host.h"


static void if_dpdk_default_config(union uinet_if_type_cfg *cfg);

static struct uinet_if_type_info if_dpdk_type_info = {
	.type = UINET_IFTYPE_DPDK,
	.type_name = "dpdk",
	.default_cfg = if_dpdk_default_config
};
UINET_IF_REGISTER_TYPE(DPDK, &if_dpdk_type_info);


struct if_dpdk_softc {
	struct ifnet *ifp;
	struct uinet_if *uif;
	uint8_t addr[ETHER_ADDR_LEN];
	int rx_disabled;
	int rx_isfile;
	int tx_disabled;
	int tx_isfile;
	int txisrx;
	int tx_use_thread;
	char *rx_ifname;
	char *tx_ifname;
	char host_ifname[IF_NAMESIZE];
	
	struct if_dpdk_host_context *dpdk_host_ctx;
	struct uinet_pd_list *tx_pds;
	struct uinet_pd_list *rx_pds;
	int rx_fd;
	unsigned int rx_thread_run_state;
	unsigned int rx_packet_waiting;
	uint32_t rx_batch_size;
	uint32_t rx_pd_count;
	
	struct thread *rx_thread;
	struct mtx rx_lock;
	struct cv rx_cv;
	struct thread *tx_thread;
	struct mtx tx_lock;
	struct cv tx_cv;
	int tx_pkts_to_send;
	struct uinet_pd_ring *tx_inject_ring;
	struct uinet_pd_ctx **tx_pdctx_to_free;
};


static int if_dpdk_setup_interface(struct if_dpdk_softc *sc);

static unsigned int interface_count;


static void
if_dpdk_default_config(union uinet_if_type_cfg *cfg)
{
	struct uinet_if_dpdk_cfg *pcfg;

	pcfg = &cfg->dpdk;

	pcfg->use_file_io_thread = 0;
	pcfg->file_snapshot_length = 65535;
	pcfg->file_per_flow = 0;
	pcfg->max_concurrent_files = 1000;
	pcfg->dir_bits = 10;
}


static void
if_dpdk_pd_alloc_user(struct uinet_if *uif, struct uinet_pd_list *pkts)
{
	uint32_t alloc_size;
	
	/* XXX add requested number of descs as an API arg instead of using the pd_list field ? */
	alloc_size = pkts->num_descs;
	pkts->num_descs = 0;
	uinet_pd_mbuf_alloc_descs(pkts, alloc_size);
}


static int
if_dpdk_extract_ifname(const char *start, const char *end, int *isfile, char **name)
{
	int offset = 0;
	
	if (strncmp(start, "file://", 7) == 0) {
		*isfile = 1;
		offset = 7;
	}

	if (start == end)
		return (1);

	*name = strndup(start + offset, end - (start + offset), M_DEVBUF);
	
	return (0);
}

/*
 * format is one of:
 *
 * <rxtx_intf>  (same interface used for rx and tx)
 * <rx_intf>,   (rx only, tx disabled)
 * ,<tx_intf>   (tx only, rx disabled)
 * <rx_intf>,<tx_intf>
 *
 * where <*_intf> is a host interface name or file://some_file_name, where
 * some_file_name does not contain a comma.
 */
static int
if_dpdk_process_configstr(struct if_dpdk_softc *sc)
{
	char *configstr = sc->uif->configstr;
	int error = 0;
	int empty;
	char *comma, *start, *end;

	comma = strchr(configstr, ',');
	start = configstr;
	end = configstr + strlen(configstr);

	empty = if_dpdk_extract_ifname(start, comma ? comma : end, &sc->rx_isfile, &sc->rx_ifname);
	if (empty) {
		if (comma)
			sc->rx_disabled = 1;
		else {
			error = EINVAL;
			goto out;
		}
	}

	if (comma) {
		empty = if_dpdk_extract_ifname(comma + 1, end, &sc->tx_isfile, &sc->tx_ifname);
		if (empty)
			sc->tx_disabled = 1;
	} else {
		sc->tx_isfile = sc->rx_isfile;
		sc->tx_ifname = strdup(sc->rx_ifname, M_DEVBUF);
	}

	if (!sc->rx_disabled && !sc->tx_disabled) {
		if (strcmp(sc->rx_ifname, sc->tx_ifname) == 0) {
			sc->txisrx = 1;

			/* can't tx and rx using same file */
			if (sc->rx_isfile && sc->tx_isfile) {
				free(sc->rx_ifname, M_DEVBUF);
				free(sc->tx_ifname, M_DEVBUF);
				error = EINVAL;
			}

			strlcpy(sc->host_ifname, sc->rx_ifname, sizeof(sc->host_ifname));
		} else 
			snprintf(sc->host_ifname, sizeof(sc->host_ifname), "%s,%s", sc->rx_ifname, sc->tx_ifname);
	} else if (!sc->rx_disabled)
		snprintf(sc->host_ifname, sizeof(sc->host_ifname), "%s", sc->rx_ifname);
	else
		snprintf(sc->host_ifname, sizeof(sc->host_ifname), "%s", sc->tx_ifname);
		
out:
	return (error);
}


int
if_dpdk_attach(struct uinet_if *uif)
{
	struct if_dpdk_softc *sc = NULL;
	struct uinet_if_dpdk_cfg *p_cfg;
	int error = 0;
	
	p_cfg = &uif->type_cfg.dpdk;

	if (NULL == uif->configstr) {
		error = EINVAL;
		goto fail;
	}

	printf("configstr is %s\n", uif->configstr);

	snprintf(uif->name, sizeof(uif->name), "dpdk%u", interface_count);
	interface_count++;

	sc = malloc(sizeof(struct if_dpdk_softc), M_DEVBUF, M_WAITOK|M_ZERO);
	if (NULL == sc) {
		printf("%s: if_dpdk_softc allocation failed\n", uif->name);
		error = ENOMEM;
		goto fail;
	}
	
	sc->uif = uif;
	sc->rx_batch_size = uif->rx_batch_size;
	
	error = if_dpdk_process_configstr(sc);
	if (0 != error) {
		goto fail;
	}
	
	sc->rx_pds = uinet_pd_list_alloc(sc->rx_batch_size);
	if (sc->rx_pds == NULL) {
		printf("%s: Failed to allocate rx pd list\n", uif->name);
		error = ENOMEM;
		goto fail;
	}
	sc->rx_pds->num_descs = 0;
	
	//fix: to increase the tx_pds size, yangbiao
	sc->tx_pds = uinet_pd_list_alloc(128);
	if (sc->tx_pds == NULL) {
		printf("%s: Failed to allocate tx pd list\n", uif->name);
		error = ENOMEM;
		goto fail;
	}
	sc->tx_pds->num_descs = 0;

	sc->dpdk_host_ctx = if_dpdk_create_handle(sc->rx_ifname, sc->rx_isfile, &sc->rx_fd,
						  sc->rx_isfile,
						  sc->tx_ifname, sc->tx_isfile,
						  p_cfg->file_snapshot_length, p_cfg->file_per_flow,
						  sc->addr, p_cfg->dir_bits,
						  epoch_number, uinet_instance_index(uif->uinst));
	if (NULL == sc->dpdk_host_ctx) {
		printf("%s: Failed to create dpdk handle\n", uif->name);
		error = ENXIO;
		goto fail;
	}

	if (!uinet_uifsts(sc->uif) || p_cfg->use_file_io_thread)
		sc->tx_use_thread = 1;
	
	if (!sc->tx_disabled && !sc->tx_isfile) {
#if 0
		if (0 != uhi_get_ifaddr(sc->tx_ifname, sc->addr)) {
			printf("%s: Failed to find interface address\n", uif->name);
			error = ENXIO;
			goto fail;
		}
#endif
	}

	sc->tx_inject_ring = uinet_pd_ring_alloc(uif->tx_inject_queue_len);
	if (sc->tx_inject_ring == NULL) {
		printf("%s: Failed to allocate transmit injection ring\n", uif->name);
		error = ENOMEM;
		goto fail;
	}

	sc->tx_pdctx_to_free = malloc(sizeof(*sc->tx_pdctx_to_free) * sc->tx_inject_ring->num_descs,
				      M_DEVBUF, M_WAITOK);
	if (sc->tx_pdctx_to_free == NULL) {
		printf("%s: Failed to allocate transmit pdctx retirement list\n", uif->name);
		error = ENOMEM;
		goto fail;
	}

	if (0 != if_dpdk_setup_interface(sc)) {
		error = ENXIO;
		goto fail;
	}

	return (0);

fail:
	if (sc) {
		if (sc->rx_ifname)
			free(sc->rx_ifname, M_DEVBUF);
		if (sc->tx_ifname)
			free(sc->tx_ifname, M_DEVBUF);
		if (sc->dpdk_host_ctx)
			if_dpdk_destroy_handle(sc->dpdk_host_ctx);
		if (sc->rx_pds)
			uinet_pd_list_free(sc->rx_pds);
		if (sc->tx_pds)
			uinet_pd_list_free(sc->tx_pds);
		if (sc->tx_inject_ring)
			uinet_pd_ring_free(sc->tx_inject_ring);
		if (sc->tx_pdctx_to_free)
			free(sc->tx_pdctx_to_free, M_DEVBUF);
		
		free(sc, M_DEVBUF);
	}

	return (error);
}


static void
if_dpdk_init(void *arg)
{
	struct if_dpdk_softc *sc = arg;
	struct ifnet *ifp = sc->ifp;

	ifp->if_drv_flags |= IFF_DRV_RUNNING;
	ifp->if_drv_flags &= ~IFF_DRV_OACTIVE;
}


static void
if_dpdk_sts_init(void *arg)
{
	struct if_dpdk_softc *sc = arg;
	struct ifnet *ifp = sc->ifp;

	ifp->if_drv_flags |= IFF_DRV_RUNNING;
	ifp->if_drv_flags |= IFF_DRV_OACTIVE; /* never call start */
}


static void
if_dpdk_inject_tx_pkts(struct uinet_if *uif, struct uinet_pd_list *pkts)
{
	struct if_dpdk_softc *sc = (struct if_dpdk_softc *)(uif->ifdata);
	struct uinet_pd_ring *txr;
	struct uinet_pd *pd;
	struct uinet_pd *first_drop_pd;
	uint32_t i;
	uint32_t space;
	uint32_t cur;
	uint32_t num_drops;
	uint32_t pds_to_check_for_drops;

	txr = sc->tx_inject_ring;
	//pd = &pkts->descs[1];
	pd = &pkts->descs[0];

	if (sc->tx_disabled) {
		first_drop_pd = pd;
		pds_to_check_for_drops = pkts->num_descs;
		goto drop;
	}

	if (sc->tx_use_thread)
		mtx_lock(&sc->tx_lock);

	space = uinet_pd_ring_space(txr);
	cur = txr->put;
	num_drops = 0;
	pds_to_check_for_drops = 0;
	for (i = 0; i < pkts->num_descs; i++, pd++) {
		if (pd->flags & UINET_PD_INJECT) {
			if (space) {
				txr->descs[cur] = *pd;
				//printf("in: put %d take %d cur %d pd->data %lx \n", txr->put, txr->take, cur, pd->data);
				cur = uinet_pd_ring_next(txr, cur);
				space--;
			} else {
				if (num_drops == 0) {
					first_drop_pd = pd;
					pds_to_check_for_drops = pkts->num_descs - i;
				}
				num_drops++;
			}
		}
	}
	txr->put = cur;
	txr->drops += num_drops;

	//fix, to avoid memory leak, yangbiao
	pkts->num_descs = 0;

	if (sc->tx_use_thread) {
		sc->tx_pkts_to_send++;
		if (sc->tx_pkts_to_send == 1)
		{
			cv_signal(&sc->tx_cv);
		}
		mtx_unlock(&sc->tx_lock);
	}

drop:
	if (pds_to_check_for_drops)
		uinet_pd_drop_injected(first_drop_pd, pds_to_check_for_drops); 
}


static int
if_dpdk_process_tx_inject_ring(struct if_dpdk_softc *sc, uint32_t *cur_inject_take)
{
	struct ifnet *ifp = sc->ifp;
	struct uinet_pd_ring *txr;
	struct uinet_pd * volatile cur_pd;
	struct uinet_pd_ctx **pdctx_to_free;
	uint32_t num_pd_ctx;
	uint32_t local_cur_inject_take, cur_inject_put;
	uint32_t inject_drops;
	uint32_t n_zero_copy;
	uint32_t n_copy;
	uint32_t total_bytes;
	void* pkts[MAX_BURST_SIZE];
	uint32_t pkts_num = 0;
	int do_single_flush;
	int done;
	int mb_len;
	
	txr = sc->tx_inject_ring;
	done = 0;

	if (sc->tx_use_thread) {
		mtx_lock(&sc->tx_lock);

		/* release inject ring descriptors we've processed */
		txr->take = *cur_inject_take;
		while ((sc->tx_pkts_to_send == 0) && !done)
			if (EWOULDBLOCK == cv_timedwait(&sc->tx_cv, &sc->tx_lock, curthread->td_stop_check_ticks))
				done = kthread_stop_check();
		sc->tx_pkts_to_send = 0;
	}

	local_cur_inject_take = txr->take;
	cur_inject_put = txr->put;
	inject_drops = txr->drops;
	txr->drops = 0;

	if (sc->tx_use_thread) {
		mtx_unlock(&sc->tx_lock);

		if (done)
			return (1);
	}
	
	n_copy = 0;
	n_zero_copy = 0;
	total_bytes = 0;

	pdctx_to_free = &sc->tx_pdctx_to_free[0];
	num_pd_ctx = 0;
	do_single_flush = 0;
	while (local_cur_inject_take != cur_inject_put) {
		cur_pd = &txr->descs[local_cur_inject_take];
		mb_len = cur_pd->ctx->m->m_hdr.mh_len;
		if (!(cur_pd->flags & UINET_PD_MGMT_ONLY)) {
			volatile void* data_ptr = (void*)cur_pd->serialno;
			pkts[pkts_num++] = data_ptr;
			//printf("out: put %d take %d local_cur_inject_take %d, cur_inject_put %d cur_pd->data%lx, ser_addr %lx, ser %lx\n", txr->put, txr->take, local_cur_inject_take, cur_inject_put, cur_pd->data, cur_pd->serialno, *(uint64_t*)(cur_pd->serialno));
			if(pkts_num == MAX_BURST_SIZE)
			{
				int n_snd = if_dpdk_sendpacket(sc->dpdk_host_ctx,
							(uint8_t *)cur_pd->data,
							    mb_len, cur_pd->serialno,
						cur_pd->ctx->timestamp, pkts, pkts_num);
				inject_drops += pkts_num - n_snd;
				n_copy += n_snd;
				pkts_num = 0;				
			}
			
			total_bytes += cur_pd->length;

			//PRINT_TIMESTAMP;			 
			
			*pdctx_to_free++ = cur_pd->ctx;
			num_pd_ctx++;
		}
#if 0
		if (cur_pd->flags & UINET_PD_FLUSH_FLOW) {
			if (sc->uif->type_cfg.dpdk.file_per_flow)
			{
				//if_dpdk_flushflow(sc->dpdk_host_ctx, cur_pd->serialno);
				printf("yangbiao: not used %s%d\n",__FUNCTION__, __LINE__);
			}
			else
				do_single_flush = 1;
		}
#endif
		local_cur_inject_take = uinet_pd_ring_next(txr, local_cur_inject_take);
	}

	if(pkts_num > 0)
	{
		int n_snd = if_dpdk_sendpacket(sc->dpdk_host_ctx,
					(uint8_t *)cur_pd->data,
						mb_len, cur_pd->serialno,
				cur_pd->ctx->timestamp, pkts, pkts_num);
		inject_drops += pkts_num - n_snd;
		n_copy += n_snd;
		pkts_num = 0;		
	}

	/*
	 * If all flows go to a single dump file, then we just do a single
	 * flush here if there was at least one flush request in the
	 * queue
	 */
#if 0
	if (do_single_flush)
	{
		printf("yangbiao: not used %s%d\n",__FUNCTION__, __LINE__);
		//if_dpdk_flushflow(sc->dpdk_host_ctx, 0);
	}
#endif
	ifp->if_oerrors += inject_drops;
	if (n_zero_copy + n_copy > 0) {
		ifp->if_opackets += n_zero_copy + n_copy;
		ifp->if_ozcopies += n_zero_copy;
		ifp->if_ocopies += n_copy;
		ifp->if_obytes += total_bytes;
	}

	uinet_pd_ref_release(sc->tx_pdctx_to_free, num_pd_ctx);

	if (!sc->tx_use_thread)
		txr->take = local_cur_inject_take;
	else
		*cur_inject_take = local_cur_inject_take;

	return (0);
}



static int
if_dpdk_transmit(struct ifnet *ifp, struct mbuf *m)
{
	struct if_dpdk_softc *sc = ifp->if_softc;
	struct uinet_pd *volatile pd;
	int error = 0;
	
	/*
	 * Everything goes to the injection queue, which will be serviced
	 * by:
	 *
	 * - the send thread in non-STS mode
	 * - the bulk-send routine in STS mode without remote I/O
	 * - the send thread in STS mode with remote I/O
	 *
	 */
	if (sc->tx_disabled) {
		error = ENETUNREACH;
		goto out;
	}
#if 1	
	volatile dh_rte_mbuf_desc dh_desc;
	void* rte_mb = dh_alloc_desc(&dh_desc);
	if(rte_mb == NULL )
	{
		error = ENOBUFS;
		goto out;
	}

	int len = m->m_pkthdr.len;
	if(len > dh_desc.buf_len)
	{
		error = ENOBUFS;
		//should free this rte mbuf
		goto out;
	}
	

   *dh_desc.rm_data_len = len;
   *dh_desc.rm_pkt_len = len;
   m_copydata(m, 0, len, (caddr_t)dh_desc.rm_data);

   void* pkts[MAX_BURST_SIZE];
   pkts[0] = rte_mb;
   if(dh_send_pkts(pkts, 1) == 0)
   {
   	    error = ENOBUFS;
		//should free this rte mbuf		
		goto out;
   }
   error = 0;
   volatile uint64_t debug_next = *(dh_desc.debug_next);

#endif
	
#if 0
	if (uinet_pd_mbuf_alloc_descs(sc->tx_pds, 1)) {
		dh_rte_mbuf_desc dh_desc;
		(void)memset(&dh_desc, 0, sizeof(dh_desc));
		void* mmmb = dh_alloc_desc(&dh_desc);
		if(dh_desc.rm_base != NULL)
		{
			//fix: to avoid invalid memory access, yangbiao
			pd = &sc->tx_pds->descs[0];
			if (m->m_pkthdr.len <= dh_desc.buf_len) {
				/* XXX set transmit timestamp */
				int len = m->m_pkthdr.len;
			#if 1
				int ll_len = 0;
				struct mbuf *tmp_m = m->m_nextpkt;
				while(tmp_m)
				{
					ll_len += tmp_m->m_len;
					tmp_m = tmp_m->m_nextpkt;
				}
#endif
				if(len > dh_desc.buf_len)
				{
					error = ENOBUFS;
					goto out;
				}
				
				pd->data = dh_desc.rm_base;
				pd->serialno = (uint64_t)dh_desc.rm_base;
				*dh_desc.rm_data_len = len;
				*dh_desc.rm_pkt_len = len;				
//				if(((uint64_t)pd->serialno - (uint64_t)dh_desc.rm_base) != (0x50))
	//				printf("1pd->data %lx, dh_desc.rm_base %lx, pd->serialno %lx, dh_desc.debug_next %lx dh_desc.rm_data %lx ll_len: %d len:%d\n",pd->data, dh_desc.rm_base, pd->serialno, dh_desc.debug_next, dh_desc.rm_data, ll_len ,len);
				
				m_copydata(m, 0, len, (caddr_t)dh_desc.rm_data);

				/* fix: to send this len to if, yangbiao*/
				pd->ctx->m->m_hdr.mh_len = len;
				pd->flags |= UINET_PD_INJECT;
				//if(((uint64_t)pd->serialno - (uint64_t)dh_desc.rm_base) != (0x50))
				//	printf("2pd->data %lx, dh_desc.rm_base %lx, pd->serialno %lx, dh_desc.debug_next %lx\n",pd->data, dh_desc.rm_base, pd->serialno, dh_desc.debug_next);
				if_dpdk_inject_tx_pkts(sc->uif, sc->tx_pds);
				volatile uint64_t debug_next = *(dh_desc.debug_next);

			}
			else
			{
				printf("ddddddddddddddddddddddddddddddddddddd\n");
			}
			error = 0;
		}
		else
		{
			error = ENOBUFS;
		}
	} 
	else
		error = ENOBUFS;
#endif
 out:
	m_freem(m);

	return (error);
}


static int
if_dpdk_batch_send(struct uinet_if *uif, int *fd, uint64_t *wait_ns)
{
	struct if_dpdk_softc *sc;

	sc = uif->ifdata;

	/*
	 * If using remote I/O, all packets are added to the TX inject ring
	 * by if_transmit() and will be processed in the send thread,
	 * otherwise the TX inject ring is handled here.
	 */
	if (!sc->tx_disabled && !sc->tx_use_thread)
		if_dpdk_process_tx_inject_ring(sc, NULL);

	*wait_ns = 0;
	*fd = -1;  /* call again at earliest convenience */
		
	return (0);
}


/*
 * This thread moves all data to the dpdk interface in non-STS mode, and in
 * STS mode when remote I/O is being used.
 */
#if 0
#undef _KERNEL
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/unistd.h>
#include<sys/fcntl.h>
int out_fifo_global;
#endif
static void
if_dpdk_send(void *arg)
{
	struct if_dpdk_softc *sc = (struct if_dpdk_softc *)arg;
	uint32_t cur_inject_take;
	
	if (sc->uif->tx_cpu >= 0)
		sched_bind(curthread, sc->uif->tx_cpu);
#if 0
	out_fifo_global = open("/tmp/out_fifo", 0666);
	if(out_fifo_global == -1) 
	{
		printf("fopen out fifo failed\n");
		return;
	}
#endif

	cur_inject_take = 0;
	while (1) {
		if (if_dpdk_process_tx_inject_ring(sc, &cur_inject_take))
			goto done;
	}

done:
	kthread_stop_ack();
}

//#define _KERNEL

static void
if_dpdk_stop(struct if_dpdk_softc *sc)
{
	struct ifnet *ifp = sc->ifp;

	ifp->if_drv_flags &= ~(IFF_DRV_RUNNING|IFF_DRV_OACTIVE);
}


static int
if_dpdk_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	int error = 0;
	struct if_dpdk_softc *sc = ifp->if_softc;

	switch (cmd) {
	case SIOCSIFFLAGS:
		if (ifp->if_flags & IFF_UP)
			if (uinet_uifsts(sc->uif))
				if_dpdk_sts_init(sc);
			else {
				if_dpdk_init(sc);

				if (!sc->rx_disabled) {
					mtx_lock(&sc->rx_lock);
					if (sc->rx_thread_run_state == 0) {
						sc->rx_thread_run_state = 1;
						cv_signal(&sc->rx_cv);
					}
					mtx_unlock(&sc->rx_lock);
				}
			}
		else if (ifp->if_drv_flags & IFF_DRV_RUNNING) {
			if (!sc->rx_disabled && !uinet_uifsts(sc->uif)) {
				mtx_lock(&sc->rx_lock);
				if (sc->rx_thread_run_state == 1) {
					sc->rx_thread_run_state = 2;
					cv_signal(&sc->rx_cv);
					while (sc->rx_thread_run_state != 3)
						cv_wait(&sc->rx_cv, &sc->rx_lock);
					sc->rx_thread_run_state = 0;
				}
				mtx_unlock(&sc->rx_lock);
			}

			if_dpdk_stop(sc);
		}
		break;
	default:
		error = ether_ioctl(ifp, cmd, data);
		break;
	}

	return (error);
}

extern void uinet_pd_mbuf_free_ctx(struct uinet_pd_ctx *pdctx);
static void if_dpdk_free_rte_buf(void* base, void* dummy)
{
	struct uinet_pd_ctx *pdctx[10];
	dh_free_desc(base);

	struct uinet_pd_ctx *ctx = (struct uinet_pd_ctx *)dummy;
	pdctx[0] = ctx;
	uinet_pd_mbuf_free_descs(pdctx, 1);
}

static int
if_dpdk_batch_receive(struct uinet_if *uif, int *fd, uint64_t *wait_ns)
{
	struct if_dpdk_softc *sc;
	struct uinet_pd *rx_pd;
	uint64_t now;
	uint64_t timestamp;
	unsigned int had_packet_waiting;
	unsigned int max_rx;
	unsigned int i;
	int rv;
	uint32_t used, to_move;
	dh_rte_mbuf_desc descs[MAX_BURST_SIZE];	

	sc = uif->ifdata;

	if (sc->rx_disabled) {
		/* don't call more often than every second, since there's nothing to do */
		*fd = -1;
		*wait_ns = 1000000000;
		return (0);
	}
	*wait_ns = 0;

	/* Top off the packet descriptor list */
	uinet_pd_mbuf_alloc_descs(sc->rx_pds, sc->rx_batch_size - sc->rx_pds->num_descs);
	now = uhi_clock_gettime_ns(UHI_CLOCK_MONOTONIC);

	had_packet_waiting = sc->rx_packet_waiting;
	sc->rx_packet_waiting = 0;
	rx_pd = &sc->rx_pds->descs[had_packet_waiting];
	max_rx = sc->rx_pds->num_descs - had_packet_waiting;

	rv = if_dpdk_getpacket(sc->dpdk_host_ctx, now, rx_pd->data, MCLBYTES,
						   &rx_pd->length, &timestamp, wait_ns, &descs);
	if(rv <= 0) 
		return (i == max_rx);

	for (i = 0; i < max_rx && i < rv; i++, rx_pd++) {
		if (uif->timestamp_mode == UINET_IF_TIMESTAMP_HW)
			rx_pd->ctx->timestamp = timestamp;

		PRINT_TIMESTAMP;
		rx_pd->ctx->m->m_ext.ref_cnt = descs[i].ref_cnt;
		m_extadd(rx_pd->ctx->m, descs[i].rm_data, descs[i].buf_len, 
			if_dpdk_free_rte_buf, descs[i].rm_base, rx_pd->ctx, 0,EXT_EXTREF);
		rx_pd->length = descs[i].data_len;
		rx_pd->flags |= UINET_PD_TO_STACK;
		if (*wait_ns > 0) {
			sc->rx_packet_waiting = 1;
			break;
		}
	}
	used = i + had_packet_waiting;
	to_move = sc->rx_pds->num_descs - used;

	if (used) {
		UIF_TIMESTAMP(uif, sc->rx_pds);
	
		UIF_BATCH_EVENT(uif, UINET_BATCH_EVENT_START);

		UIF_FIRST_LOOK(uif, sc->rx_pds);

		uinet_pd_deliver_to_stack(uif, sc->rx_pds);

		UIF_BATCH_EVENT(uif, UINET_BATCH_EVENT_FINISH);

		if (to_move)
			memmove(sc->rx_pds->descs, &sc->rx_pds->descs[used],
				sizeof(sc->rx_pds->descs[0]) * to_move);

		sc->rx_pds->num_descs -= used;
	}
	
	if (sc->rx_packet_waiting || (i == max_rx))
		*fd = -1; /* wait_ns is non-zero or we were batch limited */
	else
		*fd = sc->rx_fd; /* need to wait for a new packet to arrive */

	return (i == max_rx);
}


static void
if_dpdk_receive(void *arg)
{
	struct if_dpdk_softc *sc = (struct if_dpdk_softc *)arg;
	uint64_t wait_ns;
	int unused;
	int wait_for_start;
	int done;
	
	if (sc->uif->rx_cpu >= 0)
		sched_bind(curthread, sc->uif->rx_cpu);

	wait_for_start = 1;
	done = 0;
	while (!kthread_stop_check()) {
		mtx_lock(&sc->rx_lock);
		if (sc->rx_thread_run_state == 2) {
			sc->rx_thread_run_state = 3;
			cv_signal(&sc->rx_cv);
			wait_for_start = 1;
		}
		if (wait_for_start) {
			while ((sc->rx_thread_run_state != 1) && !done)
				if (EWOULDBLOCK == cv_timedwait(&sc->rx_cv, &sc->rx_lock,
								curthread->td_stop_check_ticks))
					done = kthread_stop_check();
		}
		mtx_unlock(&sc->rx_lock);

		if (done)
			break;
		
		if_dpdk_batch_receive(sc->uif, &unused, &wait_ns);
		if (wait_ns)
			uhi_nanosleep(wait_ns);
	}

	kthread_stop_ack();
}


static int
if_dpdk_setup_interface(struct if_dpdk_softc *sc)
{
	struct ifnet *ifp;
	struct uinet_if *uif;

	ifp = sc->ifp = if_alloc(IFT_ETHER);
	uif = sc->uif;
	
	if (uinet_uifsts(uif))
		ifp->if_init = if_dpdk_sts_init;
	else
		ifp->if_init = if_dpdk_init;
	ifp->if_softc = sc;

	if_initname(ifp, sc->uif->name, IF_DUNIT_NONE);
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = if_dpdk_ioctl;
	ifp->if_transmit = if_dpdk_transmit;

	/* XXX what values? */
	IFQ_SET_MAXLEN(&ifp->if_snd, 1024);
	ifp->if_snd.ifq_drv_maxlen = 1024;

	IFQ_SET_READY(&ifp->if_snd);

	ether_ifattach(ifp, sc->addr);
	ifp->if_capabilities = ifp->if_capenable = IFCAP_HWSTATS;

	uif->pd_alloc = if_dpdk_pd_alloc_user;
	uif->inject_tx_pkts = if_dpdk_inject_tx_pkts;
	uif->batch_rx = if_dpdk_batch_receive;
	uif->batch_tx = if_dpdk_batch_send;
	uinet_if_attach(uif, sc->ifp, sc);

	
	if (sc->tx_use_thread) {
		mtx_init(&sc->tx_lock, "txlk", NULL, MTX_DEF);
		cv_init(&sc->tx_cv, "txcv");

		if (kthread_add(if_dpdk_send, sc, NULL, &sc->tx_thread, 0, 0, "dpdk_tx: %s", ifp->if_xname)) {
			printf("Could not start transmit thread for %s (%s)\n", ifp->if_xname, sc->host_ifname);
			ether_ifdetach(ifp);
			if_free(ifp);
			return (1);
		}
	}

	if (!uinet_uifsts(uif)) {
		mtx_init(&sc->rx_lock, "rxlk", NULL, MTX_DEF);
		cv_init(&sc->rx_cv, "rxcv");
#if 1		
		if (kthread_add(if_dpdk_receive, sc, NULL, &sc->rx_thread, 0, 0, "dpdk_rx: %s", ifp->if_xname)) {
			printf("Could not start receive thread for %s (%s)\n", ifp->if_xname, sc->host_ifname);
			ether_ifdetach(ifp);
			if_free(ifp);
			return (1);
		}
#endif
	}

	return (0);
}


int
if_dpdk_detach(struct uinet_if *uif)
{
	struct if_dpdk_softc *sc = uif->ifdata;
	struct thread_stop_req rx_tsr;
	struct thread_stop_req tx_tsr;

	if (sc) {
		if (!uinet_uifsts(uif)) {
			printf("%s (%s): Stopping rx thread\n", uif->name, uif->alias[0] != '\0' ? uif->alias : "");
			kthread_stop(sc->rx_thread, &rx_tsr);
		}

		if (sc->tx_use_thread) {
			printf("%s (%s): Stopping tx thread\n", uif->name, uif->alias[0] != '\0' ? uif->alias : "");
			kthread_stop(sc->tx_thread, &tx_tsr);
		}
		
		if (!uinet_uifsts(uif)) {
			kthread_stop_wait(&rx_tsr);
			mtx_destroy(&sc->rx_lock);
			cv_destroy(&sc->rx_cv);
		}
		if (sc->tx_use_thread) {
			kthread_stop_wait(&tx_tsr);
			mtx_destroy(&sc->tx_lock);
			cv_destroy(&sc->tx_cv);
		}
			
		printf("%s (%s): Interface stopped\n", uif->name, uif->alias[0] != '\0' ? uif->alias : "");

		if (sc->rx_ifname)
			free(sc->rx_ifname, M_DEVBUF);
		if (sc->tx_ifname)
			free(sc->tx_ifname, M_DEVBUF);
		if_dpdk_destroy_handle(sc->dpdk_host_ctx);
		uinet_pd_list_free(sc->rx_pds);
		uinet_pd_list_free(sc->tx_pds);
		uinet_pd_ring_free(sc->tx_inject_ring);
		free(sc->tx_pdctx_to_free, M_DEVBUF);
		
		free(sc, M_DEVBUF);
	}

	return (0);
}


