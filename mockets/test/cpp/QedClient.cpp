/*  QedClient.cpp
 *
 * This test is meant to run together with QedServer.cpp
 *
 * The constants can be changed to adapt to the single test we want to run.
 *
 * This test has a sender thread and a producer thread. The producer produces a message of
 * MSG_DIMENSION bytes every MSG_FREQUENCY milliseconds, and the sender sends the messages as quickly as possibile.
 *
 * We have a producer that sends periodic updates to a specific destination over a low bandwidth channel.
 * Updates are a reliable, sequenced stream of data. 
 * Due to the limited link bandwidth the messages will get stuck in the network queues because the system produce
 * more messages then it is able to send out. This cause the packets to be delayed since they have to wait in the
 * queues, or if the queues are full they are deleted and they have to be retransmitted (due to the reliable service).
 * In this test we can appreciate the replacement function of Mockets that is able to delete the packet that has not
 * yet been sent with a newer update as soon as this is produced. This function allows to reduce the number of packets
 * injected in the congestioned network and allows the receiver to get the newer position update instead of receiving
 * all the updates with delay.
 *
 */

#include "Mocket.h"
#include "MessageSender.h"

#include "NLFLib.h"
#include "Queue.h"
#include "ConditionVariable.h"
#include "Mutex.h"
#include "Thread.h"

#define MSG_FREQUENCY 100
#define MSG_DIMENSION 1024
#define RELIABLE true
#define SEQUENCED true

using namespace NOMADSUtil;

bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec);

/**************** MsgPacket class **************/
class MsgPacket
{
    public:
        MsgPacket (void);
        MsgPacket (char *pBuf, unsigned short usBufSize, bool bDeleteWhenDone);
        ~MsgPacket (void);
        const char * getPacket (void);
        unsigned short getSize (void);
    private:
        char *_pBuf;
        bool _bDeleteBuf; 
        unsigned short _usBufSize;
};

MsgPacket::MsgPacket (void)
{
    _usBufSize = 0;
    _pBuf = NULL;
}

MsgPacket::MsgPacket (char *pBuf, unsigned short usBufSize, bool bDeleteWhenDone)
{
    _pBuf = pBuf;
    _usBufSize = usBufSize;
    _bDeleteBuf = bDeleteWhenDone;
}

MsgPacket::~MsgPacket (void)
{
    //printf ("calling the destructor of MsgPacket!!!");
    if (_bDeleteBuf) {
        free (_pBuf);
        _pBuf = NULL;
    }
}

const char * MsgPacket::getPacket (void)
{
    return _pBuf;
}

unsigned short MsgPacket::getSize (void)
{
    return _usBufSize;
}
/*******************************************/

/************** Producer thread *************/
class Producer : public Thread
{
    public:
        Producer (int iterations);
        void run (void);
        Queue<MsgPacket>* getQueue (void);
        //PtrQueue<MsgPacket>* getQueue (void);
    private:
        Queue<MsgPacket> *_pPacketQueue;
        //PtrQueue<MsgPacket> *_pPacketQueue;
        int _iterations;
};

Producer::Producer (int iterations)
{
    _iterations = iterations;
    // Create the queue
    _pPacketQueue = new Queue<MsgPacket> ();
}

Queue<MsgPacket>* Producer::getQueue (void)
{
    return _pPacketQueue;
}

void Producer::run (void)
{
    int i = 0;
    char * buf = NULL;
    while (i<_iterations) {
        printf("Producer: iteration %d\n", i);
        buf = (char*) malloc(MSG_DIMENSION);
        // Insert a sequence number and current time in the message
        ((int*)buf)[0] = i;
        ((int64*)buf)[4] = getTimeInMilliseconds();
        // Create the packet
        //MsgPacket *pPacket = new MsgPacket (buf, MSG_DIMENSION);
        MsgPacket pPacket (buf, MSG_DIMENSION, false);
/*        // TEST: Extract from the packet to check if the msgId has been inserted correctly
        int32 i32msgId = ((int32*)pPacket.getPacket())[0];
        printf ("Extract msgId %d\n", i32msgId);
*/
        // Insert the message in the queue
        _pPacketQueue->enqueue(pPacket);
/*        // TEST: get size of the queue:
        printf ("Size of the queue: %d\n", _pPacketQueue->size());
*/        
        i++;
        sleepForMilliseconds(MSG_FREQUENCY);
    }
    printf ("Producer thread done producing!\n");
}
/*******************************************/

int main (int argc, char *argv[])
{
    if (argc != 4) {
        fprintf (stderr, "usage: %s <port> <remotehost> [<iterations>]\n", argv[0]);
        return -1;
    }
    
    unsigned short usRemotePort = atoi (argv[1]);
    unsigned short usIterations;
    if (argc == 4) {
        usIterations = atoi (argv[3]);
    }
    else {
        usIterations = 20;
    }
    const char *pszRemoteHost = argv[2];
    printf ("Client Creating a Mocket to host %s on port %d\n", pszRemoteHost, (int) usRemotePort);
    
    Mocket *pMocket = new Mocket();
    pMocket->registerPeerUnreachableWarningCallback (unreachablePeerCallback, NULL);

    int rc;
    if (0 != (rc = pMocket->connect (pszRemoteHost, usRemotePort))) {
        fprintf (stderr, "Failed to connect using Mockets to remote host %s on port %d; rc = %d\n",
                 pszRemoteHost, usRemotePort, rc);
        printf ("doClientTask: Unable to connect\n");
        delete pMocket;
        return -1;
    }
    
    // Reliable sequenced service
    MessageSender sender = pMocket->getSender (RELIABLE, SEQUENCED);
    // Create the producer thread
    Producer *pProducer = new Producer ((int)usIterations);
    pProducer->start();
    // Get a reference to the queue to extract messages once they have been produced
    Queue<MsgPacket> *pPacketQueue = pProducer->getQueue();
/*    // TEST:
    printf ("Size of the queue main thread: %d\n", pPacketQueue->size());
*/ 
    // TODO: Create mutex and condition variable to lock the queue
    
    MsgPacket *pPacket = new MsgPacket();
    int res;
    // Send loop
    while (true) {
        // Extract message from the queue
        res = pPacketQueue->dequeue(*pPacket);
        if (res == 0) {
            //TEST: 
            int32 i32msgId = ((int32*)pPacket->getPacket())[0];
            printf ("Extract msgId %d\n", i32msgId);
            // Send using the replace functionality
            sender.replace (pPacket->getPacket(), pPacket->getSize(), 1, 1);
        }
    }
    
    printf("Closing the mocket\n");
    pMocket->close();
    //!!// I need this sleep otherwise the program ends!! Why?
    sleepForMilliseconds(10000);

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

