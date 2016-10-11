#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "InetAddr.h"
#include "NLFLib.h"
#include "UDPDatagramSocket.h"

#include "NetMetrics.h"
#include "PacketFactory.h"

using namespace IHMC_MISC;
using namespace NOMADSUtil;

int main (int argc, const char *argv[])
{
    if (argc < 3) {
        fprintf (stderr, "usage: %s [-len <packet length>] [-int <transmit interval>] [-srcIP <SourceIPAddr>] [-srcPort <SourcePort>] [-useSeqNum] <DestIPAddr> <DestPort>\n",
                 argv[0]);
        return -1;
    }
    // Assign default values and read mandatory input values
    const char *pszPacketLength = "1024";
    const char *pszTransmitInterval = "1000";
    const char *pszSourceIP = NULL;
    const char *pszSourcePort = NULL;
    const char *pszDestIP = argv[argc-2];
    const char *pszDestPort = argv[argc-1];
    bool bUseSeqNum = false;
    bool bUseRelTime = false;

    // Parse input values
    for (int i = 1; i < argc; i++) {
        if (0 == strcmp (argv[i], "-len")) {
            pszPacketLength = argv[++i];
        }
        if (0 == strcmp (argv[i], "-int")) {
            pszTransmitInterval = argv[++i];
        }
        if (0 == strcmp (argv[i], "-srcIP")) {
            pszSourceIP = argv[++i];
        }
        if (0 == strcmp (argv[i], "-srcPort")) {
            pszSourcePort = argv[++i];
        }
        if (0 == strcmp(argv[i], "-useSeqNum")) {
            bUseSeqNum = true;
        }
        if (0 == strcmp(argv[i], "-useRelTime")) {
            bUseRelTime = true;
        }
    }

    const uint32 ui32DestIP = InetAddr(pszDestIP).getIPAddress();
    const uint16 ui16DestPort = (uint16) atoi (pszDestPort);
    const uint32 ui32TransmitInterval = (uint32) atoi (pszTransmitInterval);
    const uint16 ui16PacketLength = (uint16) atoi (pszPacketLength);

    printf ("Packet length set to %u\n", ui16PacketLength);
    printf ("Transmit interval set to %u\n", ui32TransmitInterval);
    if (pszSourceIP) {
        printf ("SourceIPAddr set to %s\n", pszSourceIP);
    }
    if (pszSourcePort) {
        printf ("SourcePort set to %u\n", ui16DestPort);
    }
    if (bUseSeqNum) {
        printf ("Using sequence numbered packets\n");
    }
    if (bUseRelTime) {
        printf ("Using relative timestamped packets\n");
    }

    PacketFactory pkt (bUseSeqNum, bUseRelTime);
    UDPDatagramSocket udps;

    uint32 ui32SourceIP = 0;
    if (pszSourceIP) {
        ui32SourceIP = InetAddr(pszSourceIP).getIPAddress();
    }
    uint16 ui16SourcePort = 0;
    if (pszSourcePort) {
        ui16SourcePort = (uint16) atoi (pszSourcePort);
    }

    int rc;
    if (0 != (rc = udps.init (ui16SourcePort, ui32SourceIP))) {
        fprintf (stderr, "error: could not bind UDP socket to <%s>:<%d>; rc = %d\n",
                 InetAddr(ui32SourceIP).getIPAsString(), (int) ui16SourcePort, rc);
        return -2;
    }
    // Send
    Rate rate (getTimeInMilliseconds());
    uint8 *pui8Buf = (uint8*) malloc (ui16PacketLength);
    if ((rc = pkt.init (pui8Buf, ui16PacketLength)) < 0) {
        fprintf (stderr, "error: could not initialize packet of length <%d>; rc = %d\n",
                 ui16PacketLength, rc);
        return -3;
    }

    for (unsigned int i; true; pkt.updateHeader(), i++) {
        if ((rc = udps.sendTo (ui32DestIP, ui16DestPort, pui8Buf, ui16PacketLength)) != ui16PacketLength) {
            fprintf (stderr, "error: UDPDatagramSocket::sendTo() failed with rc = %d\n", rc);
            free (pui8Buf);
            return -3;
        }
        rate.add (getTimeInMilliseconds(), ui16PacketLength);
        sleepForMilliseconds (ui32TransmitInterval);
        if ((i % 1000) == 0) {
            fprintf (stdout, "rate: %.3f Kbps\n", rate.get(Rate::Kbps));
        }
    }

    free (pui8Buf);

    return 0;
}
