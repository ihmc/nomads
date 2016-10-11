#include <stdio.h>
#include <stdlib.h>

#include "FTypes.h"
#include "InetAddr.h"
#include "NLFLib.h"
#include "MulticastUDPDatagramSocket.h"

using namespace NOMADSUtil;

#if defined (UNIX)
    #define stricmp strcasecmp
#elif defined (WIN32)
    #define stricmp _stricmp
#endif

int main (int argc, const char *argv[])
{
    char szParams[] = "[-h|-help|-?] [-listenIP <ListenIPAddr>] <MCastAddr> <ListenPort>";

    if (argc < 3) {
        fprintf (stderr, "usage: %s %s\n",
                 argv[0], szParams);
        return -1;
    }

    const char *pszListenIP = NULL;
    const char *pszMCastAddr = argv[argc-2];
    const char *pszListenPort = argv[argc-1];

    int i = 1;
    while (i < argc) {
        if ((0 == stricmp (argv[i], "-h")) || (0 == stricmp (argv[i], "-help")) || (0 == stricmp (argv[i], "-?"))) {
            printf ("usage: %s %s\n", argv[0], szParams);
            return 0;
        }
        else if (0 == stricmp (argv[i], "-listenIP")) {
            i++;
            if (i < argc) {
                pszListenIP = argv[i];
            }
        }
        i++;
    }

    MulticastUDPDatagramSocket udps;

    uint32 ui32ListenIP = 0;
    if (pszListenIP) {
        ui32ListenIP = InetAddr(pszListenIP).getIPAddress();
    }
    uint16 ui16ListenPort = (uint16) atoi (pszListenPort);

    uint32 ui32MCastAddr = InetAddr(pszMCastAddr).getIPAddress();

    printf ("\nListening on Interface = %s\n", InetAddr(ui32ListenIP).getIPAsString());
    printf ("MCast Addr = %s:%d\n", InetAddr(ui32MCastAddr).getIPAsString(), (int) ui16ListenPort);

    int rc;
    if (0 != (rc = udps.init (ui16ListenPort, ui32ListenIP))) {
        fprintf (stderr, "error: could not bind UDP socket to <%s>:<%d>; rc = %d\n",
                 InetAddr(ui32ListenIP).getIPAsString(), (int) ui16ListenPort, rc);
        return -2;
    }
    if (0 != (rc = udps.joinGroup (ui32MCastAddr, ui32ListenIP))) {
        fprintf (stderr, "error: could not join MCast group %s; rc = %d\n",
                 InetAddr(ui32MCastAddr).getIPAsString(), rc);
        return -3;
    }

    bool bFirstPacket = true;
    int64 i64StartTime = 0;
    int64 i64IntervalStartTime = 0;
    uint32 ui32LastPacketSeqNum = 0;
    uint32 ui32BytesReceived = 0;
    uint32 ui32MissedPackets = 0;
    uint32 ui32PacketsReceived = 0;
    uint32 ui32BytesReceivedInInterval = 0;
    uint32 ui32PacketsMissedInInterval = 0;
    uint32 ui32PacketsReceivedInInterval = 0;

    while (true) {
        uint8 ui8Buf[65535];
        InetAddr senderAddr;
        if ((rc = udps.receive (ui8Buf, sizeof (ui8Buf), &senderAddr)) < 0) {
            fprintf (stderr, "error: receive() on UDP socket failed; rc = %d\n", rc);
            return -3;
        }
        ui32BytesReceived += rc;
        ui32BytesReceivedInInterval += rc;
        ui32PacketsReceived++;
        ui32PacketsReceivedInInterval++;
        uint32 ui32SeqNum = EndianHelper::ntohl (*((uint32*)(ui8Buf)));
        if (bFirstPacket) {
            i64IntervalStartTime = i64StartTime = getTimeInMilliseconds();
            ui32LastPacketSeqNum = ui32SeqNum;
            bFirstPacket = false;
        }
        else {
            if (ui32SeqNum > (ui32LastPacketSeqNum + 1)) {
                ui32MissedPackets += ((ui32LastPacketSeqNum + 1) - ui32SeqNum);
                ui32PacketsMissedInInterval += ((ui32LastPacketSeqNum + 1) - ui32SeqNum);
                ui32LastPacketSeqNum = ui32SeqNum;
            }
            else if (ui32SeqNum < ui32LastPacketSeqNum) {
                printf ("WARNING: Out of order packet received\n");
            }
            else if (ui32SeqNum == ui32LastPacketSeqNum) {
                printf ("WARNING: Duplicate packet received\n");
            }
            else {
                ui32LastPacketSeqNum = ui32SeqNum;
            }
        }
        int64 i64CurrTime = getTimeInMilliseconds();
        uint32 ui32IntervalTime = (uint32)(i64CurrTime - i64IntervalStartTime);
        if (ui32IntervalTime >= 5000) {
            float fbPS = (float) (ui32BytesReceivedInInterval / (ui32IntervalTime / 5000.0)) * 8;
            float fBPS = fbPS / 8;
            printf ("Interval: %u \tMissed: %u packets \tReceived: %u packets \t%.1f bps \t%.1f Kbps \t%.1f Mbps \t%.lf Bps \t%.1f KBps \t%.1f MBps from %s\n",
                    ui32IntervalTime, ui32PacketsMissedInInterval, ui32PacketsReceivedInInterval, fbPS, fbPS / 1024, fbPS / (1024 * 1024), fBPS, fBPS / 1024, fBPS / (1024 * 1024), udps.getLocalAddr ().getIPAsString ());
            i64IntervalStartTime = i64CurrTime;
            ui32BytesReceivedInInterval = 0;
            ui32PacketsMissedInInterval = 0;
            ui32PacketsReceivedInInterval = 0;
        }
    }

    return 0;
}
