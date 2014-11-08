/* 
 * File:   RemoteStatsTest.cpp
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses
 */

#include "Mocket.h"
#include "MocketStatusNotifier.h"

#include <cstdlib>


using namespace std;
using namespace NOMADSUtil;

/*
 * 
 */
int main(int argc, char** argv)
{
    bool bSender = true;
    if (argc < 2) {
        fprintf (stderr, "1- usage: %s sender <ip> or %s receiver\n", argv[0], argv[0]);
        return -1;
    }
    
    if (0 == strcmp (argv[1], "sender")) {
        bSender = true;
        if (argc !=3) {
            fprintf (stderr, "A receiver must be defined\n");
            fprintf (stderr, "2- usage: %s sender <ip> or %s receiver\n", argv[0], argv[0]);
            return -1;
        }
    }
    else if (0 == strcmp (argv[1], "receiver")) {
        bSender = false;
    }
    else {
        fprintf (stderr, "3- usage: %s sender <ip> or %s receiver\n", argv[0], argv[0]);
        return -1;
    }
    
    int port = 1400;
    
    // Sender sends stats
    if (bSender) {
        printf ("RemoteStatsTest - sender\n");
        MocketStatusNotifier *pMocketStatusNotifier;
        pMocketStatusNotifier = new MocketStatusNotifier();
        pMocketStatusNotifier->init (argv[2], port, false);
        while (true) {
            pMocketStatusNotifier->disconnected ("abcd");
            printf ("Message sent\n");
            sleepForMilliseconds (2000);
        }
    }
    // Receiver listens for stats
    else {
        printf ("RemoteStatsTest - receiver\n");
        NOMADSUtil::UDPDatagramSocket *pDGSocket = new UDPDatagramSocket();
        if (pDGSocket->init (port)) {
            printf ("ERROR\n");
            return -1;
        }
        
        int rc = 0;
        char buf [1024];
        InetAddr remoteAddr;
        while (true) {
            rc = pDGSocket->receive (buf, 1024, &remoteAddr);
            if (rc>0) {
                printf ("Received: %s\n", buf);
            }
            
        }
    }
    
    

    return 0;
}

