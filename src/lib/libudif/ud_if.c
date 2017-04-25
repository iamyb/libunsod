#include<stdio.h>
#include<stdint.h>
#include "uinet_api.h"
#include "ud_ifconfig.h"

#define MAX_UDIF 16

static struct ud_if{
	struct ud_ifcfg cfg;
	uinet_if_t uif;
}ud_ifs[MAX_UDIF];

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
		if (0 != (error = uinet_interface_add_alias(uinet_instance_default(), param->name, param->addr, param->broadcast, param->mask))) {
			printf("Loopback alias add failed %d\n", error);
		}		
	}
	return error;
}

int ud_ifclose(const char* eth)
{
//
}