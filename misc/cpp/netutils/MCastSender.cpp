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
    const char szParams[] = "[-h|-help|-?] [-len <packet length>] [-int <transmit interval>] [-localIP <LocalIPAddr>] [-localPort <SourcePort] [-ttl <MCastTTL>] <MCastAddr> <DestPort>";

    if (argc < 3) {
        fprintf (stderr, "usage: %s %s\n",
                 argv[0], szParams);
        return -1;
    }

    const char *pszPacketLength = "1024";
    const char *pszTransmitInterval = "1000";
    const char *pszSourceIP = NULL;
    const char *pszSourcePort = NULL;
    const char *pszMCastAddr = argv[argc-2];
    const char *pszDestPort = argv[argc-1];
    const char *pszMCastTTL = "10";

    int i = 1;
    while (i < argc) {
        if ((0 == stricmp (argv[i], "-h")) || (0 == stricmp (argv[i], "-help")) || (0 == stricmp (argv[i], "-?"))) {
            printf ("usage: %s %s\n", argv[0], szParams);
            return 0;
        }
        else if (0 == stricmp (argv[i], "-len")) {
            i++;
            if (i < argc) {
                pszPacketLength = argv[i];
            }
        }
        else if (0 == stricmp (argv[i], "-int")) {
            i++;
            if (i < argc) {
                pszTransmitInterval = argv[i];
            }
        }
        else if (0 == stricmp (argv[i], "-localIP")) {
            i++;
            if (i < argc) {
                pszSourceIP = argv[i];
            }
        }
        else if (0 == stricmp (argv[i], "-localPort")) {
            i++;
            if (i < argc) {
                pszSourcePort = argv[i];
            }
        }
        else if (0 == stricmp (argv[i], "-ttl")) {
            i++;
            if (i < argc) {
                pszMCastTTL = argv[i];
            }
        }
        i++;
    }

    uint32 ui32SourceIP = 0;
    if (pszSourceIP) {
        ui32SourceIP = InetAddr(pszSourceIP).getIPAddress();
    }
    uint16 ui16SourcePort = 0;
    if (pszSourcePort) {
        ui16SourcePort = (uint16) atoi (pszSourcePort);
    }
    uint32 ui32MCastAddr = InetAddr(pszMCastAddr).getIPAddress();
    uint16 ui16DestPort = (uint16) atoi (pszDestPort);
    uint8 ui8MCastTTL = (uint8) atoi (pszMCastTTL);
    uint16 ui16PacketLength = (uint16) atoi (pszPacketLength);
    uint32 ui32TransmitInterval = (uint32) atoi (pszTransmitInterval);

    MulticastUDPDatagramSocket udps;

    int rc;
    if (0 != (rc = udps.init (ui16SourcePort, ui32SourceIP))) {
        fprintf (stderr, "error: could not bind UDP socket to <%s>:<%d>; rc = %d\n",
                 InetAddr(ui32SourceIP).getIPAsString(), (int) ui16SourcePort, rc);
        return -2;
    }

    if (ui16SourcePort == 0) {
        ui16SourcePort = udps.getLocalPort();
    }

    printf ("\nLocal Addr = %s:%d\n", InetAddr(ui32SourceIP).getIPAsString(), (int) ui16SourcePort);
    printf ("MCast Addr = %s:%d\n", InetAddr(ui32MCastAddr).getIPAsString(), (int) ui16DestPort);
    printf ("MCast TTL = %d\n", (int) ui8MCastTTL);
    printf ("Packet Interval = %lu ms\n", ui32TransmitInterval);
    printf ("Packet Length = %d bytes\n\n", (int) ui16PacketLength);

    if (0 != (rc = udps.joinGroup (ui32MCastAddr, ui32SourceIP))) {
        fprintf (stderr, "error: could not join multicast group <%s>; rc = %d\n",
                 InetAddr(ui32MCastAddr).getIPAsString(), rc);
        return -3;
    }

    if (0 != (rc = udps.setTTL (ui8MCastTTL))) {
        fprintf (stderr, "error: could not set the TTL to <%d>; rc = %d\n", ui8MCastTTL, rc);
        return -4;
    }

    uint8 *pui8Buf = (uint8*) malloc (ui16PacketLength);
    for (uint16 ui16 = 0; ui16 < ui16PacketLength; ui16++) {
        pui8Buf[ui16] = (uint8) rand();
    }

    uint32 ui32SeqNum = 0;
    int64 i64IntervalStartTime = 0;
    uint32 ui32PacketsSentInInterval = 0;
    while (true) {
        if (i64IntervalStartTime == 0) {
            i64IntervalStartTime = getTimeInMilliseconds();
        }
		*((uint32*)(pui8Buf)) = EndianHelper::htonl (ui32SeqNum++);
        if ((rc = udps.sendTo (ui32MCastAddr, ui16DestPort, pui8Buf, ui16PacketLength)) != ui16PacketLength) {
            fprintf (stderr, "error: MulticastUDPDatagramSocket::sendTo() failed with rc = %d\n", rc);
            free (pui8Buf);
            return -3;
        }
        ui32PacketsSentInInterval++;
        int64 i64CurrentTime = getTimeInMilliseconds();
        uint32 ui32IntervalTime = (uint32) (i64CurrentTime - i64IntervalStartTime);
        if (ui32IntervalTime >= 5000) {
            printf ("Interval: %u \tSent %u packets\n", ui32IntervalTime, ui32PacketsSentInInterval);
            ui32PacketsSentInInterval = 0;
            i64IntervalStartTime = i64CurrentTime;
        }
        if (ui32TransmitInterval > 0) {
            sleepForMilliseconds (ui32TransmitInterval);
        }
    }

    free (pui8Buf);

    return 0;
}
