/* 
 * File:   UnreliableIntDataTest.cpp
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses
 */

#include <stdio.h>
#include <stdlib.h>

#include "DatagramSocket.h"
#include "UDPDatagramSocket.h"
#include "FTypes.h"
#include "InetAddr.h"
#include "NLFLib.h"
#include "OSThread.h"
#include "ServerMocket.h"
#include "Mocket.h"
#include "MessageSender.h"

#if defined (UNIX)
    #define stricmp strcasecmp
    #include <strings.h>
#endif        
        
#define UDP_PORT_NUM 8002
#define MOCKETS_PORT_NUM 9002

#define MINIMUM_PACKET_SIZE 8
#define MAXIMUM_PACKET_SIZE 65535

using namespace NOMADSUtil;

void printUsage (void);
int server (void);
int client (const char *pszServerIP, uint16 ui16PacketSize, uint16 ui16NumPackets, bool bMockets, bool bSequenced);
void clientReceiver (void *pArg);
void serverMocket (void *pArg);
int udpServer (void);

ServerMocket msgServerMocket;
UDPDatagramSocket dgSocket;
int dgSocketClientPortNum = UDP_PORT_NUM + 1;

FILE * pLogFile = NULL;

char buf[MAXIMUM_PACKET_SIZE];
char logbuf[512];

int server (void)
{
    pLogFile = fopen ("unreliable_data_test.log", "a");
    if (pLogFile == NULL) {
        printf ("error creating the logfile\n");
        exit(1);
    }

    int rc;
    if ( (rc = msgServerMocket.listen (MOCKETS_PORT_NUM)) < 0 ) {
        printf ("server:: error on msgServerMocket.listen(). rc = %d\n", rc);
        return -1;
    }
    if (0 != (rc = dgSocket.init (UDP_PORT_NUM))) {
        fprintf (stderr, "failed to bind datagram socket to port %d; rc = %d\n", UDP_PORT_NUM, rc);
        return -2;
    }

    //printf ("server():: before mocketServerThread()\n");
    OSThread mocketServerThread;
    mocketServerThread.start (serverMocket, NULL);

    rc = udpServer();

    return rc;
}

void serverMocket(void *pArg)
{
    int rc;
    uint32 ui32SeqNum;

    while (true) {
        int lastSequenceNumber = -1;
        int numPacketsRecvd = 0;
        int numPacketsLost = 0;

        int outOfOrderPackets = 0;
        uint64 ui64FirstPacketTS = 0;
        uint64 ui64TotalBytesReceived = 0;

        //printf ("serverMocket:: before msgServerMocket.accept()\n");
        Mocket *msgMocket = msgServerMocket.accept();

        if (msgMocket == NULL) {
            printf ("serverMocket:: msgServerMocket.accept() returned NULL\n");
            return;
        }

        while (true) {
            rc = msgMocket->receive (buf, MAXIMUM_PACKET_SIZE, 0);
            if (rc < 0) {
                break;
            }
            ui32SeqNum = *( (uint32*)buf );
            //printf ("serverMocket:: received a packet of size %d with seq. number = %d\n", rc, ui32SeqNum);
            numPacketsRecvd++;
            ui64TotalBytesReceived += rc;

            if (lastSequenceNumber == -1) {
                lastSequenceNumber = ui32SeqNum;
                ui64FirstPacketTS = getTimeInMilliseconds();
                //printf ("Mockets ui64FirstPacketTS %llu\n", ui64FirstPacketTS);
                continue;
            }

            if (ui32SeqNum < lastSequenceNumber) {
                outOfOrderPackets++;
            }
            else if (ui32SeqNum != (lastSequenceNumber + 1)) {
                numPacketsLost += ui32SeqNum - lastSequenceNumber;
            }

            lastSequenceNumber = ui32SeqNum;
        }
        //printf ("Mockets: start time = %llu, now = %llu, received %llu bytes\n", ui64FirstPacketTS, getTimeInMilliseconds(), ui64TotalBytesReceived);

        //printf ("Mockets done timestamp %llu\n", getTimeInMilliseconds());
        sprintf (logbuf, "Mockets::\tLoss = %d%\tOutOfOrder = %d\tThroughput = %5.2f bytes/sec\n", 
                (numPacketsLost * 100)/(lastSequenceNumber + 1),
                outOfOrderPackets, 
                (float) ui64TotalBytesReceived / (getTimeInMilliseconds() - ui64FirstPacketTS)
               );

        printf ("%s", logbuf);
        fprintf (pLogFile, "%s", logbuf);

        msgMocket->close();
        delete msgMocket;
    }
}

int udpServer (void)
{
    int rc; 
    uint32 ui32SeqNum;

    uint16 ui16ClientPort = 0;
    int lastSequenceNumber = -1;
    int numPacketsRecvd = 0;
    int numPacketsLost = 0;
    uint64 ui64FirstPacketTS = 0;
    uint64 ui64LastPacketTS = 0;
    uint64 ui64TotalBytesReceived = 0;

    int outOfOrderPackets = 0;

    while (true) {
        InetAddr clientAddr;
        // ui64LastPacketTS will be used when client port changes to calculate
        // how long it took to receive a chunk of data
        ui64LastPacketTS = getTimeInMilliseconds();
        if ((rc = dgSocket.receive (buf, sizeof (buf), &clientAddr)) <= 0) {
            fprintf (stderr, "failed to receive from datagram socket; rc = %d\n", rc);
            continue;
        }

        ui32SeqNum = *( (uint32*)buf );
        //printf ("udpServer:: received a packet with seq. number = %d and size %d from remote port %d\n", ui32SeqNum, rc, clientAddr.getPort());

        if (ui16ClientPort != clientAddr.getPort()) {
            if ( (ui16ClientPort != 0) && (lastSequenceNumber >= 0) ) {
                // The statistics can be printed only when the second cycle starts
                //printf ("UDP done timestamp %llu\n", ui64LastPacketTS);
                sprintf (logbuf, "UDP::\t\tLoss = %d%\tOutOfOrder = %d\tThroughput = %5.2f bytes/sec\n", 
                        (numPacketsLost * 100)/(lastSequenceNumber + 1),
                        outOfOrderPackets, 
                        (float) ui64TotalBytesReceived / (ui64LastPacketTS - ui64FirstPacketTS), 
                        ui64TotalBytesReceived
                       );

                printf ("%s", logbuf);
                fprintf (pLogFile, "%s", logbuf);
            }

            lastSequenceNumber = ui32SeqNum;
            numPacketsRecvd = 0;
            numPacketsLost = 0;
            outOfOrderPackets = 0;

            ui64FirstPacketTS = getTimeInMilliseconds();
            //printf ("UDP ui64FirstPacketTS %llu\n", ui64FirstPacketTS);
            ui64TotalBytesReceived = 0;

            ui16ClientPort = clientAddr.getPort();
        }
        
        numPacketsRecvd++;
        ui64TotalBytesReceived += rc;

        if (ui32SeqNum < lastSequenceNumber) {
            outOfOrderPackets++;
        }
        else if (ui32SeqNum != (lastSequenceNumber + 1)) {
            numPacketsLost += ui32SeqNum - lastSequenceNumber;
        }

        lastSequenceNumber = ui32SeqNum;
    }

    return 0;
}

int client (const char *pszServerIP, uint16 ui16PacketSize, uint32 ui32DataLength, bool bMockets, bool bSequenced)
{
    int rc;
    uint32 ui32Aux, ui32BytesSent;
    char *buf = (char*) malloc (ui16PacketSize);
    memset (buf, 0, ui16PacketSize);

    if (bMockets) {
        Mocket msgMocket;
        printf ("client:: trying to connect to %s\n", pszServerIP);
        rc = msgMocket.connect (pszServerIP, MOCKETS_PORT_NUM);
        if (rc < 0) {
            printf ("client:: error connecting. quitting.\n");
            free (buf);
            return -1;
        }

        MessageSender msgSender = msgMocket.getSender (false, bSequenced);
        for (ui32Aux = 0, ui32BytesSent = 0; ui32BytesSent < ui32DataLength; ui32Aux++) {
            memcpy (buf, &ui32Aux, sizeof(uint32));
            uint32 ui32Size = (ui32DataLength - ui32BytesSent);
            
            if (ui32Size < ui16PacketSize) {
                //the last packet...
                ui32Size = ui32Size % ui16PacketSize;
            }
            else {
                ui32Size = ui16PacketSize;
            }

            if ((rc = msgSender.send (buf, ui32Size)) < 0) {
                fprintf (stderr, "failed to send mocket packet to %s:%d; rc = %d\n", pszServerIP, MOCKETS_PORT_NUM, rc);
                free (buf);
                return -2;
            }
            ui32BytesSent += ui32Size;
        }
/*
        for (ui32Aux = 0; ui32Aux < ui16NumPackets; ui32Aux++) {
            memcpy (buf, &ui32Aux, sizeof(uint32));
            rc = msgSender.send (buf, ui16PacketSize);
        }
*/
        msgMocket.close();
    }
    else {
        UDPDatagramSocket dgSocket;
        dgSocket.init(dgSocketClientPortNum++);

        for (ui32Aux = 0, ui32BytesSent = 0; ui32BytesSent < ui32DataLength; ui32Aux++) {
            memcpy (buf, &ui32Aux, sizeof(uint32));
            uint32 ui32Size = (ui32DataLength - ui32BytesSent);
            
            if (ui32Size < ui16PacketSize) {
                //the last packet...
                ui32Size = ui32Size % ui16PacketSize;
            }
            else {
                ui32Size = ui16PacketSize;
            }

            if ((rc = dgSocket.sendTo (pszServerIP, UDP_PORT_NUM, buf, ui32Size)) < 0) {
                fprintf (stderr, "failed to send datagram packet to %s:%d; rc = %d\n", pszServerIP, UDP_PORT_NUM, rc);
                free (buf);
                return -2;
            }

            ui32BytesSent += ui32Size;
        }    

/*
        for (ui32Aux = 0; ui32Aux < ui16NumPackets; ui32Aux++) {
            memcpy (buf, &ui32Aux, sizeof(uint32));

            if ((rc = dgSocket.sendTo (pszServerIP, UDP_PORT_NUM, buf, ui16PacketSize)) < 0) {
                fprintf (stderr, "failed to send datagram packet to %s:%d; rc = %d\n", pszServerIP, UDP_PORT_NUM, rc);
                return -2;
            }
        }    
*/
    }

    free (buf);
    return 0;
}

void printUsage (void)
{
    fprintf (stderr, "usage: UnreliableIntDataTest server\n"
                     "       UnreliableIntDataTest client <sequenced|unsequenced> <serverip> <packetsize (in bytes)> <datalength (in MB)>\n");
}

int main (int argc, char *argv[])
{
    if (argc < 2) {
        printUsage();
        return -1;
    }

    if (0 == stricmp (argv[1], "server")) {
        return server();
    }
    else if (0 == stricmp (argv[1], "client")) {
        if (argc < 6) {
            printUsage();
            return -2;
        }

        if ( (0 != stricmp(argv[2], "sequenced")) && (0 != stricmp(argv[2], "unsequenced")) ) {
            printUsage();
            return -3;
        }

        bool bSequenced = (0 == stricmp(argv[2], "sequenced"));

        const char *pszServerIP = argv[3];
        uint16 ui16PacketSize = (uint16) atoi (argv[4]);
        uint32 ui32DataLength = (uint32) atoi (argv[5]) * 1024 * 1024;
        uint16 ui16NumIterations = 100;

        if (argc > 4) {
            int iPacketSize = atoi (argv[4]);
            if ((iPacketSize < MINIMUM_PACKET_SIZE) || (iPacketSize > MAXIMUM_PACKET_SIZE)) {
                printUsage();
                return -4;
            }
            else {
                ui16PacketSize = (uint16) iPacketSize;
            }
        }

        for (int i = 0; i < ui16NumIterations; i++) {
            if (client(pszServerIP, ui16PacketSize, ui32DataLength, true, bSequenced)) {
                return -10;
            }
            sleepForMilliseconds (2000);

            if (client(pszServerIP, ui16PacketSize, ui32DataLength, false, bSequenced)) {
                return -11;
            }
            sleepForMilliseconds (2000);
        }
    }
    else {
        printUsage();
        return -4;
    }

    return 0;
}
