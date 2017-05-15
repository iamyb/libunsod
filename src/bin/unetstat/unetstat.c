/**
********************************************************************************
Copyright (C) 2017 billyang
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
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <getopt.h>
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include "uinet_host_netstat_api.h"
#include "uinet_api_types.h"

#define handle_error(msg)\
    do{                                                                      \
        char buf[100];                                                       \
        snprintf(buf,sizeof(buf),"FILE:%s,LINE:%d,%s",__FILE__,__LINE__,msg);\
        perror(buf);                                                         \
        goto ERR;                                                  \
    }while(0)

/*---------------------------------------------------------------------------*/
static void print_tcpstat(char* buf)
{
    struct uinet_tcpstat* stat = (struct uinet_tcpstat* )buf;

    printf("tcps_connattempt:           %lu \n", stat->tcps_connattempt);
    printf("tcps_accepts:               %lu \n", stat->tcps_accepts);
    printf("tcps_connects:              %lu \n", stat->tcps_connects);
    printf("tcps_drops:                 %lu \n", stat->tcps_drops);
    printf("tcps_conndrops:             %lu \n", stat->tcps_conndrops);
    printf("tcps_minmssdrops:           %lu \n", stat->tcps_minmssdrops);
    printf("tcps_closed:                %lu \n", stat->tcps_closed);
    printf("tcps_segstimed:             %lu \n", stat->tcps_segstimed);
    printf("tcps_rttupdated:            %lu \n", stat->tcps_rttupdated);
    printf("tcps_delack:                %lu \n", stat->tcps_delack);
    printf("tcps_timeoutdrop:           %lu \n", stat->tcps_timeoutdrop);
    printf("tcps_rexmttimeo:            %lu \n", stat->tcps_rexmttimeo);
    printf("tcps_persisttimeo:          %lu \n", stat->tcps_persisttimeo);
    printf("tcps_keeptimeo:             %lu \n", stat->tcps_keeptimeo);
    printf("tcps_keepprobe:             %lu \n", stat->tcps_keepprobe);
    printf("tcps_keepdrops:             %lu \n", stat->tcps_keepdrops);

    printf("tcps_sndtotal:              %lu \n", stat->tcps_sndtotal);
    printf("tcps_sndpack:               %lu \n", stat->tcps_sndpack);
    printf("tcps_sndbyte:               %lu \n", stat->tcps_sndbyte);
    printf("tcps_sndrexmitpack:         %lu \n", stat->tcps_sndrexmitpack);
    printf("tcps_sndrexmitbyte:         %lu \n", stat->tcps_sndrexmitbyte);
    printf("tcps_sndrexmitbad:          %lu \n", stat->tcps_sndrexmitbad);
    printf("tcps_sndacks:               %lu \n", stat->tcps_sndacks);
    printf("tcps_sndprobe:              %lu \n", stat->tcps_sndprobe);
    printf("tcps_sndurg:                %lu \n", stat->tcps_sndurg);
    printf("tcps_sndwinup:              %lu \n", stat->tcps_sndwinup);
    printf("tcps_sndctrl:               %lu \n", stat->tcps_sndctrl);

    printf("tcps_rcvtotal:              %lu \n", stat->tcps_rcvtotal);
    printf("tcps_rcvpack:               %lu \n", stat->tcps_rcvpack);
    printf("tcps_rcvbyte:               %lu \n", stat->tcps_rcvbyte);
    printf("tcps_rcvbadsum:             %lu \n", stat->tcps_rcvbadsum);
    printf("tcps_rcvbadoff:             %lu \n", stat->tcps_rcvbadoff);
    printf("tcps_rcvmemdrop:            %lu \n", stat->tcps_rcvmemdrop);
    printf("tcps_rcvshort:              %lu \n", stat->tcps_rcvshort);
    printf("tcps_rcvduppack:            %lu \n", stat->tcps_rcvduppack);
    printf("tcps_rcvdupbyte:            %lu \n", stat->tcps_rcvdupbyte);
    printf("tcps_rcvpartduppack:        %lu \n", stat->tcps_rcvpartduppack);
    printf("tcps_rcvpartdupbyte:        %lu \n", stat->tcps_rcvpartdupbyte);
    printf("tcps_rcvoopack:             %lu \n", stat->tcps_rcvoopack);
    printf("tcps_rcvoobyte:             %lu \n", stat->tcps_rcvoobyte);
    printf("tcps_rcvpackafterwin:       %lu \n", stat->tcps_rcvpackafterwin);
    printf("tcps_rcvbyteafterwin:       %lu \n", stat->tcps_rcvbyteafterwin);
    printf("tcps_rcvafterclose:         %lu \n", stat->tcps_rcvafterclose);
    printf("tcps_rcvwinprobe:           %lu \n", stat->tcps_rcvwinprobe);
    printf("tcps_rcvdupack:             %lu \n", stat->tcps_rcvdupack);
    printf("tcps_rcvacktoomuch:         %lu \n", stat->tcps_rcvacktoomuch);
    printf("tcps_rcvackpack:            %lu \n", stat->tcps_rcvackpack);
    printf("tcps_rcvackbyte:            %lu \n", stat->tcps_rcvackbyte);
    printf("tcps_rcvwinupd:             %lu \n", stat->tcps_rcvwinupd);
    printf("tcps_pawsdrop:              %lu \n", stat->tcps_pawsdrop);
    printf("tcps_predack:               %lu \n", stat->tcps_predack);
    printf("tcps_preddat:               %lu \n", stat->tcps_preddat);
    printf("tcps_pcbcachemiss:          %lu \n", stat->tcps_pcbcachemiss);
    printf("tcps_cachedrtt:             %lu \n", stat->tcps_cachedrtt);
    printf("tcps_cachedrttvar:          %lu \n", stat->tcps_cachedrttvar);
    printf("tcps_cachedssthresh:        %lu \n", stat->tcps_cachedssthresh);
    printf("tcps_usedrtt:               %lu \n", stat->tcps_usedrtt);
    printf("tcps_usedrttvar:            %lu \n", stat->tcps_usedrttvar);
    printf("tcps_usedssthresh:          %lu \n", stat->tcps_usedssthresh);
    printf("tcps_persistdrop:           %lu \n", stat->tcps_persistdrop);
    printf("tcps_badsyn:                %lu \n", stat->tcps_badsyn);
    printf("tcps_mturesent:             %lu \n", stat->tcps_mturesent);
    printf("tcps_listendrop:            %lu \n", stat->tcps_listendrop);
    printf("tcps_badrst:                %lu \n", stat->tcps_badrst);

    printf("tcps_sc_added:              %lu \n", stat->tcps_sc_added);
    printf("tcps_sc_retransmitted:      %lu \n", stat->tcps_sc_retransmitted);
    printf("tcps_sc_dupsyn:             %lu \n", stat->tcps_sc_dupsyn);
    printf("tcps_sc_dropped:            %lu \n", stat->tcps_sc_dropped);
    printf("tcps_sc_completed:          %lu \n", stat->tcps_sc_completed);
    printf("tcps_sc_bucketoverflow:     %lu \n", stat->tcps_sc_bucketoverflow);
    printf("tcps_sc_cacheoverflow:      %lu \n", stat->tcps_sc_cacheoverflow);
    printf("tcps_sc_reset:              %lu \n", stat->tcps_sc_reset);
    printf("tcps_sc_stale:              %lu \n", stat->tcps_sc_stale);
    printf("tcps_sc_aborted:            %lu \n", stat->tcps_sc_aborted);
    printf("tcps_sc_badack:             %lu \n", stat->tcps_sc_badack);
    printf("tcps_sc_unreach:            %lu \n", stat->tcps_sc_unreach);
    printf("tcps_sc_zonefail:           %lu \n", stat->tcps_sc_zonefail);
    printf("tcps_sc_sendcookie:         %lu \n", stat->tcps_sc_sendcookie);
    printf("tcps_sc_recvcookie:         %lu \n", stat->tcps_sc_recvcookie);

    printf("tcps_hc_added:              %lu \n", stat->tcps_hc_added);
    printf("tcps_hc_bucketoverflow:     %lu \n", stat->tcps_hc_bucketoverflow);

    printf("tcps_finwait2_drops:        %lu \n", stat->tcps_finwait2_drops);

    printf("tcps_sack_recovery_episode: %lu \n", stat->tcps_sack_recovery_episode);
    printf("tcps_sack_rexmits:          %lu \n", stat->tcps_sack_rexmits);
    printf("tcps_sack_rexmit_bytes:     %lu \n", stat->tcps_sack_rexmit_bytes);
    printf("tcps_sack_rcv_blocks:       %lu \n", stat->tcps_sack_rcv_blocks);
    printf("tcps_sack_send_blocks:      %lu \n", stat->tcps_sack_send_blocks);
    printf("tcps_sack_sboverflow:       %lu \n", stat->tcps_sack_sboverflow);

    printf("tcps_ecn_ce:                %lu \n", stat->tcps_ecn_ce);
    printf("tcps_ecn_ect0:              %lu \n", stat->tcps_ecn_ect0);
    printf("tcps_ecn_ect1:              %lu \n", stat->tcps_ecn_ect1);
    printf("tcps_ecn_shs:               %lu \n", stat->tcps_ecn_shs);
    printf("tcps_ecn_rcwnd:             %lu \n", stat->tcps_ecn_rcwnd);

    printf("tcps_sig_rcvgoodsig:        %lu \n", stat->tcps_sig_rcvgoodsig);
    printf("tcps_sig_rcvbadsig:         %lu \n", stat->tcps_sig_rcvbadsig);
    printf("tcps_sig_err_buildsig:      %lu \n", stat->tcps_sig_err_buildsig);
    printf("tcps_sig_err_sigopt:        %lu \n", stat->tcps_sig_err_sigopt);
    printf("tcps_sig_err_nosigopt:      %lu \n", stat->tcps_sig_err_nosigopt);
}

/*---------------------------------------------------------------------------*/
static void print_ifstat(char* buf)
{
    struct uinet_ifstat* stat = (struct uinet_ifstat* )buf;

    printf("ifi_ipackets:               %lu \n", stat->ifi_ipackets);
    printf("ifi_ierrors:                %lu \n", stat->ifi_ierrors);
    printf("ifi_opackets:               %lu \n", stat->ifi_opackets);
    printf("ifi_oerrors:                %lu \n", stat->ifi_oerrors);
    printf("ifi_collisions:             %lu \n", stat->ifi_collisions);
    printf("ifi_ibytes:                 %lu \n", stat->ifi_ibytes);
    printf("ifi_obytes:                 %lu \n", stat->ifi_obytes);
    printf("ifi_imcasts:                %lu \n", stat->ifi_imcasts);
    printf("ifi_omcasts:                %lu \n", stat->ifi_omcasts);
    printf("ifi_iqdrops:                %lu \n", stat->ifi_iqdrops);
    printf("ifi_noproto:                %lu \n", stat->ifi_noproto);
    printf("ifi_hwassist:               %lu \n", stat->ifi_hwassist);
    printf("ifi_epoch:                  %lu \n", stat->ifi_epoch);
    printf("ifi_icopies:                %lu \n", stat->ifi_icopies);
    printf("ifi_izcopies:               %lu \n", stat->ifi_izcopies);
    printf("ifi_ocopies:                %lu \n", stat->ifi_ocopies);
    printf("ifi_ozcopies:               %lu \n", stat->ifi_ozcopies);
}
/*---------------------------------------------------------------------------*/
static void print_udpstat(char* buf)
{
}
static void print_ipstat(char* buf)
{
}

struct s_matrix {
    unsigned int type;
    unsigned int size;
    void (*print)(char*);
};

typedef enum e_flag {
    e_flag_tcp = 1,
    e_flag_udp = 2,
    e_flag_if  = 3,
    e_flag_ip  = 4,
    e_flag_none
} e_flag;

static struct s_matrix matrix[] = {
    {0, 0, NULL}, /*not used*/
    {NS_TCPSTAT, sizeof(struct uinet_tcpstat),print_tcpstat},
    {NS_UDPSTAT, 0,print_udpstat},
    {NS_IFSTAT,  sizeof(struct uinet_ifstat),print_ifstat},
    {NS_IPSTAT,  0,print_ipstat},
};

/*---------------------------------------------------------------------------*/
static int connect_to_server(void)
{
    int s;
    struct sockaddr_un sun;
    int r;
    char *spath;

    spath = getenv("SYSCTL_SOCK");
    if (spath == NULL)
        spath = "/tmp/sysctl.sock";

    bzero(&sun, sizeof(sun));

    strcpy(sun.sun_path, spath);
    sun.sun_family = AF_UNIX;

    s = socket(AF_UNIX, SOCK_STREAM, 0);
    if (s < 0) {
        handle_error("socket open failed");
    }

    r = connect(s, (struct sockaddr *) &sun, sizeof(struct sockaddr_un));
    if (r < 0) {
        handle_error("connect failed");
    }
    return s;
ERR:
    return (-1);
}

/*---------------------------------------------------------------------------*/
void main(int argc, char** argv)
{
    struct ns_netstat_req req;
    char* ethname = NULL;
    e_flag flag = e_flag_none;
    char c;

    while ((c = getopt (argc, argv, "i:tumr")) != -1) {
        switch(c) {
        case 'i':
            ethname = optarg;
            break;
        case 't':
            flag = e_flag_tcp;
            break;
        case 'u':
            flag = e_flag_udp;
            break;
        case 'm':
            flag = e_flag_if;
            break;
        case 'r':
            flag = e_flag_ip;
            break;
        default:
            handle_error("invalid parameters!");
        }
    }

    int sock = connect_to_server();
    if(sock < 0)
        handle_error("connect failed!");

    char *buf = malloc(matrix[flag].size);
    if(buf == NULL)
        handle_error("malloc failed!");

    /* prepare the request */
    req.type = matrix[flag].type;

    /* send it to the server */
    int n = send(sock, &req, sizeof(req), 0);
    if(n < 0)
        handle_error("msg send failed!");

    /* recv until get all data */
    while((n = recv(sock, buf, matrix[flag].size, 0)) == 0);
    if(n != matrix[flag].size)
        handle_error("msg recv failed!");

    /* print the stats */
    matrix[flag].print(buf);
ERR:
    if(buf != NULL)
        free(buf);

    if(sock != -1)
        close(sock);
}
