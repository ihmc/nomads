#ifdef IW 
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>

#include <list>
#include <iostream>
#include <memory> 


#include "FTypes.h"
#include "nl80211.h"
#include "IWStationDump.h"
#include "IWStationDumpTable.h"
#include "NLDumpManager.h"


#define BIT(x) (1ULL<<(x))
#define ETH_ALEN 6

using namespace IHMC_NETSENSOR;
using namespace NOMADSUtil;


enum plink_state {
    LISTEN,
    OPN_SNT,
    OPN_RCVD,
    CNF_RCVD,
    ESTAB,
    HOLDING,
    BLOCKED
};

void mac_addr_n2a(char *mac_addr, unsigned char *arg)
{
	int i, l;

	l = 0;
	for (i = 0; i < ETH_ALEN ; i++) {
		if (i == 0) {
			sprintf(mac_addr+l, "%02x", arg[i]);
			l += 2;
		} else {
			sprintf(mac_addr+l, ":%02x", arg[i]);
			l += 3;
		}
	}
}

int mac_addr_a2n(unsigned char *mac_addr, char *arg)
{
	int i;

	for (i = 0; i < ETH_ALEN ; i++) {
		int temp;
		char *cp = strchr(arg, ':');
		if (cp) {
			*cp = 0;
			cp++;
		}
		if (sscanf(arg, "%x", &temp) != 1)
			return -1;
		if (temp < 0 || temp > 255)
			return -1;

		mac_addr[i] = temp;
		if (!cp)
			break;
		arg = cp;
	}
	if (i < ETH_ALEN - 1)
		return -1;

	return 0;
}



static void print_power_mode(struct nlattr *a, char *buf)
{
	enum nl80211_mesh_power_mode pm = nla_get_u32(a);

	switch (pm) {
	case NL80211_MESH_POWER_ACTIVE:
		sprintf(buf,"ACTIVE");
		break;
	case NL80211_MESH_POWER_LIGHT_SLEEP:
		sprintf(buf,"LIGHT SLEEP");
		break;
	case NL80211_MESH_POWER_DEEP_SLEEP:
		sprintf(buf,"DEEP SLEEP");
		break;
	default:
		sprintf(buf,"UNKNOWN");
		break;
	}
}


void parse_tx_bitrate(struct nlattr *bitrate_attr, char *buf, int buflen)
{
	int rate = 0;
	char *pos = buf;
	struct nlattr *rinfo[NL80211_RATE_INFO_MAX + 1];
	static struct nla_policy rate_policy[NL80211_RATE_INFO_MAX + 1];
	rate_policy[NL80211_RATE_INFO_BITRATE].type = NLA_U16;
	rate_policy[NL80211_RATE_INFO_BITRATE32].type = NLA_U32;
	rate_policy[NL80211_RATE_INFO_MCS].type = NLA_U8;
	rate_policy[NL80211_RATE_INFO_40_MHZ_WIDTH].type = NLA_FLAG;
	rate_policy[NL80211_RATE_INFO_SHORT_GI].type = NLA_FLAG;

	if (nla_parse_nested(rinfo, NL80211_RATE_INFO_MAX,
			     bitrate_attr, rate_policy)) {
		//snprintf(buf, buflen, "failed to parse nested rate attributes!");
		return;
	}

	if (rinfo[NL80211_RATE_INFO_BITRATE32])
		rate = nla_get_u32(rinfo[NL80211_RATE_INFO_BITRATE32]);
	else if (rinfo[NL80211_RATE_INFO_BITRATE])
		rate = nla_get_u16(rinfo[NL80211_RATE_INFO_BITRATE]);
	if (rate > 0)
		pos += snprintf(pos, buflen - (pos - buf),
				"%d.%d MBit/s", rate / 10, rate % 10);

	if (rinfo[NL80211_RATE_INFO_MCS])
		pos += snprintf(pos, buflen - (pos - buf),
				" MCS %d", nla_get_u8(rinfo[NL80211_RATE_INFO_MCS]));
	if (rinfo[NL80211_RATE_INFO_VHT_MCS])
		pos += snprintf(pos, buflen - (pos - buf),
				" VHT-MCS %d", nla_get_u8(rinfo[NL80211_RATE_INFO_VHT_MCS]));
	if (rinfo[NL80211_RATE_INFO_40_MHZ_WIDTH])
		pos += snprintf(pos, buflen - (pos - buf), " 40MHz");
	if (rinfo[NL80211_RATE_INFO_80_MHZ_WIDTH])
		pos += snprintf(pos, buflen - (pos - buf), " 80MHz");
	if (rinfo[NL80211_RATE_INFO_80P80_MHZ_WIDTH])
		pos += snprintf(pos, buflen - (pos - buf), " 80P80MHz");
	if (rinfo[NL80211_RATE_INFO_160_MHZ_WIDTH])
		pos += snprintf(pos, buflen - (pos - buf), " 160MHz");
	if (rinfo[NL80211_RATE_INFO_SHORT_GI])
		pos += snprintf(pos, buflen - (pos - buf), " short GI");
	if (rinfo[NL80211_RATE_INFO_VHT_NSS])
		pos += snprintf(pos, buflen - (pos - buf),
				" VHT-NSS %d", nla_get_u8(rinfo[NL80211_RATE_INFO_VHT_NSS]));
}

NLDumpManager::NLDumpManager(){}
NLDumpManager::~NLDumpManager(){}

int NLDumpManager::stationDumpCallback(struct nl_msg *pMsg, void *arg)
{
	IWStationDump Res;
	struct nlattr *tb[NL80211_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = (genlmsghdr*) nlmsg_data(nlmsg_hdr(pMsg));
	struct nlattr *sinfo[NL80211_STA_INFO_MAX + 1];
	char mac_addr[20], state_name[10], dev[20];
	struct nl80211_sta_flag_update *sta_flags;
	static struct nla_policy stats_policy[NL80211_STA_INFO_MAX + 1];
	IWStationDumpTable * pTable = (IWStationDumpTable *) arg;
	stats_policy[NL80211_STA_INFO_INACTIVE_TIME].type = NLA_U32;
	stats_policy[NL80211_STA_INFO_RX_BYTES].type = NLA_U32;
	stats_policy[NL80211_STA_INFO_TX_BYTES].type = NLA_U32 ;
	stats_policy[NL80211_STA_INFO_RX_PACKETS].type = NLA_U32;
	stats_policy[NL80211_STA_INFO_TX_PACKETS].type = NLA_U32;
	stats_policy[NL80211_STA_INFO_SIGNAL].type = NLA_U8;
	stats_policy[NL80211_STA_INFO_T_OFFSET].type = NLA_U64;
	stats_policy[NL80211_STA_INFO_TX_BITRATE].type = NLA_NESTED;
	stats_policy[NL80211_STA_INFO_LLID].type = NLA_U16;
	stats_policy[NL80211_STA_INFO_PLID].type = NLA_U16;
	stats_policy[NL80211_STA_INFO_PLINK_STATE].type = NLA_U8;
	stats_policy[NL80211_STA_INFO_TX_RETRIES].type = NLA_U32;
	stats_policy[NL80211_STA_INFO_TX_FAILED].type = NLA_U32;
	stats_policy[NL80211_STA_INFO_STA_FLAGS].minlen = sizeof(struct nl80211_sta_flag_update);
	stats_policy[NL80211_STA_INFO_LOCAL_PM].type = NLA_U32;
	stats_policy[NL80211_STA_INFO_PEER_PM].type = NLA_U32;
	stats_policy[NL80211_STA_INFO_NONPEER_PM].type = NLA_U32;
    nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);
    
    if (!tb[NL80211_ATTR_STA_INFO]) {
		fprintf(stderr, "sta stats missing!\n");
        goto failure;
	}
	if (nla_parse_nested(sinfo, NL80211_STA_INFO_MAX,
			     tb[NL80211_ATTR_STA_INFO],
			     stats_policy)) {
		fprintf(stderr, "failed to parse nested attributes!\n");
		goto failure;
	}
	mac_addr_n2a(mac_addr, (unsigned char*)nla_data(tb[NL80211_ATTR_MAC]));
	if_indextoname(nla_get_u32(tb[NL80211_ATTR_IFINDEX]), dev);
	Res.sMacAddr = mac_addr;
	Res.sInterface = dev;
	if (sinfo[NL80211_STA_INFO_INACTIVE_TIME])
		Res.ui32InactiveTime = nla_get_u32(sinfo[NL80211_STA_INFO_INACTIVE_TIME]);
	if (sinfo[NL80211_STA_INFO_RX_BYTES])
		Res.ui32RxBytes = nla_get_u32(sinfo[NL80211_STA_INFO_RX_BYTES]);
	if (sinfo[NL80211_STA_INFO_RX_PACKETS])
		Res.ui32RxPackets = nla_get_u32(sinfo[NL80211_STA_INFO_RX_PACKETS]);
	if (sinfo[NL80211_STA_INFO_TX_BYTES])
		Res.ui32TxBytes = nla_get_u32(sinfo[NL80211_STA_INFO_TX_BYTES]);
	if (sinfo[NL80211_STA_INFO_TX_PACKETS])
		Res.ui32TxPackets = nla_get_u32(sinfo[NL80211_STA_INFO_TX_PACKETS]);
	if (sinfo[NL80211_STA_INFO_TX_RETRIES])
		Res.ui32TxRetries = nla_get_u32(sinfo[NL80211_STA_INFO_TX_RETRIES]);
	if (sinfo[NL80211_STA_INFO_TX_FAILED])
		Res.ui32TxFailed = nla_get_u32(sinfo[NL80211_STA_INFO_TX_FAILED]);
	if (sinfo[NL80211_STA_INFO_SIGNAL])
		Res.ui8SignalPower = (uint8)nla_get_u8(sinfo[NL80211_STA_INFO_SIGNAL]);
	if (sinfo[NL80211_STA_INFO_SIGNAL_AVG])
		Res.ui8SignalPowerAvg = (uint8)nla_get_u8(sinfo[NL80211_STA_INFO_SIGNAL_AVG]);
	if (sinfo[NL80211_STA_INFO_T_OFFSET])
		Res.ui64TOffset = (uint64)nla_get_u64(sinfo[NL80211_STA_INFO_T_OFFSET]);

	if (sinfo[NL80211_STA_INFO_TX_BITRATE]) {
		char buf[100];

		parse_tx_bitrate(sinfo[NL80211_STA_INFO_TX_BITRATE], buf, sizeof(buf));
		Res.sTxBitrate = String(buf);
	}
    if (sinfo[NL80211_STA_INFO_LLID])
    	Res.meshLLID = nla_get_u16(sinfo[NL80211_STA_INFO_LLID]);
	if (sinfo[NL80211_STA_INFO_PLID])
		Res.meshPLID = nla_get_u16(sinfo[NL80211_STA_INFO_PLID]);
	if (sinfo[NL80211_STA_INFO_PLINK_STATE]) {
		switch (nla_get_u8(sinfo[NL80211_STA_INFO_PLINK_STATE])) {
		case LISTEN:
			Res.sLinkState = "LISTEN";
			break;
		case OPN_SNT:
			Res.sLinkState = "OPN_SNT";
			break;
		case OPN_RCVD:
			Res.sLinkState = "OPN_RCVD";
			break;
		case CNF_RCVD:
			Res.sLinkState = "CNF_RCVD";
			break;
		case ESTAB:
			Res.sLinkState = "ESTAB";
			break;
		case HOLDING:
			Res.sLinkState = "HOLDING";
			break;
		case BLOCKED:
			Res.sLinkState = "BLOCKED";
			break;
		default:
			Res.sLinkState = "UNKNOWN";
			break;
		}
	}
	if (sinfo[NL80211_STA_INFO_LOCAL_PM]) {
		char buf[50];
		printf("\n\tmesh local PS mode:\t");
		print_power_mode(sinfo[NL80211_STA_INFO_LOCAL_PM],buf);
		Res.sLocalPowerMode = String(buf);
	}
	if (sinfo[NL80211_STA_INFO_PEER_PM]) {
		char buf[50];
		printf("\n\tmesh peer PS mode:\t");
		print_power_mode(sinfo[NL80211_STA_INFO_PEER_PM],buf);
		Res.sPeerPowerMode = String(buf);
	}
	/*if (sinfo[NL80211_STA_INFO_NONPEER_PM]) {
		char buf[50];
		printf("\n\tmesh non-peer PS mode:\t");
		print_power_mode(sinfo[NL80211_STA_INFO_NONPEER_PM],buf,sizeof(buf));
	}*/

	if (sinfo[NL80211_STA_INFO_STA_FLAGS]) {
		sta_flags = (struct nl80211_sta_flag_update *)
			    nla_data(sinfo[NL80211_STA_INFO_STA_FLAGS]);

		if (sta_flags->mask & BIT(NL80211_STA_FLAG_AUTHORIZED)) {
			if (sta_flags->set & BIT(NL80211_STA_FLAG_AUTHORIZED))
				Res.bAuthorized = true;
			else
				Res.bAuthorized = false;
		}

		if (sta_flags->mask & BIT(NL80211_STA_FLAG_AUTHENTICATED)) {
			if (sta_flags->set & BIT(NL80211_STA_FLAG_AUTHENTICATED))
				Res.bAuthenticated = true;
			else
				Res.bAuthenticated = false;
		}

		if (sta_flags->mask & BIT(NL80211_STA_FLAG_SHORT_PREAMBLE)) {
			if (sta_flags->set & BIT(NL80211_STA_FLAG_SHORT_PREAMBLE))
				Res.sPreamble = "short";
			else
				Res.sPreamble = "long";
		}

		if (sta_flags->mask & BIT(NL80211_STA_FLAG_WME)) {
			if (sta_flags->set & BIT(NL80211_STA_FLAG_WME))
				Res.WMM_WME = true;
			else
				Res.WMM_WME = false;
		}

		if (sta_flags->mask & BIT(NL80211_STA_FLAG_MFP)) {
			if (sta_flags->set & BIT(NL80211_STA_FLAG_MFP))
				Res.MFP = true;
			else
				Res.MFP = false;
		}

		if (sta_flags->mask & BIT(NL80211_STA_FLAG_TDLS_PEER)) {
			if (sta_flags->set & BIT(NL80211_STA_FLAG_TDLS_PEER))
				Res.TDLS = true;
			else
				Res.TDLS = false;
		}
	}
	pTable->put(& Res);
	return NL_SKIP;

failure:
    
    return NL_SKIP;
}

int NLDumpManager::debug(struct nl_msg *pMsg, void *arg)
{
    //arg must be the class proto to send back to netsensor
    
    struct nlattr *tb[NL80211_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = (genlmsghdr*) nlmsg_data(nlmsg_hdr(pMsg));
	struct nlattr *sinfo[NL80211_STA_INFO_MAX + 1];
	char mac_addr[20], state_name[10], dev[20];
	struct nl80211_sta_flag_update *sta_flags;
	static struct nla_policy stats_policy[NL80211_STA_INFO_MAX + 1]; 
	stats_policy[NL80211_STA_INFO_INACTIVE_TIME].type = NLA_U32;
	stats_policy[NL80211_STA_INFO_RX_BYTES].type = NLA_U32;
	stats_policy[NL80211_STA_INFO_TX_BYTES].type = NLA_U32 ;
	stats_policy[NL80211_STA_INFO_RX_PACKETS].type = NLA_U32;
	stats_policy[NL80211_STA_INFO_TX_PACKETS].type = NLA_U32;
	stats_policy[NL80211_STA_INFO_SIGNAL].type = NLA_U8;
	stats_policy[NL80211_STA_INFO_T_OFFSET].type = NLA_U64;
	stats_policy[NL80211_STA_INFO_TX_BITRATE].type = NLA_NESTED;
	stats_policy[NL80211_STA_INFO_LLID].type = NLA_U16;
	stats_policy[NL80211_STA_INFO_PLID].type = NLA_U16;
	stats_policy[NL80211_STA_INFO_PLINK_STATE].type = NLA_U8;
	stats_policy[NL80211_STA_INFO_TX_RETRIES].type = NLA_U32;
	stats_policy[NL80211_STA_INFO_TX_FAILED].type = NLA_U32;
	stats_policy[NL80211_STA_INFO_STA_FLAGS].minlen = sizeof(struct nl80211_sta_flag_update);
	stats_policy[NL80211_STA_INFO_LOCAL_PM].type = NLA_U32;
	stats_policy[NL80211_STA_INFO_PEER_PM].type = NLA_U32;
	stats_policy[NL80211_STA_INFO_NONPEER_PM].type = NLA_U32;
    nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);
    if (!tb[NL80211_ATTR_STA_INFO]) {
		fprintf(stderr, "sta stats missing!\n");
        goto failure;
	}
	if (nla_parse_nested(sinfo, NL80211_STA_INFO_MAX,
			     tb[NL80211_ATTR_STA_INFO],
			     stats_policy)) {
		fprintf(stderr, "failed to parse nested attributes!\n");
		goto failure;
	}
	mac_addr_n2a(mac_addr, (unsigned char*)nla_data(tb[NL80211_ATTR_MAC]));
	if_indextoname(nla_get_u32(tb[NL80211_ATTR_IFINDEX]), dev);
    printf("Station %s (on %s)", mac_addr, dev);
    if (sinfo[NL80211_STA_INFO_INACTIVE_TIME])
		printf("\n\tinactive time:\t%u ms",	nla_get_u32(sinfo[NL80211_STA_INFO_INACTIVE_TIME]));
	if (sinfo[NL80211_STA_INFO_RX_BYTES])
		printf("\n\trx bytes:\t%u",
			nla_get_u32(sinfo[NL80211_STA_INFO_RX_BYTES]));
	if (sinfo[NL80211_STA_INFO_RX_PACKETS])
		printf("\n\trx packets:\t%u",
			nla_get_u32(sinfo[NL80211_STA_INFO_RX_PACKETS]));
	if (sinfo[NL80211_STA_INFO_TX_BYTES])
		printf("\n\ttx bytes:\t%u",
			nla_get_u32(sinfo[NL80211_STA_INFO_TX_BYTES]));
	if (sinfo[NL80211_STA_INFO_TX_PACKETS])
		printf("\n\ttx packets:\t%u",
			nla_get_u32(sinfo[NL80211_STA_INFO_TX_PACKETS]));
	if (sinfo[NL80211_STA_INFO_TX_RETRIES])
		printf("\n\ttx retries:\t%u",
			nla_get_u32(sinfo[NL80211_STA_INFO_TX_RETRIES]));
	if (sinfo[NL80211_STA_INFO_TX_FAILED])
		printf("\n\ttx failed:\t%u",
			nla_get_u32(sinfo[NL80211_STA_INFO_TX_FAILED]));
	if (sinfo[NL80211_STA_INFO_SIGNAL])
		printf("\n\tsignal:  \t%d dBm",
			(int8_t)nla_get_u8(sinfo[NL80211_STA_INFO_SIGNAL]));
	if (sinfo[NL80211_STA_INFO_SIGNAL_AVG])
		printf("\n\tsignal avg:\t%d dBm",
			(int8_t)nla_get_u8(sinfo[NL80211_STA_INFO_SIGNAL_AVG]));
	if (sinfo[NL80211_STA_INFO_T_OFFSET])
		printf("\n\tToffset:\t%lld us",
			(unsigned long long)nla_get_u64(sinfo[NL80211_STA_INFO_T_OFFSET]));
    
    if (sinfo[NL80211_STA_INFO_TX_BITRATE]) {
		char buf[100];

		parse_tx_bitrate(sinfo[NL80211_STA_INFO_TX_BITRATE], buf, sizeof(buf));
		printf("\n\ttx bitrate:\t%s", buf);
	}
    if (sinfo[NL80211_STA_INFO_LLID])
		printf("\n\tmesh llid:\t%d",
			nla_get_u16(sinfo[NL80211_STA_INFO_LLID]));
	if (sinfo[NL80211_STA_INFO_PLID])
		printf("\n\tmesh plid:\t%d",
			nla_get_u16(sinfo[NL80211_STA_INFO_PLID]));
	if (sinfo[NL80211_STA_INFO_PLINK_STATE]) {
		switch (nla_get_u8(sinfo[NL80211_STA_INFO_PLINK_STATE])) {
		case LISTEN:
			strcpy(state_name, "LISTEN");
			break;
		case OPN_SNT:
			strcpy(state_name, "OPN_SNT");
			break;
		case OPN_RCVD:
			strcpy(state_name, "OPN_RCVD");
			break;
		case CNF_RCVD:
			strcpy(state_name, "CNF_RCVD");
			break;
		case ESTAB:
			strcpy(state_name, "ESTAB");
			break;
		case HOLDING:
			strcpy(state_name, "HOLDING");
			break;
		case BLOCKED:
			strcpy(state_name, "BLOCKED");
			break;
		default:
			strcpy(state_name, "UNKNOWN");
			break;
		}
		printf("\n\tmesh plink:\t%s", state_name);
	}
	if (sinfo[NL80211_STA_INFO_LOCAL_PM]) {
		printf("\n\tmesh local PS mode:\t");
		char buf [50];
		print_power_mode(sinfo[NL80211_STA_INFO_LOCAL_PM],buf);
		printf("%s\n",buf);
	}
	if (sinfo[NL80211_STA_INFO_PEER_PM]) {
		printf("\n\tmesh peer PS mode:\t");
		char buf [50];
		print_power_mode(sinfo[NL80211_STA_INFO_PEER_PM],buf);
		printf("%s\n",buf);
	}
	if (sinfo[NL80211_STA_INFO_NONPEER_PM]) {
		printf("\n\tmesh non-peer PS mode:\t");
		char buf [50];
		print_power_mode(sinfo[NL80211_STA_INFO_NONPEER_PM],buf);
		printf("%s\n",buf);
	}

	if (sinfo[NL80211_STA_INFO_STA_FLAGS]) {
		sta_flags = (struct nl80211_sta_flag_update *)
			    nla_data(sinfo[NL80211_STA_INFO_STA_FLAGS]);

		if (sta_flags->mask & BIT(NL80211_STA_FLAG_AUTHORIZED)) {
			printf("\n\tauthorized:\t");
			if (sta_flags->set & BIT(NL80211_STA_FLAG_AUTHORIZED))
				printf("yes");
			else
				printf("no");
		}

		if (sta_flags->mask & BIT(NL80211_STA_FLAG_AUTHENTICATED)) {
			printf("\n\tauthenticated:\t");
			if (sta_flags->set & BIT(NL80211_STA_FLAG_AUTHENTICATED))
				printf("yes");
			else
				printf("no");
		}

		if (sta_flags->mask & BIT(NL80211_STA_FLAG_SHORT_PREAMBLE)) {
			printf("\n\tpreamble:\t");
			if (sta_flags->set & BIT(NL80211_STA_FLAG_SHORT_PREAMBLE))
				printf("short");
			else
				printf("long");
		}

		if (sta_flags->mask & BIT(NL80211_STA_FLAG_WME)) {
			printf("\n\tWMM/WME:\t");
			if (sta_flags->set & BIT(NL80211_STA_FLAG_WME))
				printf("yes");
			else
				printf("no");
		}

		if (sta_flags->mask & BIT(NL80211_STA_FLAG_MFP)) {
			printf("\n\tMFP:\t\t");
			if (sta_flags->set & BIT(NL80211_STA_FLAG_MFP))
				printf("yes");
			else
				printf("no");
		}

		if (sta_flags->mask & BIT(NL80211_STA_FLAG_TDLS_PEER)) {
			printf("\n\tTDLS peer:\t");
			if (sta_flags->set & BIT(NL80211_STA_FLAG_TDLS_PEER))
				printf("yes");
			else
				printf("no");
		}
	}
	printf("\n");
	return NL_STOP;

failure:
    //delete arg;
    arg = NULL;
    return NL_STOP;
}
#endif