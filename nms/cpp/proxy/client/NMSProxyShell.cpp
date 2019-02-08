/*
 * NMSProxyShell.cpp
 *
 * This file is part of the IHMC Network Message Service Library
 * Copyright (c) 1993-2014 IHMC.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 (GPLv3) as published by the Free Software Foundation.
 *
 * U.S. Government agencies and organizations may redistribute
 * and/or modify this program under terms equivalent to
 * "Government Purpose Rights" as defined by DFARS
 * 252.227-7014(a)(12) (February 2014).
 *
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on March 1, 2015, 7:40 PM
 */

#include "NMSProxyShell.h"

#include "DArray2.h"
#include "FileReader.h"
#include "InetAddr.h"
#include "LineOrientedReader.h"
#include "Logger.h"
#include "NLFLib.h"
#include "StringTokenizer.h"
#include "NetworkMessageService.h"
#include "NMSProperties.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

using namespace NOMADSUtil;

#if defined (WIN32)
    #define snprintf _snprintf
    #define stat _stat32
    #define stricmp _stricmp
    #define PATH_MAX MAX_PATH
#elif defined (UNIX)
    #define stricmp strcasecmp
    #include <limits.h>
#endif

namespace NOMADSUtil
{
    enum CastType
    {
        GROUPCAST,
        TRANSMIT,
        TRANSMIT_RELIABLE
    };

    bool isNumber (const char *pszString)
    {
        if (pszString == NULL) {
            return false;
        }
        for (unsigned int i = 0; pszString[i] != '\0'; i++) {
            if (!isdigit (pszString[i])) {
                return false;
            }
        }
        return true;
    }

    void handleGenericaCast (NetworkMessageServiceProxy &nmsProxy, CastType type, const void *pToken, const char *pszCmdLine, bool bSecure = false)
    {
        StringTokenizer st (pszCmdLine);
        st.getNextToken();  // This is the command itself - discard
        const char *pszMsgType = st.getNextToken();
        const InetAddr dstAddr (st.getNextToken());
        const char *pszMsgLen = st.getNextToken();
        DArray<char *> interfaces;
        const char *pszInterfaces = st.getNextToken();
        if (pszInterfaces != NULL) {
            StringTokenizer ifaces (pszInterfaces, ',', ',');
            unsigned int i = 0;
            for (String iface; (iface = ifaces.getNextToken()).length() > 0;) {
                interfaces[i++] = iface.r_str();
            }
        }
        uint8 ui8MsgType = (uint8) atoui32 (pszMsgType);
        const char **ppszOutgoingInterfaces = NULL;
        if (interfaces.size() > 0) {
            interfaces[interfaces.size()] = NULL;
            ppszOutgoingInterfaces = (const char **) interfaces.getData();
        }
        uint32 ui32DestAddr = (uint32) dstAddr.getIPAddress();
        uint16 ui16MsgId = 0;
        uint8 ui8HopCount = 0;
        uint8 ui8TTL = 1;
        uint16 ui16DelayTolerance = 0;
        const char *pszMsgMetaData = "shell";
        uint16 ui16MsgMetaDataLen = strlen (pszMsgMetaData);
        const uint16 ui16MsgLen = (uint16) atoui32 (pszMsgLen);
        char *pMsg = (char *) calloc (ui16MsgLen, 1);
        for (unsigned int i = 0; i < ui16MsgLen; i++) {
            pMsg[i] = (i % 127); // loop through ASCII characters
        }
        bool bExpedited = false;
        const char *pszHints = NULL;

        int rc = 0;
        String methodName;
        switch (type) {
        case GROUPCAST:
            methodName = "broadcastMessage";
            rc = nmsProxy.broadcastMessage (ui8MsgType, ppszOutgoingInterfaces, ui32DestAddr, ui16MsgId,
                                            ui8HopCount, ui8TTL, ui16DelayTolerance, pszMsgMetaData, ui16MsgMetaDataLen,
                                            pMsg, ui16MsgLen, bExpedited, pszHints);
            break;

        case TRANSMIT:
            methodName = "transmitMessage";
            rc = nmsProxy.transmitMessage (ui8MsgType, ppszOutgoingInterfaces, ui32DestAddr, ui16MsgId,
                                           ui8HopCount, ui8TTL, ui16DelayTolerance, pszMsgMetaData, ui16MsgMetaDataLen,
										   pMsg, ui16MsgLen, pszHints);
            break;

        case TRANSMIT_RELIABLE:
            methodName = "transmitReliableMessage";
            rc = nmsProxy.transmitReliableMessage (ui8MsgType, ppszOutgoingInterfaces, ui32DestAddr, ui16MsgId,
                                                   ui8HopCount, ui8TTL, ui16DelayTolerance, pszMsgMetaData, ui16MsgMetaDataLen,
												   pMsg, ui16MsgLen, pszHints);
            break;

        default:
            rc = -1;
            assert(false);
        }
        printf("<%s> returned %d.\n", pszCmdLine, rc);
    }
}

//This is the method where the sessionKey has to be inserted
NMSProxyShell::NMSProxyShell()
    : NetworkMessageServiceListener (0), _nmsProxy (0, true)
{
}

NMSProxyShell::~NMSProxyShell()
{
}

int NMSProxyShell::init (const char *pszServerHost, uint16 ui16ServerPort)
{
    int rc = _nmsProxy.init (pszServerHost, ui16ServerPort);
    if (0 != rc) {
        return -1;
    }
    _nmsProxy.start();
    return 0;
}

int NMSProxyShell::processCmd (const void *pToken, char *pszCmdLine)
{
    String line (pszCmdLine);
    line.trim();
    StringTokenizer cmdTokenizer (line, ' ', ' ');
    String cmd (cmdTokenizer.getNextToken());
    cmd.trim();
    cmd.convertToLowerCase();
    if (cmd.length() <= 0) {
        // Should not happen, but return 0 anyways
        return 0;
    }
    else if (cmd == "load") {
        StringTokenizer st (pszCmdLine);
        st.getNextToken();  // This is the command itself - discard
        const char *pszBatchFileName = st.getNextToken();
        FILE *pBatchFile = fopen (pszBatchFileName, "r");
        if (pBatchFile == NULL) {
            print (pToken, "can't load file <%s>\n", pszBatchFileName);
            return -2;
        }
        FileReader fr (pBatchFile, true);
        LineOrientedReader lr (&fr, false);
        char buf[1024];
        int rc = 0;
        while ((rc = lr.readLine (buf, 1024)) >= 0) {
            String batchCmd (buf, rc);
            if (batchCmd.startsWith ("#") == 0) {
                // Not a comment
                if ((rc = processCmd (pToken, batchCmd)) != 0) {
                    return rc;
                }
            }
        }
    }
    else if ((cmd == "ping") || (cmd == "test")) {
        handlePing (pToken, line);
    }
    else if ((cmd == "broadcast") || (cmd == "bcast")) {
        handleBroadcast (pToken, line);
    }
    else if ((cmd == "transmit") || (cmd == "tr")) {
        handleTransmit (pToken, line);
    }
    else if ((cmd == "transmit-reliable") || (cmd == "tr-rel")) {
        handleTransmitReliable (pToken, line);
    }
    else if ((cmd == "set-retransmission-timeout") || (cmd == "set-ret-tout")) {
        handleSetRetransmissionTimeout (pToken, line);
    }
    else if ((cmd == "set-primary-interface") || (cmd == "set-prim-iface")) {
        handleSetPrimaryInterface (pToken, line);
    }
    else if ((cmd == "get-minumium-mtu") || (cmd == "get-min-mtu")) {
        handleGetMinimumMTU (pToken, line);
    }
    else if ((cmd == "get-interfaces") || (cmd == "get-ifaces")) {
        handleGetInterfaces (pToken, line);
    }
    else if ((cmd == "get-propagation-mode") || (cmd == "get-prop-mode") || (cmd == "get-mode")) {
        handleGetPropagationMode (pToken, line);
    }
    else if ((cmd == "get-delivery-queue-size") || (cmd == "get-del-q-size")) {
        handleGetDeliveryQueueSize (pToken, line);
    }
    else if (cmd == "get-receive-rate") {
        handleGetReceiveRate (pToken, line);
    }
    else if ((cmd == "get-transmission-queue-size") || (cmd == "get-tr-q-size")) {
        handleGetTransmissionQueueSize (pToken, line);
    }
    else if ((cmd == "get-rescaled-transmission-queue-size") || (cmd == "get-resc-tr-queue-size")) {
        handleGetRascaledTransmissionQueueSize (pToken, line);
    }
    else if ((cmd == "get-transmission-queue-max-size") || (cmd == "get-tr-q-max-size")) {
        hanldeGetTransmissionQueueMaxSize (pToken, line);
    }
    else if ((cmd == "get-transmit-rate-limit") || (cmd == "get-tr-rate-limit")) {
        handleGetTransmitRateLimit (pToken, line);
    }
    else if ((cmd == "set-transmission-queue-max-size") || (cmd == "set-tr-q-max-size")) {
        handleSetTransmissionQueueMaxSize (pToken, line);
    }
    else if ((cmd == "set-transmit-rate-limit") || (cmd == "set-tr-rate-limit")) {
        handleSetTransmitRateLimit (pToken, line);
    }
    else if (cmd == "get-link-capacity") {
        handleGetLinkCapacity (pToken, line);
    }
    else if (cmd == "set-link-capacity") {
        handleSetLinkCapacity (pToken, line);
    }
    else if ((cmd == "get-neighbor-queue-length") || (cmd == "get-neighbor-q-length")) {
        handleGetNeighborQueueLength (pToken, line);
    }
    else if (cmd == "clear-to-send") {
        handleClearToSend (pToken, line);
    }
    else if (cmd == "register") {
        handleRegisterListener (pToken, line);
    }
    else if ((cmd == "help")) {
        handleHelp (pToken, line);
    }
    return 0;
}

int NMSProxyShell::messageArrived (const char *pszIncomingInterface, uint32 ui32SourceIPAddress, uint8 ui8MsgType,
                                   uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL, bool bUnicast,
                                   const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                   const void *pMsg, uint16 ui16MsgLen, int64 i64Timestamp, uint64 ui64MsgCount, uint64 ui64UnicastMsgCount)
{
    InetAddr addr (ui32SourceIPAddress);
    String metadata ((const char *)pMsgMetaData, ui16MsgMetaDataLen);
    printf ("Source %s.\tIncoming interface: %s.\n", addr.getIPAsString(), pszIncomingInterface);
    printf ("Metadata %s.\n", metadata.c_str());
    printf ("Data len %u.\n", ui16MsgLen);
    printf ("Hop count: %d\tTTL %d.\n", (int) ui8HopCount, int (ui8TTL));
    printf ("%s message count: %llu.\n", bUnicast ? "Unicast" : "Multicast", ui64MsgCount);
    return 0;
}

void NMSProxyShell::handleBroadcast (const void *pToken, const char *pszCmdLine)
{
    handleGenericaCast (_nmsProxy, GROUPCAST, pToken, pszCmdLine);
}

void NMSProxyShell::handleHelp (const void *pToken, const char *pszCmdLine)
{
}

void NMSProxyShell::handleTransmit (const void *pToken, const char *pszCmdLine)
{
    handleGenericaCast(_nmsProxy, TRANSMIT, pToken, pszCmdLine);
}

void NMSProxyShell::handleTransmitReliable (const void *pToken, const char *pszCmdLine)
{
    handleGenericaCast(_nmsProxy, TRANSMIT_RELIABLE, pToken, pszCmdLine);
}

void NMSProxyShell::handleSetRetransmissionTimeout (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszTimeout = st.getNextToken();
    const uint32 ui32Timeout = atoui32 (pszTimeout);

    int rc = _nmsProxy.setRetransmissionTimeout (ui32Timeout);
    printf ("<%s> returned %d.\n", pszCmdLine, rc);
}

void NMSProxyShell::handleSetPrimaryInterface (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszPrimaryInterface = st.getNextToken();

    int rc = _nmsProxy.setPrimaryInterface (pszPrimaryInterface);
    printf ("<%s> returned %d.\n", pszCmdLine, rc);
}

void NMSProxyShell::handleGetMinimumMTU (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
}

void NMSProxyShell::handleGetInterfaces (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszDestination = st.getNextToken();

    char **ppszInterfaces = NULL;
    if (pszDestination == NULL) {
        ppszInterfaces = _nmsProxy.getActiveNICsInfoAsString();
    }
    else if (isNumber(pszDestination)) {
        uint32 ui32SenderRemoteIPv4Addr = atoui32 (pszDestination);
        ppszInterfaces = _nmsProxy.getActiveNICsInfoAsStringForDestinationAddr (ui32SenderRemoteIPv4Addr);
    }
    else {
        ppszInterfaces = _nmsProxy.getActiveNICsInfoAsStringForDestinationAddr (pszDestination);
    }

    String ifaces;
    if (ppszInterfaces != NULL) {
        for (unsigned int i = 0; ppszInterfaces[i] != NULL; i++) {
            if (i > 0) {
                ifaces += ", ";
            }
            ifaces += ppszInterfaces[i];
        }
    }

    printf ("<%s> returned <%s>.\n", pszCmdLine, ifaces.c_str());
}

void NMSProxyShell::handleGetPropagationMode (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard

    printf ("<%s> returned %d.\n", pszCmdLine, _nmsProxy.getPropagationMode());
}

void NMSProxyShell::handleGetDeliveryQueueSize (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard

    printf ("<%s> returned %u.\n", pszCmdLine, _nmsProxy.getDeliveryQueueSize());
}


void NMSProxyShell::handleGetReceiveRate (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszAddr = st.getNextToken();

    printf ("<%s> returned %lld.\n", pszCmdLine, _nmsProxy.getReceiveRate(pszAddr));
}

void NMSProxyShell::handleGetTransmissionQueueSize (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszOutgoingInterface = st.getNextToken();

    printf ("<%s> returned %u.\n", pszCmdLine, _nmsProxy.getTransmissionQueueSize(pszOutgoingInterface));
}

void NMSProxyShell::handleGetRascaledTransmissionQueueSize (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszOutgoingInterface = st.getNextToken();

    printf ("<%s> returned %d.\n", pszCmdLine, (int) _nmsProxy.getRescaledTransmissionQueueSize (pszOutgoingInterface));
}

void NMSProxyShell::hanldeGetTransmissionQueueMaxSize (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszOutgoingInterface = st.getNextToken();

    printf ("<%s> returned %u.\n", pszCmdLine, _nmsProxy.getTransmissionQueueMaxSize (pszOutgoingInterface));
}

void NMSProxyShell::handleGetTransmitRateLimit (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszInterface = st.getNextToken();

    printf ("<%s> returned %u.\n", pszCmdLine, _nmsProxy.getTransmitRateLimit(pszInterface));
}

void NMSProxyShell::handleSetTransmissionQueueMaxSize (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszOutgoingInterface = st.getNextToken();
    const char *pszQueueMaxSize = st.getNextToken();
    uint32 ui32QueueMaxSize = atoui32 (pszQueueMaxSize);

    printf ("<%s> returned %d.\n", pszCmdLine, _nmsProxy.setTransmissionQueueMaxSize(pszOutgoingInterface, ui32QueueMaxSize));
}

void NMSProxyShell::handleSetTransmitRateLimit (const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st (pszCmdLine);
    st.getNextToken();  // This is the command itself - discard

    DArray2<String> tokens (3U);
    unsigned int i = 0;
    for (const char *pszToken; (pszToken = st.getNextToken()) != NULL; i++) {
        tokens[i] = pszToken;
    }
    if (tokens.size() == 0) {
        printf ("<%s> returned an error: at least the rate limit must be set\n", pszCmdLine);
    }

    const char *pszInterface = (tokens.getHighestIndex() > 1 ? tokens[tokens.getHighestIndex()-2].c_str() : NULL);
    const char *pszDestinationAddress = (tokens.getHighestIndex() > 0 ? tokens[tokens.getHighestIndex()-1].c_str() : NULL);
    const char *pszRateLimit = tokens[tokens.getHighestIndex()].c_str();
    uint32 ui32RateLimit = atoui32(pszRateLimit);

    printf("<%s> returned %d.\n", pszCmdLine, _nmsProxy.setTransmitRateLimit (pszInterface, pszDestinationAddress, ui32RateLimit));
}

void NMSProxyShell::handleGetLinkCapacity(const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st(pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszInterface = st.getNextToken();

    printf("<%s> returned %u.\n", pszCmdLine, _nmsProxy.getLinkCapacity(pszInterface));
}

void NMSProxyShell::handleSetLinkCapacity(const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st(pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszInterface = st.getNextToken();
    const char *pszLinkCapacity = st.getNextToken();
    const uint32 ui32LinkCapacity = atoui32(pszLinkCapacity);

    _nmsProxy.setLinkCapacity(pszInterface, ui32LinkCapacity);
    printf("<%s> worked.\n", pszCmdLine);
}

void NMSProxyShell::handleGetNeighborQueueLength(const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st(pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszInterface = st.getNextToken();
    const char *pszSenderAddr = st.getNextToken();
    unsigned long ulSenderRemoteAddr = 0;
    if (pszSenderAddr != NULL) {
        const InetAddr inet(pszSenderAddr);
        ulSenderRemoteAddr = inet.getIPAddress();
    }

    printf("<%s> returned %d.\n", pszCmdLine, (int)_nmsProxy.getNeighborQueueLength(pszInterface, ulSenderRemoteAddr));
}

void NMSProxyShell::handleClearToSend(const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st(pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszInterface = st.getNextToken();

    printf("<%s> returned %d.\n", pszCmdLine, (int)_nmsProxy.clearToSend(pszInterface));
}

void NMSProxyShell::handleRegisterListener(const void *pToken, const char *pszCmdLine)
{
    StringTokenizer st(pszCmdLine);
    st.getNextToken();  // This is the command itself - discard
    const char *pszMsgType = st.getNextToken();
    uint32 ui32Type = (pszMsgType == NULL ? 0U : atoui32(pszMsgType));
    if (ui32Type > 0xFF) {
        printf("<%s> failed: message type %u too large.", pszCmdLine, ui32Type);
    }

    printf("<%s> returned %d.\n", pszCmdLine, _nmsProxy.registerHandlerCallback((uint8)ui32Type, this));
}

void NMSProxyShell::handlePing(const void *pToken, const char *pszCmdLine)
{
    _nmsProxy.ping();
}

int parseArguments(int argc, char *argv[], String &host, uint16 &ui16Port, String &homeDir, String &batchFile)
{
    homeDir = getProgHomeDir(argv[0]);

    for (int i = 1; i < argc;) {
        if (0 == stricmp(argv[i], "-host")) {
            i++;
            if (i < argc) {
                host = argv[i];
            }
        }
        else if (0 == stricmp(argv[i], "-port")) {
            i++;
            if (i < argc) {
                ui16Port = (uint16)atoi(argv[i]);
            }
        }
        else if (0 == stricmp(argv[i], "-load")) {
            i++;
            if (i < argc) {
                String tmp(argv[i]);
                tmp.trim();
                if (!tmp.startsWith("/")) {
                    // relative path - make it absolute
                    batchFile = homeDir;
                    batchFile += "/";
                }
                batchFile += tmp;
            }
        }
        else {
            printf("usage: %s [-host <host>] [-port <port>] [-load <BatchFile>]\n", argv[0]);
            return -1;
        }
        i++;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    if (!pLogger) {
        pLogger = new Logger();
    }
    pLogger->enableScreenOutput();
    pLogger->setDebugLevel(Logger::L_LowDetailDebug);

    String host("127.0.0.1");
    uint16 ui16Port = NMSProperties::DEFAULT_NMS_PROXY_PORT;
    String homeDir;
    String batchFile;
    if (parseArguments(argc, argv, host, ui16Port, homeDir, batchFile) < 0) {
        printf("Usage: %s -host hostname -port port -load batchfile\n", argv[0]);
    }

    printf("using host <%s> and port <%d> to connect to DisService\n",
        (const char *)host, (int)ui16Port);
    NMSProxyShell shell;
    shell.setPrompt("NMSProxy");
    int rc = shell.init(host, ui16Port);
    if (rc < 0) {
        printf("could not init network message service. Return code: %d.\n", rc);
    }

    if (batchFile.length() > 0) {
        // Batch mode
        printf("Running in batch mode\n");
        String cmdLine("load ");
        cmdLine += batchFile;
        char *pszCmdLine = cmdLine.r_str();
        if (pszCmdLine != NULL) {
            shell.processCmd(NULL, pszCmdLine);
            free(pszCmdLine);
        }
        while (true) {
            sleepForMilliseconds(5000);
        }
    }
    else {
        // Interactive mode
        printf("Running in interactive mode\n");
        shell.run();
    }

    return 0;
}

