/**
********************************************************************************
Copyright (C) 2016 b20yang
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
#include<stdio.h>
#include<stdint.h>
#include<pthread.h>
#include "uinet_api.h"
#include "ud_ifconfig.h"
#include "uinet_host_netstat_api.h"

#define MAX_UDIF 16

struct ud_if {
    struct ud_ifcfg cfg;
    uinet_if_t uif;
} ud_ifs[MAX_UDIF];

unsigned int udif_count = 0;

uinet_if_t udif_getuif(char *ethname)
{
    return ud_ifs[0].uif;
}

int ud_ifsetup(struct ud_ifcfg* param)
{
    int error = 0;
    uinet_if_t ud_uif;

    struct uinet_global_cfg cfg;
    uinet_default_cfg(&cfg, UINET_GLOBAL_CFG_MEDIUM);
    uinet_init(&cfg, NULL);

    uinet_install_sighandlers();

    struct uinet_if_cfg ifcfg;
    uinet_if_default_config(UINET_IFTYPE_DPDK, &ifcfg);

    ifcfg.configstr = param->name;
    ifcfg.alias = param->name;

    error = uinet_ifcreate(uinet_instance_default(), &ifcfg, &ud_uif);
    if (0 != error) {
        printf("Failed to create interface (%d)\n", error);
    } else {
        error = uinet_interface_up(uinet_instance_default(), param->name, 1, 0);
        if (0 != error) {
            printf("Failed to bring up interface (%d)\n", error);
        }
        if (0 != (error = uinet_interface_add_alias(uinet_instance_default(),
                          param->name, param->addr, param->broadcast, param->mask))) {
            printf("Loopback alias add failed %d\n", error);
        }
        ud_ifs[udif_count].cfg = *param;
        ud_ifs[udif_count].uif = ud_uif;
    }
#if 1
    int tid;
    if(pthread_create(&tid, NULL, uinet_host_netstat_listener_thread, NULL)==-1)
        printf("pthread_create error!\n");
#endif
    return error;
}

int ud_ifclose(const char* eth)
{
//
}