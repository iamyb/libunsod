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
export RTE_SDK=${CURDIR}/src/dpdk
export RTE_TARGET=x86_64-native-linuxapp-gcc
export LD_LIBRARY_PATH=${RTE_SDK}/${RTE_TARGET}/lib/ 

all: uinet dpdk unsod app

.PHONY:app

app: 
	make -C ./app

	
unsod: 
	make -C ./lib

uinet:	
	make -C ./src
	make -C ./lib

#make -C ./src/dpdk
dpdk:
	make -C ./src/dpdk config T=x86_64-native-linuxapp-gcc
	make -C ./src/dpdk install T=x86_64-native-linuxapp-gcc	
	make -C ./src
	make -C ./lib
	
clean_app:
	make -C ./app clean

clean_unsod: 
	make -C ./lib clean

clean_uinet:
	make -C ./src clean

clean_dpdk:
	make -C ./src/dpdk clean
	
clean: clean_app clean_unsod clean_uinet clean_dpdk
