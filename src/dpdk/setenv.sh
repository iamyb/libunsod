#!/bin/bash

# 1. Export Env
#------------------------------------------------------------------------------
export RTE_SDK=$(pwd)
export RTE_TARGET=x86_64-native-linuxapp-gcc
export LD_LIBRARY_PATH=${RTE_SDK}/${RTE_TARGET}/lib/

# 2. Create Huge Page
#------------------------------------------------------------------------------
sudo mkdir -p /mnt/huge 
sudo mount -t hugetlbfs nodev /mnt/huge
sudo sh -c "echo 256 > /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages"

# 3. Install rte_kni
#------------------------------------------------------------------------------
sudo insmod ${RTE_SDK}/${RTE_TARGET}/kmod/rte_kni.ko

# 4. ifdown eth1/eth2
#------------------------------------------------------------------------------
sudo ifconfig eth1 down
#sudo ifdown eth2

# 5. Bind eth1/2 to UIO
#------------------------------------------------------------------------------
sudo modprobe uio_pci_generic
sudo ${RTE_SDK}/tools/dpdk-devbind.py --bind=uio_pci_generic eth1
#${RTE_SDK}/tools/dpdk-devbind.py --bind=uio_pci_generic eth2

# 6. start KNI app
#------------------------------------------------------------------------------
sudo ./examples/kni/kni/${RTE_TARGET}/kni -c 0x0c -n 2 -- -P -p 0x1 --config="(0,2,3)"
#./examples/kni/kni/${RTE_TARGET}/kni -c 0x0c -n 2 -- -P -p 0x3 --config="(0,2,3),(1,2,3)"

# 7. IF CONFIG IP
#------------------------------------------------------------------------------
sudo ifconfig vEth0 192.168.1.100
#ifconfig vEth1 192.168.1.101
