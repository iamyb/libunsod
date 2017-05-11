################################################################################
# Copyright (C) 2017 b20yang 
# ---
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation; either version 3 of the License, or (at your option) any
# later version.

# This program is distributed in the hope that it will be useful,but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

# You should have received a copy of the GNU Lesser General Public License along
# with this program. If not, see <http://www.gnu.org/licenses/>.
################################################################################
ROOT_DIR=..
DPDK_LIBS_PATH=$(ROOT_DIR)/src/dpdk/$(RTE_TARGET)/lib
LIBUNSOD=libunsod.a

DPDK_LIBS=$(DPDK_LIBS_PATH)/librte_acl.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/libethdev.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_cfgfile.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_cmdline.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_cryptodev.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_distributor.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_eal.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_hash.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_ip_frag.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_jobstats.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_kni.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_kvargs.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_lpm.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_mbuf.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_mempool.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_pdump.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_pipeline.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_pmd_af_packet.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_pmd_bnxt.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_pmd_bond.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_pmd_cxgbe.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_pmd_e1000.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_pmd_ena.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_pmd_enic.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_pmd_fm10k.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_pmd_i40e.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_pmd_ixgbe.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_pmd_null.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_pmd_null_crypto.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_pmd_ring.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_pmd_vhost.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_pmd_virtio.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_pmd_vmxnet3_uio.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_power.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_reorder.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_ring.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_table.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_timer.a
DPDK_LIBS+=$(DPDK_LIBS_PATH)/librte_vhost.a

UINET_LIBS=$(ROOT_DIR)/src/lib/libdpdk_helper/libdpdk_helper.a
UINET_LIBS+=$(ROOT_DIR)/src/lib/liblatprof/liblatprof.a
UINET_LIBS+=$(ROOT_DIR)/src/lib/libuinet/libuinet.a
UINET_LIBS+=$(ROOT_DIR)/src/lib/libudif/libudif.a
UINET_LIBS+=$(ROOT_DIR)/src/lib/libudsock/libudsock.a

all: $(LIBUNSOD)

$(LIBUNSOD):$(DPDK_LIBS) $(UINET_LIBS)
	@echo "create libunsod.a" > libunsod.mri
	@$(foreach dir,$(DPDK_LIBS),echo "addlib $(dir)" >> libunsod.mri;)
	@$(foreach dir,$(UINET_LIBS),echo "addlib $(dir)" >> libunsod.mri;)	
	@echo "save" >> libunsod.mri
	@echo "end" >> libunsod.mri
	ar -M <libunsod.mri

DPDK_INSTALL:
	make -C ../src/dpdk install T=x86_64-native-linuxapp-gcc

clean:
	rm libunsod.mri
	rm libunsod.a
