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

# 3. ifdown eth1/eth2
#------------------------------------------------------------------------------
sudo ifconfig eth1 down

# 4. Bind eth1/2 to UIO
#------------------------------------------------------------------------------
sudo modprobe uio_pci_generic
sudo chmod 777 ${RTE_SDK}/tools/dpdk-devbind.py
sudo ${RTE_SDK}/tools/dpdk-devbind.py --bind=uio_pci_generic eth1


