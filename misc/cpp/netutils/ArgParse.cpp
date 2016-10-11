#include "ArgParse.h"

#include <stdio.h>
#include <stdlib.h>

#include "InetAddr.h"
#include "NLFLib.h"
#include "StrClass.h"

using namespace IHMC_MISC;
using namespace NOMADSUtil;

Arguments ArgParse::parseSenderArgs (int argc, const char *argv[])
{
    return parse (argc, argv, true);
}

Arguments ArgParse::parseReceiverArgs (int argc, const char *argv[])
{
    return parse (argc, argv, false);
}

Arguments ArgParse::parse (int argc, const char *argv[], bool bSender)
{
    // Assign default values and read mandatory input values
    const char *pszPacketLength = "1024";
    const char *pszTransmitInterval = "0";
    
    const char *pszSourceIP = NULL;
    const char *pszSourcePort = NULL;
    const char *pszDestIP = NULL;
    const char *pszDestPort = NULL;
    if (bSender) {
        pszDestIP = argv[argc - 2];
        pszDestPort = argv[argc - 1];
        argc -= 2;
    }
    else {
        pszSourcePort = argv[argc - 1];
        argc -= 1;
    }

    bool bUseSeqNum = false;
    bool bUseRelTime = false;
    bool bPrintData = false;

    // Parse input values
    for (int i = 1; i < argc; i++) {
        const String arg (argv[i]);
        if (arg == "-useSeqNum") {
            bUseSeqNum = true;
        }
        else if (arg == "-useRelTime") {
            bUseRelTime = true;
        }
        else if (0 == strcmp (argv[i], "-printData")) {
            bPrintData = true;
        }
        else if (bSender && arg == "-len") {
            pszPacketLength = argv[++i];
        }
        else if (bSender && arg == "-int") {
            pszTransmitInterval = argv[++i];
        }
        else if (bSender && arg == "-srcIP") {
            pszSourceIP = argv[++i];
        }
        else if (bSender && arg == "-srcPort") {
            pszSourcePort = argv[++i];
        }
        else if (!bSender && arg == "-listenIP") {
            pszSourceIP = argv[++i];
        }
        else {
            exit (-2);
        }
    }

    const uint32 ui32DestIP = bSender ? InetAddr (pszDestIP).getIPAddress () : 0U;
    const uint16 ui16DestPort = bSender  ? (uint16) atoi (pszDestPort) : 0;
    const uint32 ui32TransmitInterval = atoui32 (pszTransmitInterval);
    const uint16 ui16PacketLength = (uint16) atoi (pszPacketLength);
    uint16 ui16SrcPort = 0;
    if (pszSourcePort) {
        ui16SrcPort = (uint16) atoi (pszSourcePort);
    }

    printf ("Packet length set to %u\n", ui16PacketLength);
    printf ("Transmit interval set to %u\n", ui32TransmitInterval);
    if (pszSourceIP) {
        printf ("SourceIPAddr set to %s\n", pszSourceIP);
    }
    if (pszSourcePort) {
        printf ("SourcePort set to %u\n", ui16SrcPort);
    }
    if (bUseSeqNum) {
        printf ("Using sequence numbered packets\n");
    }
    if (bUseRelTime) {
        printf ("Using relative timestamped packets\n");
    }

    uint32 ui32SrcIP = 0U;
    if (pszSourceIP) {
        ui32SrcIP = InetAddr (pszSourceIP).getIPAddress();
    }

    return Arguments (bPrintData, bUseSeqNum, bUseRelTime, ui16PacketLength,
                      ui32TransmitInterval, ui32SrcIP, ui16SrcPort, ui32DestIP,
                      ui16DestPort);
}

//-----------------------------------------------------------------------------
// Arguments
//-----------------------------------------------------------------------------

Arguments::Arguments (bool bPrntData, bool bUseSeqNum, bool bUseRelTime,
                      uint16 ui16PktLen, uint32 ui32TransmInterval,
                      uint32 ui32SrcAddr, uint16 ui16SrcPort,
                      uint32 ui32DstAddr, uint16 ui16DstPort)
    : _bPrntData (bPrntData),
      _bUseSeqNum (bUseSeqNum),
      _bUseRelTime (bUseRelTime),
      _ui16PktLen (ui16PktLen),
      _ui16SrcPort (ui16SrcPort),
      _ui16DstPort (ui16DstPort),
      _ui32TransmInterval (ui32TransmInterval),
      _ui32SrcAddr (ui32SrcAddr),
      _ui32DstAddr (ui32DstAddr)
{
}

Arguments::Arguments (const Arguments &rhsArgs)
    : _bPrntData (rhsArgs._bPrntData),
      _bUseSeqNum (rhsArgs._bUseSeqNum),
      _bUseRelTime (rhsArgs._bUseRelTime),
      _ui16PktLen (rhsArgs._ui16PktLen),
      _ui16SrcPort (rhsArgs._ui16SrcPort),
      _ui16DstPort (rhsArgs._ui16DstPort),
      _ui32TransmInterval (rhsArgs._ui32TransmInterval),
      _ui32SrcAddr (rhsArgs._ui32SrcAddr),
      _ui32DstAddr (rhsArgs._ui32DstAddr)
{
}

Arguments::~Arguments (void)
{
}

