#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <net/if.h>
#include <iproute/libnetlink.h>
#include <linux/pkt_sched.h>

#include "wnetemu.h"

#define TCA_BUF_MAX	(64*1024)


struct rtnl_request {
	struct nlmsghdr n;
        struct tcmsg t;
        char buf[TCA_BUF_MAX];
};

static struct if_nameindex interfaces[] = {
//	{ 0, "eth0" },
	{ 0, "eth1" }
};

static struct rtnl_handle rth;
static struct rtnl_request req;


void init_netem (void)
{
	int i;
	
	if (rtnl_open(&rth, 0) < 0) {
		fprintf (stderr, "Cannot open rtnetlink!!!\n");
		exit (EXIT_FAILURE);
	}

	/* no need to use libnetlink interface name-to-index mapping cache */
	for (i = 0; i < sizeof(interfaces)/sizeof(interfaces[0]); ++i) {
		if ((interfaces[i].if_index = if_nametoindex(interfaces[i].if_name)) == 0) {
			fprintf (stderr, "Cannot find device \"%s\"\n", interfaces[i].if_name);
			exit (EXIT_FAILURE);
		}
	}
}

void setup_netem (const channel_state_info_t *csi)
{
	/* for "tc qdisc add"     we have to use NLM_F_REQUEST|NLM_F_EXCL|NLM_F_CREATE
	 * for "tc qdisc change"  we have to use NLM_F_REQUEST 
	 * for "tc qdisc replace" we have to use NLM_F_REQUEST|NLM_F_REPLACE */
	static int request_count = 0;
	int flags = request_count++ ? NLM_F_REPLACE : NLM_F_EXCL|NLM_F_CREATE;
	static const char *netem_name = "netem";
	struct tc_netem_qopt qopt;
#if 0
	struct tc_netem_corr corr;
	struct tc_netem_reorder reorder;
	struct tc_netem_corrupt corrupt;
#endif
	int i;

	/* tc qdisc add/replace dev ethX root handle 1: netem ... */
	memset (&req, 0, sizeof(req));
        req.n.nlmsg_len   = NLMSG_LENGTH (sizeof(req.t));
        req.n.nlmsg_flags = NLM_F_REQUEST | flags;
        req.n.nlmsg_type  = RTM_NEWQDISC;
        req.t.tcm_family  = AF_UNSPEC;
	req.t.tcm_handle  = 1<<16;
	req.t.tcm_parent  = TC_H_ROOT;

	memset (&qopt, 0, sizeof(qopt));
	qopt.latency   = csi->delay;          /* added delay (us) */
        qopt.jitter    = csi->jitter;         /* random jitter in latency (us) */
        qopt.loss      = csi->loss_rate;      /* random packet loss (0=none ~0=100%) */
        qopt.duplicate = csi->duplicate_rate; /* random packet dup  (0=none ~0=100%) */
        qopt.limit     = csi->queue_size;     /* fifo limit (packets) */
        qopt.gap       = csi->reorder_gap;    /* re-ordering gap (0 for none) */

#if 0
	memset (&corr, 0, sizeof(corr));
	corr.delay_corr = delay_corr;    /* delay correlation */
	corr.loss_corr  = loss_corr;     /* packet loss correlation */
	corr.dup_corr   = dup_corr;      /* duplicate correlation  */
	
	memset (&reorder, 0, sizeof(reorder));
	reorder.probability = reorder_prob; 
	reorder.correlation = reorder_corr;
	
	memset (&corrupt, 0, sizeof(corrupt));
	corrupt.probability = corrupt_prob; 
	corrupt.correlation = corrupt_corr;
#endif
	
	if (addattr_l(&req.n, sizeof(req), TCA_KIND, netem_name, strlen(netem_name)+1) < 0) {
		fputs ("Cannot add attribute netem_name!!!\n", stderr);
		exit (EXIT_FAILURE);
	}

	if (addattr_l(&req.n, sizeof(req), TCA_OPTIONS, &qopt, sizeof(qopt)) < 0) {
		fputs ("Cannot add attribute netem_qopt!!!\n", stderr);
		exit (EXIT_FAILURE);
	}

#if 0
	if (addattr_l(&req.n, sizeof(req), TCA_NETEM_CORR, &corr, sizeof(corr)) < 0) {
		fputs ("Cannot add attribute netem_corr!!!\n", stderr);
		exit (EXIT_FAILURE);
	}

	if (addattr_l(&req.n, sizeof(req), TCA_NETEM_REORDER, &reorder, sizeof(reorder)) < 0) {
		fputs ("Cannot add attribute netem_reorder!!!\n", stderr);
		exit (EXIT_FAILURE);
	}

        if (addattr_l(&req.n, sizeof(req), TCA_NETEM_CORRUPT, &corrupt, sizeof(corrupt)) < 0) {
		fputs ("Cannot add attribute netem_corrupt!!!\n", stderr);
		exit (EXIT_FAILURE);
        }

	/* make sure qopt.latency and qopt.jitter are not 0 when using delay distribution */
	if (addattr_l(&req.n, sizeof(req), TCA_NETEM_DELAY_DIST, dist_data, 
		      dist_size * sizeof(dist_data[0])) < 0) {
		fputs ("Cannot add attribute netem delay distribution!!!\n", stderr);
		exit (EXIT_FAILURE);
        }
#endif

	for (i = 0; i < sizeof(interfaces)/sizeof(interfaces[0]); ++i) {
		req.t.tcm_ifindex = interfaces[i].if_index;
		
		if (rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL) < 0) {
			fputs ("Cannot call netem!!!\n", stderr);
			exit (EXIT_FAILURE);
		}
	}
}

void close_netem (void)
{
	int i;

	/* tc qdisc del dev ethX root */
	memset (&req, 0, sizeof(req));
        req.n.nlmsg_len   = NLMSG_LENGTH (sizeof(req.t));
        req.n.nlmsg_flags = NLM_F_REQUEST;
        req.n.nlmsg_type  = RTM_DELQDISC;
        req.t.tcm_family  = AF_UNSPEC;
	req.t.tcm_handle  = 1<<16;
	req.t.tcm_parent  = TC_H_ROOT;
	
	for (i = 0; i < sizeof(interfaces)/sizeof(interfaces[0]); ++i) {
		req.t.tcm_ifindex = interfaces[i].if_index;
		
		if (rtnl_talk(&rth, &req.n, 0, 0, NULL, NULL, NULL) < 0) {
			fputs ("Cannot close netem!!!\n", stderr);
			exit (EXIT_FAILURE);
		}
	}

	rtnl_close (&rth);
}


