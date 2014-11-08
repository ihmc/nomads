/* This test is supposed to be used with a server side (file MigrationFileRec).
 * This file contains a client-sender and a client-receiver.
 * The client-sender starts the communication with the server
 * and then suspends and sends the mocket status to a new client node.
 * The client-receiver restores the state after the migration and continues the communication.
 * The test sends a file to the server sending only the even packets from the client-sender
 * and after the migration sending the odd packets from the client-receiver to the server.
 * Test files can be found under mockets/test/cpp: migration_file_10k.txt migration_file_15k.txt
 * migration_file_100k.txt migration_file_150k.txt
 */
#include "Mocket.h"
#include "MessageSender.h"

#include <stdio.h>
#if defined (OSX)
    #include <strings.h>
#endif

#include "Logger.h"

#include "NLFLib.h"
#include "TCPSocket.h"
#include "BufferedReader.h"
#include "BufferedWriter.h"
#include "SocketReader.h"
#include "SocketWriter.h"
#include "ObjectDefroster.h"
#include "ObjectFreezer.h"

#if defined (UNIX)
    #define stricmp strcasecmp
#elif defined (WIN32)
    #define stricmp _stricmp
#endif

#define REM_PORT 4000

#if defined (_DEBUG)
    #include <crtdbg.h>
#endif       


using namespace NOMADSUtil;

bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec);

int main (int argc, char *argv[])
{
    #if defined (WIN32) && defined (_DEBUG)
        //getchar();    // Useful if you need to attach to the process before the break alloc happens
        _CrtSetReportMode (_CRT_ERROR, _CRTDBG_MODE_FILE);
        _CrtSetReportFile (_CRT_ERROR, _CRTDBG_FILE_STDERR);
        _CrtSetReportMode (_CRT_WARN, _CRTDBG_MODE_FILE);
        _CrtSetReportFile (_CRT_WARN, _CRTDBG_FILE_STDERR);
        //_CrtSetBreakAlloc (58);
    #endif

    if (argc < 2) {
        fprintf (stderr, "usage: %s <receiver | sender <filetosend> <server-hostname> <client-hostname> >\n", argv[0]);
        return -1;
    }
        
    pLogger = new Logger();
    pLogger->initLogFile ("filesend.log", false);
    pLogger->enableFileOutput();
    pLogger->disableScreenOutput();
    pLogger->setDebugLevel (Logger::L_MediumDetailDebug);
    
    Mocket *pMocket = new Mocket();
    pMocket->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);
    
    if (0 == stricmp (argv[1], "sender")) {
        if (argc != 5) {
            fprintf (stderr, "usage: %s <receiver | sender <filetosend> <server-hostname> <client-hostname> >\n", argv[0]);
            return -1;
        }
        
        pMocket->setIdentifier ("FileSend-Client");
        
        FILE *file = fopen (argv[2], "rb");
        if (file == NULL) {
            fprintf (stderr, "failed to open file %s\n", argv[2]);
            return -2;
        }

        // With this method we debug state capture by setting in mockets that only even packets can be sent.
        // Odd packets will remain in the queues and migrated to the new node along with the status of the mocket.
        pMocket->debugStateCapture();

        int rc;
        // Exchange security keys at connection time with parameter bSupportConnectivity = true
        // Or exchange keys as part of suspend/resume process with parameter bSupportConnectivity = false
        bool bSupportConnectivity = true;
        if (0 != (rc = pMocket->connect (argv[3], REM_PORT, bSupportConnectivity))) {
            fprintf (stderr, "failed to connect to host %s; rc = %d\n", argv[3], rc);
            delete pMocket;
            return -3;
        }

        char * buf = NULL;
        int n;
        int buffsize = 512;

        MessageSender sender = pMocket->getSender (true, true);
        while (true) {
            if (buffsize > 2048) {
                buffsize /= 5;
            }
            buf = (char*) malloc(buffsize);

            if ((n = fread (buf, 1, buffsize, file)) <= 0) {
                printf ("%s: EOF reached\n", argv[0]);
                free (buf);
                break;
            }
            else {
                printf("using buffer of size %d. Sending %d bytes\n", buffsize, n);
                sender.send (buf, n);
            }

            free(buf);
            // Increase the buffer size so we test with different packet dimensions
            buffsize *= 1.25;
        }
        fclose (file);
        // Suspend
        uint64 ui64StartSuspend = getTimeInMilliseconds();
        printf ("Mocket suspend finished with status %d\n", pMocket->suspend(0, 5000));
        printf ("TIME FOR SUSPENSION **  %llu  **\n", (getTimeInMilliseconds() - ui64StartSuspend));

        // Open a socket to transfer the data to the new client node
        String strNextNodeHost = argv[4];
        int iNextNodePort = 4444;
        TCPSocket *pSocketToNextNode;
        pSocketToNextNode = new TCPSocket();
        while (true) {
            printf ("trying to connect to %s:%d...\n", (const char*) strNextNodeHost, iNextNodePort);
            fflush (stdout);
            if (0 != (rc = pSocketToNextNode->connect (strNextNodeHost, iNextNodePort))) {
                printf ("failed - will try again in 2 seconds\n");
                sleepForMilliseconds (2000);
            }
            else {
                printf ("Connected\n");
                break;
            }
        }

        SocketWriter sw (pSocketToNextNode);
        BufferedWriter bw (&sw, 2048);
        
        // Freeze
        int result;
        uint64 ui64StartFreeze = getTimeInMilliseconds();
        result = pMocket->getState (&bw);
        printf ("TIME FOR GETSTATE **  %llu  **\n", (getTimeInMilliseconds() - ui64StartFreeze));
        printf ("getState ended with status %d\n", result);
        if (0 != result) {
            delete pMocket;
            return -3;
        }
    }

    else if (0 == stricmp (argv[1], "receiver")) {
        // Create the Server Socket
        int rc;
        TCPSocket *pServerSocket;
        pServerSocket = new TCPSocket();
        int iLocalPortNum = 4444;
        if (0 != (rc = pServerSocket->setupToReceive (iLocalPortNum))) {
            return -3;
        }
        printf ("listening on port %d\n", iLocalPortNum);
        
        Socket *pSocketFromPreviousNode;
        // Wait for connection from previous node
        printf ("Waiting for connection from previous node...\n");
        fflush (stdout);
        pSocketFromPreviousNode = pServerSocket->accept();
        if (pSocketFromPreviousNode == NULL) {
            printf ("Failed\n");
            return -4;
        }
        printf ("Connected\n");
        pSocketFromPreviousNode->bufferingMode(0);
        
        SocketReader sr (pSocketFromPreviousNode);
        BufferedReader br (&sr);
        
        // Resume
        int result;
        uint64 ui64StartResumeAndRestoreState = getTimeInMilliseconds();
        result = pMocket->resumeAndRestoreState (&br);
        printf ("TIME FOR RESUMEANDRESTORESTATE **  %llu  **\n", (getTimeInMilliseconds() - ui64StartResumeAndRestoreState));
        printf ("resumeFromSuspension ended with status %d\n", result);
        
        if (0 != result) {
            printf ("ERROR\n");
            delete pMocket;
            return -5;
        }

        /*
         * The connection has been restored successfully.
         * We do not set to skip odd packets with the method debugStateCapture().
         * Odd packets that were in the queues and have migrated to the new node will be sent now.
         * Sleep for a little bit before closing the connection so that the queues can be emptied.
         */
        sleepForMilliseconds (5000);

        delete (pServerSocket);
        pServerSocket = NULL;
        delete (pSocketFromPreviousNode);
        pSocketFromPreviousNode = NULL;
        pMocket->close();
    }
    else {
        printf ("ERROR\n");
        return 0;
    }
    
    printf ("Mocket statistics:\n");
    printf ("    Packets sent: %d\n", pMocket->getStatistics()->getSentPacketCount());
    printf ("    Bytes sent: %d\n", pMocket->getStatistics()->getSentByteCount());
    printf ("    Packets received: %d\n", pMocket->getStatistics()->getReceivedPacketCount());
    printf ("    Bytes received: %d\n", pMocket->getStatistics()->getReceivedByteCount());
    printf ("    Retransmits: %d\n", pMocket->getStatistics()->getRetransmitCount());

    delete pMocket;

    delete pLogger;
    pLogger = NULL;

    #if defined (WIN32) && defined (_DEBUG)
        _CrtDumpMemoryLeaks();
    #endif
    return 0;
}

bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec)
{
    printf ("peer unreachable warning: %lu ms\n", ulMilliSec);
    if (ulMilliSec > 10000) {
        return true;
    }
    else {
        return false;
    }
}

