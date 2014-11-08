#include "Mocket.h"
#include "MessageSender.h"
#include "ServerMocket.h"

#include "Logger.h"

#include <stdio.h>

#if defined (WIN32)
    #include <crtdbg.h>
    #define stricmp _stricmp
#elif defined (UNIX)
    #define stricmp strcasecmp
    #include <strings.h>
#endif

using namespace NOMADSUtil;
        
bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec);
int handleConnectionFromClient (Mocket *pMocket);
bool validateMessage (char *pMsgBuf, uint32 ui32BufSize, uint32 ui32MsgNum);
int handleConnectionToServer (Mocket *pMocket);
void fillInMessage (char *pMsgBuf, uint32 ui32BufSize, uint32 ui32MsgNum);

#define MSG_SIZE 10240  // Change Validate message if this is smaller than 8192

int main (int argc, char *argv[])
{
    #if defined (WIN32) && defined (_DEBUG)
        //getchar();    // Useful if you need to attach to the process before the break alloc happens
        _CrtSetReportMode (_CRT_ERROR, _CRTDBG_MODE_FILE);
        _CrtSetReportFile (_CRT_ERROR, _CRTDBG_FILE_STDERR);
        _CrtSetReportMode (_CRT_WARN, _CRTDBG_MODE_FILE);
        _CrtSetReportFile (_CRT_WARN, _CRTDBG_FILE_STDERR);
        //_CrtSetBreakAlloc (66);
    #endif


    pLogger = new Logger();
    pLogger->disableScreenOutput();
    pLogger->setDebugLevel (Logger::L_MediumDetailDebug);
    if (argc < 3) {
        fprintf (stderr, "usage: %s <server|client> <port> [<remotehost>]\n", argv[0]);
        return -1;
    }
    else {        
        if (0 == stricmp (argv[1], "server")) {
            pLogger->initLogFile ("unseqser.log", false);
            pLogger->enableFileOutput();
            unsigned short usPort = atoi (argv[2]);
            ServerMocket serverMocket;
            serverMocket.listen (usPort);
            Mocket *pMocket = serverMocket.accept();
            if (pMocket == NULL) {
                fprintf (stderr, "%s: failed to accept a connection\n", argv[0]);
                return -2;
            }
            fprintf (stdout, "received a connection\n");
            pMocket->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);
            handleConnectionFromClient (pMocket);
            delete pMocket;
            serverMocket.close();
        }
        else if (0 == stricmp (argv[1], "client")) {
            pLogger->initLogFile ("unseqcli.log", false);
            pLogger->enableFileOutput();
            if (argc < 4) {
                fprintf (stderr, "usage: %s client <port> <remotehost>\n", argv[0]);
                return -3;
            }
            unsigned short usRemotePort = atoi (argv[2]);
            const char *pszRemoteHost = argv[3];
            Mocket mocket;
            int rc;
            if (0 != (rc = mocket.connect (pszRemoteHost, usRemotePort))) {
                fprintf (stderr, "%s: failed to connect to %s:%d; rc = %d\n", argv[0], pszRemoteHost, (int)usRemotePort, rc);
                return -4;
            }
            fprintf (stdout, "connected; sending data\n");
            mocket.registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);
            handleConnectionToServer (&mocket);
            fprintf (stdout, "done sending data - sleeping for 15 seconds\n");
            sleepForMilliseconds (15000);
            mocket.close();
        }
    }

    delete pLogger;
    pLogger = NULL;

    #if defined (WIN32) && defined (_DEBUG)
        _CrtDumpMemoryLeaks();
    #endif

    return 0;
}

int handleConnectionFromClient (Mocket *pMocket)
{
    uint32 ui32NextMsg = 0;
    int64 i64TimeLastPacketReceived = 0;
    int64 i64IntervalStartTime = getTimeInMilliseconds();
    int32 i32IntervalPacketCount = 0;
    while (true) {
        char msgBuf[MSG_SIZE];
        int rc = pMocket->receive (&msgBuf, sizeof (msgBuf));
        int64 i64CurrTime = getTimeInMilliseconds();
        if (rc != sizeof (msgBuf)) {
            fprintf (stdout, "receive() returned %d; terminating\n", rc);
            fprintf (stdout, "last msg received = %d\n", ui32NextMsg - 1);
            return 0;
        }
        uint32 ui32Msg;
        memcpy (&ui32Msg, msgBuf, sizeof (uint32));
        if (ui32NextMsg < ui32Msg) {
            fprintf (stdout, "lost packet: expected %lu but got %lu; elapsed time = %lu\n", ui32NextMsg, ui32Msg, (uint32) (i64CurrTime - i64TimeLastPacketReceived));
        }
        else if (ui32NextMsg > ui32Msg) {
            fprintf (stdout, "ERROR: out of sequence packet - expected %lu but got %lu\n", ui32NextMsg, ui32Msg);
        }
        if (!validateMessage (&msgBuf[4], sizeof(msgBuf)-4, ui32Msg)) {
            fprintf (stdout, "ERROR: contents of message %lu are not valid\n", ui32Msg);
        }
        ui32NextMsg = ui32Msg + 1;
        i64TimeLastPacketReceived = i64CurrTime;
        i32IntervalPacketCount++;
        if ((i64CurrTime - i64IntervalStartTime) > 5000) {
            fprintf (stdout, "receive rate = %5.1fHz\n", (float) ((float)i32IntervalPacketCount*1000) / (i64CurrTime - i64IntervalStartTime));
            i64IntervalStartTime = i64CurrTime;
            i32IntervalPacketCount = 0;
        }
        sleepForMilliseconds (2000);    // Uncomment this line to force receiver to drop packets
    }
}

bool validateMessage (char *pMsgBuf, uint32 ui32BufSize, uint32 ui32MsgNum)
{
    // Check and make sure that the entire message contains the same alphabet
    char ch = (ui32MsgNum % 26) + 'A';
    for (uint32 ui32 = 0; ui32 < ui32BufSize; ui32++) {
        if (pMsgBuf[ui32] != ch) {
            return false;
        }
    }
    //printf ("%c%c%c%c\n", pMsgBuf[0], pMsgBuf[2047], pMsgBuf[4095], pMsgBuf[8191]);  // Just to satisfy visually...
    return true;
}

int handleConnectionToServer (Mocket *pMocket)
{
	uint32 ui32Msg;
    MessageSender sender = pMocket->getSender (false, true);
    for (ui32Msg = 0; ui32Msg < 10000; ui32Msg++) {
        char msgBuf[MSG_SIZE];
        memcpy (msgBuf, &ui32Msg, sizeof (uint32));
        fillInMessage (&msgBuf[4], sizeof(msgBuf)-4, ui32Msg);
        int64 i64StartTime = getTimeInMilliseconds();
        sender.send (msgBuf, sizeof (msgBuf));
        int64 i64ElapsedTime = getTimeInMilliseconds() - i64StartTime;
        if (i64ElapsedTime > 10) {
            fprintf (stdout, "blocked for %lu ms\n", (uint32) i64ElapsedTime);
        }
        sleepForMilliseconds (20);
    }
    fprintf (stdout, "last message sent = %d\n", ui32Msg - 1);
    return 0;
}

void fillInMessage (char *pMsgBuf, uint32 ui32BufSize, uint32 ui32MsgNum)
{
    // Fill the entire buffer of the message with the same alphabet (chosen from 'A' to 'Z')
    char ch = (ui32MsgNum % 26) + 'A';
    for (uint32 ui32 = 0; ui32 < ui32BufSize; ui32++) {
        pMsgBuf[ui32] = ch;
    }
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
