#ifndef _UD_IFCONFIG_H
#define _UD_IFCONFIG_H
struct ud_ifcfg
{
	const char *name;      /* eth name  */
	const char *addr;      /* eth addr  */
	const char *mask;      /* eth mask  */
	const char *broadcast; /* broadcast */
};

int ud_ifsetup(struct ud_ifcfg* cfg);
int ud_ifclose(const char* eth);

#endif