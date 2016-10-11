#include <stdio.h>
#include <stdlib.h>

#include "ArgParse.h"
#include "InetAddr.h"
#include "NLFLib.h"
#include "TCPSocket.h"

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

    const Arguments args (ArgParse::parseSenderArgs (argc, argv));
    PacketFactory pkt (args._bUseSeqNum, args._bUseRelTime);
    TCPSocket tcps;

    int rc;
    InetAddr inet (args._ui32DstAddr);
    const String ipAddr (inet.getIPAsString());
    if (0 != (rc = tcps.connect (ipAddr, args._ui16DstPort))) {
        fprintf (stderr, "error: could connect bind TCP socket to <%s>:<%d>; rc = %d\n",
                 ipAddr.c_str(), (int) args._ui16DstPort, rc);
        return -2;
    }
    tcps.bufferingMode (false);  // disable Naggle

    // Send
    Rate rate (getTimeInMilliseconds());
    uint8 *pui8Buf = static_cast<uint8*>(malloc (args._ui16PktLen));
    if ((rc = pkt.init (pui8Buf, args._ui16PktLen)) < 0) {
        fprintf (stderr, "error: could not initialize packet of length <%d>; rc = %d\n",
                 args._ui16PktLen, rc);
        return -3;
    }

    for (unsigned int i = 0; true; pkt.updateHeader(), i++) {
        if ((rc = tcps.send (pui8Buf, args._ui16PktLen)) != args._ui16PktLen) {
            fprintf (stderr, "error: TCPSocket::send() failed with rc = %d\n", rc);
            free (pui8Buf);
            return -3;
        }
        rate.add (getTimeInMilliseconds(), args._ui16PktLen);
        sleepForMilliseconds (args._ui32TransmInterval);
        if ((i % 1000) == 0) {
            fprintf (stdout, "rate: %.3f Kbps\n", rate.get (Rate::Kbps));
        }
    }

    free (pui8Buf);

    return 0;
}

