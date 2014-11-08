/* Client/server test the client sends
 * 2 flows of position updates (from different sensors):
 *      position updates sensor 1: reliable, sequenced, low priority, with message replacement
 *      position updates sensor 2: reliable, sequenced, low priority, with message replacement
 * position update packets of 1KB
 * The test is ment to be run in two different machines using NistNet to work on the delay, packet loss, and bandwidth
 */

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iosfwd>

#include "InetAddr.h"
#include "Logger.h"
#include "Mocket.h"
#include "MessageSender.h"
#include "NLFLib.h"
#include "ServerMocket.h"
#include "Socket.h"
#include "TCPSocket.h"
#include "Thread.h"

#include "Statistics.h"

#if defined (UNIX)
#define stricmp strcasecmp
#elif defined (WIN32)
#define stricmp _stricmp
#endif

#if defined (_DEBUG)
#include <crtdbg.h>
#endif

using namespace NOMADSUtil;        

#define POSITION_UPDATE_MSG_FREQUENCY 50
#define POSITION_UPDATE_SENSOR_1_TAG 1
#define POSITION_UPDATE_SENSOR_2_TAG 2
#define POSITION_UPDATE_SIZE 1024

bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec);

class ServerMocketThread : public Thread
	{
    public:
        ServerMocketThread (int usServerPort);
        void run (void);
		
    private:
        ServerMocket *_pServerMocket;
	};

class ConnHandler : public Thread
	{
    public:
        ConnHandler (Mocket *pMocket);
        void run (void);
		
    private:
        Mocket *_pMocket;
	};

ServerMocketThread::ServerMocketThread (int usServerPort)
{
    _pServerMocket = new ServerMocket();
    _pServerMocket->listen (usServerPort);
}

void ServerMocketThread::run (void)
{
    while (true) {
        Mocket *pMocket = _pServerMocket->accept();
		
        printf ("ServerMocket: got a connection\n");
        ConnHandler *pHandler = new ConnHandler (pMocket);
        pHandler->start();
    }
	
}

ConnHandler::ConnHandler (Mocket *pMocket)
{
    _pMocket = pMocket;
}

void ConnHandler::run (void)
{
    int i;
    int msgSize = POSITION_UPDATE_SIZE;
    char buf[msgSize];
	
    int iBytesRead = 0;
    int iBytesToRead = POSITION_UPDATE_SIZE;
    int iTime = 0;
    bool cont = true;
    
	
    printf ("ConnectionHandler: client handler thread started\n");
    int32 i32StreamId, i32msgId;
	
    int64 i64msgTS, now, i64Time0Pos, i64TS0Pos, i64Time0Pos2, i64TS0Pos2;
    int delay, replaced, lastIdRcvPos, lastIdRcvPos2;
    lastIdRcvPos = 0;
    lastIdRcvPos2 = 0;
	
	
    _pMocket->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);
	
    FILE *filePosUp = fopen ("QedTest2_position_update.txt", "w");
    if (filePosUp == NULL) {
        fprintf (stderr, "failed to append to file QedTest2_position_update.txt\n");
        return;
    }
    
    
    while (cont)  {
        if ((i = _pMocket->receive (buf, sizeof (buf))) > 0) {
//            printf ("Received %d bytes\n", i);
            i32StreamId = ((int32*)buf)[0];
            now = getTimeInMilliseconds();
            if (i32StreamId == POSITION_UPDATE_SENSOR_1_TAG) {             
                //now = getTimeInMilliseconds();
                // Extract the application sequence number and the timestamp from the msg
                i32msgId = ((int32*)buf)[4];
                i64msgTS = ((int64*)buf)[8];
                if (i32msgId == 0) {
                    i64Time0Pos = now;
                    i64TS0Pos = i64msgTS;
                    lastIdRcvPos = 0;
                }
                else {
                    delay = (int)(now - i64Time0Pos - (i64msgTS - i64TS0Pos));
                    replaced = i32msgId - lastIdRcvPos - 1;
                    lastIdRcvPos = i32msgId;
					
                    printf("Mocket PosUpdate 1 Recvd:: time %lu\ttimestamp %lu\tmessageId %d\tdelay %d\treplaced %d\n", 
						   (long)now, (long)i64msgTS, i32msgId, delay, replaced);
                    fprintf(filePosUp, "Mocket PosUpdate 1 Recvd:: %lu %lu %d %d\n", 
                            (long)now, (long)i64msgTS, i32msgId, delay);
                    fflush(filePosUp);
                }
            }
            else {
                if (i32StreamId == POSITION_UPDATE_SENSOR_2_TAG) {
                    //now = getTimeInMilliseconds();
                    // Extract the application sequence number and the timestamp from the msg
                    i32msgId = ((int32*)buf)[4];
                    i64msgTS = ((int64*)buf)[8];
                    if (i32msgId == 0) {
                        i64Time0Pos2 = now;
                        i64TS0Pos2 = i64msgTS;
                        lastIdRcvPos2 = 0;
                    }
                    else {
                        delay = (int)(now - i64Time0Pos2 - (i64msgTS - i64TS0Pos2));
                        replaced = i32msgId - lastIdRcvPos2 - 1;
                        lastIdRcvPos2 = i32msgId;

                        printf("Mocket PosUpdate 2 Recvd:: time %lu\ttimestamp %lu\tmessageId %d\tdelay %d\treplaced %d\n", 
                                                       (long)now, (long)i64msgTS, i32msgId, delay, replaced);
                        fprintf(filePosUp, "Mocket PosUpdate 2 Recvd:: %lu %lu %d %d\n", 
                                (long)now, (long)i64msgTS, i32msgId, delay);
                        fflush(filePosUp);
                    }
                }
                else {
                    printf ("Empty msg\n");
                }
            }
        }
        else {
            printf ("receive returned %d; closing connection\n", i);
            cont = false;
            break;
        }
    }
	
    printf("closing the mocket connection\n");
    fclose (filePosUp);
	
    _pMocket->close();
	
    delete _pMocket;
    
    printf ("ConnectionHandler: client handler thread finished\n");
}

class PosUpdateSend : public Thread
	{
    public:
        PosUpdateSend (Mocket *pMocket, int iter);
        void run (void);
		
    private:
        Mocket *_pMocket;
        int _iter;
	};

PosUpdateSend::PosUpdateSend (Mocket *pMocket, int iter)
{
    _pMocket = pMocket;
    _iter = iter;
}

void PosUpdateSend::run(void)
{
    int rc;
    // Packet size 1Kb
    const int msgSize = POSITION_UPDATE_SIZE;
    static char bufPosUpdate [msgSize];
    uint32 ui32;
    
    // Reliable sequenced service
    MessageSender senderRelSeq = _pMocket->getSender (true, true);
    
    for (ui32 = 0; ui32 < _iter; ui32++) {
        printf("Client iteration %d\n", ui32);
        // Insert the ID for Position Updates msg
        ((int*)bufPosUpdate)[0] = POSITION_UPDATE_SENSOR_1_TAG;
        // Insert a sequence number in the msg
        ((int*)bufPosUpdate)[4] = ui32;
        // Insert the current time in the packet
        ((int64*)bufPosUpdate)[8] = getTimeInMilliseconds();
        // Send using the replace functionality (default priority used, value of 5)
        senderRelSeq.replace (bufPosUpdate, sizeof (bufPosUpdate), POSITION_UPDATE_SENSOR_1_TAG, POSITION_UPDATE_SENSOR_1_TAG);
        // Insert the ID for Position Updates msg
        ((int*)bufPosUpdate)[0] = POSITION_UPDATE_SENSOR_2_TAG;
        // Insert a sequence number in the msg
        ((int*)bufPosUpdate)[4] = ui32;
        // Insert the current time in the packet
        ((int64*)bufPosUpdate)[8] = getTimeInMilliseconds();
        // Send using the replace functionality (default priority used, value of 5)
        senderRelSeq.replace (bufPosUpdate, sizeof (bufPosUpdate), POSITION_UPDATE_SENSOR_2_TAG, POSITION_UPDATE_SENSOR_2_TAG);
        
        sleepForMilliseconds(POSITION_UPDATE_MSG_FREQUENCY);
    }
    
    printf ("DONE!!!\n");
    MocketStats *ms = _pMocket->getStatistics();
    MocketStats::MessageStats *posMsgStat = ms->getMessageStatisticsForType(POSITION_UPDATE_SENSOR_1_TAG);
    MocketStats::MessageStats *sensMsgStat = ms->getMessageStatisticsForType(POSITION_UPDATE_SENSOR_2_TAG);
    
    printf ("Sent reliable sequenced (position update): %d\n", posMsgStat->ui32SentReliableSequencedMsgs);
    printf ("Sent reliable unsequenced (sensor update): %d\n", sensMsgStat->ui32SentReliableUnsequencedMsgs);
    printf ("Retransmitted: %d\n", ms->getRetransmitCount());
    //printf ("Retransmitted sensor updates: %d", sensMsgStat->getRetransmitCount());
    
    // TODO
    
	/*    m.lock();
	 mainthread._bPositionUpdateSendTerminated = true;
	 cv.notifyAll();
	 m.unlock();
	 */    
    
}



int doClientTask (const char *pszRemoteHost, unsigned short usRemotePort, unsigned short usIter, Statistics *pStats)
{
    int rc;
    
    int numIterations = (int) usIter;
	
    Mocket *pMocket = new Mocket();
    pMocket->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);
	
    if (0 != (rc = pMocket->connect (pszRemoteHost, usRemotePort))) {
        fprintf (stderr, "doClientTask: failed to connect using Mockets to remote host %s on port %d; rc = %d\n",
                 pszRemoteHost, usRemotePort, rc);
        printf ("doClientTask: Unable to connect\n");
        delete pMocket;
        return -1;
    }
    
	/*    Mutex m;
	 ConditionVariable cv(&m);
	 */    
    // Start threads
    PosUpdateSend *sendPositionUpdates = new PosUpdateSend (pMocket, numIterations);
    sendPositionUpdates->start();
    
    
	/*    
	 m.lock();
	 while ((!_bPositionUpdateSendTerminated) && (!_bSensorUpdateSendTerminated)) {
	 cv.wait();
	 }
	 m.unlock();
	 printf("closing the mocket\n");
	 pMocket->close();
	 //!!// I need this sleep otherwise the program ends!! Why?
	 sleepForMilliseconds(10000);
	 */
    while (true) {
        sleepForMilliseconds (2000);
    }
    
    return 0;
}

int main (int argc, char *argv[])
{
    pLogger = new Logger();
    pLogger->initLogFile ("qed.log", false);
    pLogger->enableFileOutput();
    pLogger->disableScreenOutput();
    pLogger->setDebugLevel (Logger::L_MediumDetailDebug);
	
#if defined (WIN32) && defined (_DEBUG)
	//getchar();    // Useful if you need to attach to the process before the break alloc happens
	_CrtSetReportMode (_CRT_ERROR, _CRTDBG_MODE_FILE);
	_CrtSetReportFile (_CRT_ERROR, _CRTDBG_FILE_STDERR);
	_CrtSetReportMode (_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile (_CRT_WARN, _CRTDBG_FILE_STDERR);
	//_CrtSetBreakAlloc (58);
#endif
	
    if ((argc != 3) && (argc != 4) && (argc != 5)) {
        fprintf (stderr, "usage: %s <server> <port> ---- %s <client> <port> <remotehost> [<iterations>]\n", argv[0], argv[0]);
        return -1;
    }
    else {        
        if (0 == stricmp (argv[1], "server")) {
            unsigned short usPort = atoi (argv[2]);
            
            printf ("Creating a ServerMocket on port %d\n", (int) usPort);
            ServerMocketThread *pSMT = new ServerMocketThread (usPort);
            pSMT->run();
        }
        else if (0 == stricmp (argv[1], "client")) {
            unsigned short usRemotePort = atoi (argv[2]);
            unsigned short usIterations;
            if (argc == 5) {
                usIterations = atoi (argv[4]);
            }
            else {
                usIterations = 20;
            }
            const char *pszRemoteHost = argv[3];
            printf ("Client Creating a Mocket to host %s on port %d\n", pszRemoteHost, (int) usRemotePort);
			
            Statistics mocketStats;
            int rc;
            if (0 != (rc = doClientTask (pszRemoteHost, usRemotePort, usIterations, &mocketStats))) {
                fprintf (stderr, "main: doClientTask failed for mockets with rc = %d\n", rc);
                return -3;
            }
			
        }
    }
	
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



