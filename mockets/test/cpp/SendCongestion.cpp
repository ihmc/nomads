#include "Mocket.h"
#include "MessageSender.h"

#include <stdio.h>
#include <sstream>
#include <csignal>
#include <cstring>
#include <ctime>
#include <cstdlib>


#include "Logger.h"

#define REM_PORT 4000

#if defined (_DEBUG)
    #include <crtdbg.h>
#endif       

using namespace NOMADSUtil;

bool keepSending = true;

bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec);
void stopSending( int status );

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

    pLogger = new Logger();
    pLogger->initLogFile ("sendcongestion.log", false);
    pLogger->enableFileOutput();
    pLogger->disableScreenOutput();
    //pLogger->setDebugLevel (Logger::L_MediumDetailDebug);
    pLogger->setDebugLevel (Logger::L_HighDetailDebug);

    if (argc != 7) {
        fprintf (stderr, "usage: %s {-k <kilobytes> || -s <seconds>} <hostname> <port> <congestionControl> <random/fixed size>\n", argv[0]);
        return -1;
    }
   
    if( strcmp(argv[1],"-k") && strcmp(argv[1],"-s") ) {
        fprintf (stderr, "first argument must be -k or -s\n");
        return -1;
    }
    bool seconds;
    if( strcmp("-k", argv[1]) )
        seconds = true;
    else seconds = false;
 
    std::stringstream ss(argv[2]); // Could of course also have done ss("1234") directly.
    int k_or_s;
  
    if( (ss >> k_or_s).fail() )
    {
        fprintf (stderr, "second argument must be a numerical string \n");
        return -1;
    }

    std::stringstream sss(argv[4]);
    int port;
  
    if( (sss >> port).fail() )
    {
        fprintf (stderr, "fourth argument must be a numerical string \n");
        return -1;
    }

    int rc;
    Mocket *pm = new Mocket();
    pm->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);
    if (0 != (rc = pm->connect (argv[3], port))) {

        fprintf (stderr, "failed to connect to host %s; rc = %d\n", argv[3], rc);
        delete pm;
        return -3;
    }

    if( !strcmp(argv[5], "true") )
        pm->activateCongestionControl();
    int n;
    int buffsize = 1024;
    char buf[buffsize];

    memset(buf, 'a', buffsize); 

    MessageSender sender = pm->getSender (true, true);

    if( !strcmp(argv[6], "random") ) {
	srand(time(0));
    	if(seconds) {
    	    signal( SIGALRM, stopSending );
    	    alarm( k_or_s );
    	    while( keepSending ) 
    	        sender.send (buf, (rand() % buffsize) + 1);
    	}
    	else {
    	    for(n=0; n<k_or_s; n++) 
    	        sender.send (buf, (rand() % buffsize) + 1);
    	}
    }
    else if( !strcmp(argv[6], "fixed") ) {
    	if(seconds) {
    	    signal( SIGALRM, stopSending );
    	    alarm( k_or_s );
    	    while( keepSending ) 
    	        sender.send (buf, buffsize);
    	}
    	else {
    	    for(n=0; n<k_or_s; n++) 
    	        sender.send (buf, buffsize);
	}
    }
    pm->close();

    printf ("Mocket statistics:\n");
    printf ("    Packets sent: %d\n", pm->getStatistics()->getSentPacketCount());
    printf ("    Bytes sent: %d\n", pm->getStatistics()->getSentByteCount());
    printf ("    Packets received: %d\n", pm->getStatistics()->getReceivedPacketCount());
    printf ("    Bytes received: %d\n", pm->getStatistics()->getReceivedByteCount());
    printf ("    Retransmits: %d\n", pm->getStatistics()->getRetransmitCount());
    //printf ("    Packets discarded - below window threshold: %d\n", pm->getStatistics()->getDiscardedPacketCounts()._iBelowWindow);
    //printf ("    Packets discarded - no room: %d\n", pm->getStatistics()->getDiscardedPacketCounts()._iNoRoom);
    //printf ("    Packets discarded - overlap: %d\n", pm->getStatistics()->getDiscardedPacketCounts()._iOverlap);

    delete pm;

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

void stopSending ( int status )
{
    keepSending = false;
}

