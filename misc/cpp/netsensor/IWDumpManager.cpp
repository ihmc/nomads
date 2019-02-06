#ifdef IW
#include <errno.h>
#include <stdio.h>
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
#include "nl80211.h"

#include <string.h>
#include <memory> 
#include <chrono>

#include "IWDumpManager.h"




//#define checkAndLogMsg if (pLogger) pLogger->logMsg
#define BIT(x) (1ULL<<(x))
#define ETH_ALEN 6

using namespace IHMC_NETSENSOR;
using namespace std;

void IWParsingUtils::mac_addr_n2a(char *mac_addr, unsigned char *arg)
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

int IWParsingUtils::mac_addr_a2n(unsigned char *mac_addr, char *arg)
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



void IWParsingUtils::print_power_mode(struct nlattr *a, char *buf)
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


void IWParsingUtils::parse_tx_bitrate(struct nlattr *bitrate_attr, char *buf, int buflen)
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



int IWCallBacks::stationDumpCallback(struct nl_msg *pMsg, void *arg)
{
    IWStationDump Res;

    struct nlattr *tb[NL80211_ATTR_MAX + 1];
    struct genlmsghdr *gnlh = (genlmsghdr*) nlmsg_data(nlmsg_hdr(pMsg));
    struct nlattr *sinfo[NL80211_STA_INFO_MAX + 1];
    char mac_addr[20], state_name[10], dev[20];
    struct nl80211_sta_flag_update *sta_flags;
    static struct nla_policy stats_policy[NL80211_STA_INFO_MAX + 1];

    IWDumpList * pList = (IWDumpList *) arg;
    

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
    IWParsingUtils::mac_addr_n2a (mac_addr, (unsigned char*) nla_data(tb[NL80211_ATTR_MAC]));
    if_indextoname (nla_get_u32 (tb[NL80211_ATTR_IFINDEX]), dev);
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

        IWParsingUtils::parse_tx_bitrate(sinfo[NL80211_STA_INFO_TX_BITRATE], buf, sizeof(buf));
        Res.sTxBitrate = buf;
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
        IWParsingUtils::print_power_mode(sinfo[NL80211_STA_INFO_LOCAL_PM],buf);
        Res.sLocalPowerMode = buf;
    }
    if (sinfo[NL80211_STA_INFO_PEER_PM]) {
        char buf[50];
        IWParsingUtils::print_power_mode(sinfo[NL80211_STA_INFO_PEER_PM],buf);
        Res.sPeerPowerMode = buf;
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
    pList->put(Res);
    return NL_SKIP;

failure:
    
    return NL_SKIP;
}

/*
void IWDumpManager::thread (const char * pszIname)
{
    struct nl80211_state nlstate;
    struct nl_cb *pCallback,*pSocketCallback;
    struct nl_msg *pMsg;
    char *pBuf;
    signed long long interfaceID =0;
    
    nlstate.nl_sock = nl_socket_alloc();
    if (!nlstate.nl_sock) {
        //error fprintf(stderr, "Failed to allocate netlink socket.\n");
        //return -ENOMEM;
        return;
    }
    
    nl_socket_set_buffer_size (nlstate.nl_sock, 8192, 8192);

    if (genl_connect (nlstate.nl_sock)) {
        //error fprintf(stderr, "Failed to connect to generic netlink.\n");
        nl_socket_free (state->nl_sock);
        //return -ENOLINK;
        return;
    }

    nlstate.nl80211_id = genl_ctrl_resolve (nlstate.nl_sock, "nl80211");
    if (nlstate.nl80211_id < 0) {
        //error fprintf(stderr, "nl80211 not found.\n");
        nl_socket_free (nlstate.nl_sock);
        //return -ENOENT
        return;
    }

    interfaceID = if_nametoindex (pszIname);
    if(interfaceID == 0){
        //log error iface not found
        return ;
    }
    pMsg = nlmsg_alloc();
    pCallback = nl_cb_alloc (NL_CB_DEFAULT);
    pSocketCallback = nl_cb_alloc (NL_CB_DEFAULT);
    genlmsg_put (pMsg, 0, 0, nlstate.nl80211_id, 0, NLM_F_DUMP, NL80211_CMD_GET_STATION, 0);
    nla_put_u32 (pMsg, NL80211_ATTR_IFINDEX, interfaceID);
    
    nl_cb_set (pCallback, NL_CB_VALID, NL_CB_CUSTOM, NLDumpManager::debug , _pStationDumpTable);
    nl_socket_set_cb (nlstate.nl_sock, pSocketCallback);
    if (nl_send_auto_complete(nlstate.nl_sock, pMsg) < 0) {
        //error
        return;
    }

    nl_recvmsgs (nlstate.nl_sock, pCallback);
}
*/
int IWCallBacks::debug(struct nl_msg *pMsg, void *arg)
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
    IWParsingUtils::mac_addr_n2a(mac_addr, (unsigned char*)nla_data(tb[NL80211_ATTR_MAC]));
    if_indextoname(nla_get_u32(tb[NL80211_ATTR_IFINDEX]), dev);
    printf("Station %s (on %s)", mac_addr, dev);
    if (sinfo[NL80211_STA_INFO_INACTIVE_TIME])
        printf("\n\tinactive time:\t%u ms", nla_get_u32(sinfo[NL80211_STA_INFO_INACTIVE_TIME]));
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

        IWParsingUtils::parse_tx_bitrate(sinfo[NL80211_STA_INFO_TX_BITRATE], buf, sizeof(buf));
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
        IWParsingUtils::print_power_mode(sinfo[NL80211_STA_INFO_LOCAL_PM],buf);
        printf("%s\n",buf);
    }
    if (sinfo[NL80211_STA_INFO_PEER_PM]) {
        printf("\n\tmesh peer PS mode:\t");
        char buf [50];
        IWParsingUtils::print_power_mode(sinfo[NL80211_STA_INFO_PEER_PM],buf);
        printf("%s\n",buf);
    }
    if (sinfo[NL80211_STA_INFO_NONPEER_PM]) {
        printf("\n\tmesh non-peer PS mode:\t");
        char buf [50];
        IWParsingUtils::print_power_mode(sinfo[NL80211_STA_INFO_NONPEER_PM],buf);
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
    return NL_SKIP;

failure:
    //delete arg;
    arg = NULL;
    return NL_SKIP;
}

int IWCallBacks::finishHandler (struct nl_msg *msg, void *arg)
{
    int *ret = (int *) arg;
    *ret = 1;
    return NL_SKIP;
}

IWDumpList::IWDumpList(){}

IWDumpList::~IWDumpList(){}

/*string IWDumpList::getIname (void)
{
    return _sIname;
}*/
int IWDumpList::size (void)
{
    int ret;
    _m.lock();
    ret = _IWStationList.size();
    _m.unlock();   
    return ret;
}

void IWDumpList::put (IWStationDump& iwDump)
{
    _m.lock();
    _IWStationList.emplace_front (iwDump);
    _m.unlock();
}

IWStationDump * IWDumpList::get (int index)
{
    _m.lock();
    IWStationDump *pRes = NULL;
    if (index > _IWStationList.size()) {
        _m.unlock();
        return pRes;
    }
    pRes = new IWStationDump (*next(_IWStationList.begin(), index));
    _m.unlock();
}

list<IWStationDump> * IWDumpList::getCopy (void) 
{
    list<IWStationDump> *pRes = new list<IWStationDump>();
    _m.lock();
    for (auto it = _IWStationList.begin(); it != _IWStationList.end(); it++) {
        pRes->emplace_front (*it);
    }
    _m.unlock();
    return pRes;
}

void IWDumpList::clear (void) 
{
    _m.lock();
    _IWStationList.clear();
    _m.unlock();
}

void IWDumpList::printList (void) 
{
    _m.lock();
    for (auto it = _IWStationList.begin(); it != _IWStationList.end(); it++) {
        it->print();
    }
    _m.unlock();
}

IWDumpManager::IWDumpManager ():
    _sIname("")
{
    _bTerminationRequested = false;
}

int IWDumpManager::init (const char * pszIname, uint64 ui64SleepInteval)
{
    if (pszIname == nullptr) {
        return -1;
    }
    _sIname = pszIname;
    _ui64SleepInterval = ui64SleepInteval;
}

IWDumpManager::~IWDumpManager()
{
    if (_bTerminationRequested == false) {
        _bTerminationRequested = true;
    }
    if (_pT != nullptr) {
        _pT->join();
        delete _pT;
    }
}


void IWDumpManager::start (void) 
{
    _pT =  new thread ([] (atomic_bool& bTerminationRequested, uint64 ui64SleepInterval, string sIname, IWDumpList& IWStationList) 
    {
        while(bTerminationRequested == false){

            struct nl80211_state nlstate;
            struct nl_cb *pCallback,*pSocketCallback;
            struct nl_msg *pMsg;
            char *pBuf;
            signed long long interfaceID =0;
            
            nlstate.nl_sock = nl_socket_alloc();
            if (!nlstate.nl_sock) {
                fprintf (stderr, "Failed to allocate netlink socket. err_code = %d\n", -ENOMEM);
                //return -ENOMEM;
                return;
            }
            
            nl_socket_set_buffer_size (nlstate.nl_sock, 8192, 8192);

            if (genl_connect (nlstate.nl_sock)) {
                fprintf (stderr, "Failed to connect to generic netlink. err_code = %d\n", -ENOLINK);
                nl_socket_free (nlstate.nl_sock);
                //return -ENOLINK;
                return;
            }

            nlstate.nl80211_id = genl_ctrl_resolve (nlstate.nl_sock, "nl80211");
            if (nlstate.nl80211_id < 0) {
                fprintf (stderr, "nl80211 not found. err_code = %d\n", -ENOENT);
                nl_socket_free (nlstate.nl_sock);
                //return -ENOENT
                return;
            }

            interfaceID = if_nametoindex (sIname.c_str());
            if(interfaceID == 0){
                fprintf(stderr, "interface %s not found.\n", sIname);
                return ;
            }
            int end = 0;
            IWStationList.clear();
            pMsg = nlmsg_alloc();
            pCallback = nl_cb_alloc (NL_CB_DEFAULT);
            pSocketCallback = nl_cb_alloc (NL_CB_DEFAULT);
            genlmsg_put (pMsg, 0, 0, nlstate.nl80211_id, 0, NLM_F_DUMP, NL80211_CMD_GET_STATION, 0);
            nla_put_u32 (pMsg, NL80211_ATTR_IFINDEX, interfaceID);
            
            nl_cb_set (pCallback, NL_CB_VALID, NL_CB_CUSTOM, IWCallBacks::stationDumpCallback, &IWStationList);
            nl_cb_set (pCallback, NL_CB_FINISH, NL_CB_CUSTOM, IWCallBacks::finishHandler, &end);


            nl_socket_set_cb (nlstate.nl_sock, pSocketCallback);
            if (nl_send_auto_complete (nlstate.nl_sock, pMsg) < 0) {
                //error
                nl_cb_put(pCallback);
                nlmsg_free(pMsg);
                nl_close(nlstate.nl_sock);
                nl_socket_free(nlstate.nl_sock);
                return;
            }
            while (end == 0) {
                nl_recvmsgs (nlstate.nl_sock, pCallback);
            }
            nlmsg_free(pMsg);
            nl_close(nlstate.nl_sock);
            nl_socket_free(nlstate.nl_sock);

            this_thread::sleep_for (chrono::milliseconds (ui64SleepInterval));
        }
    }, ref(_bTerminationRequested), _ui64SleepInterval, _sIname, ref(_iwDumps));
    //_pT =  new thread (func, ref(_bTerminationRequested), ref(_ui64SleepInterval), _sIname, ref(_iwDumps));
}

void IWDumpManager::requestTermination (void)
{
    _bTerminationRequested = true;
}

list<IWStationDump> * IWDumpManager::getDumps (void)
{  
    return _iwDumps.getCopy();
}

const char * IWDumpManager::getIname (void)
{
    return _sIname.c_str();
}

void IWDumpManager::printDumps (void)
{
    _iwDumps.printList();
}

#endif