/*  QedClientTest2.cpp
 *
 * This test is meant to run together with QedServerTest3.cpp
 *
 * The constants can be changed to adapt to the single test we want to run.
 *
 * We have position updates messages A and sensor information B. The sensor information
 * are not subject to the replacement, every single message is important. Sensor reports
 * are a reliable, unsequenced, high priority (higher then the GPS update) stream.
 *
 */
#include "Mocket.h"
#include "MessageSender.h"

#include "NLFLib.h"
#include "Queue.h"
#include "ConditionVariable.h"
#include "Mutex.h"
#include "Thread.h"

#define MSG_FREQUENCY_STREAM_A 50
#define MSG_DIMENSION_STREAM_A 1024
#define MSG_FREQUENCY_STREAM_B 1000
#define MSG_DIMENSION_STREAM_B 5120
#define RELIABLE true
#define SEQUENCED true
#define UNSEQUENCED false
#define TAG_SENSOR_A 1
#define TAG_SENSOR_B 2
#define PRIORITY_SENSOR_B 7

using namespace NOMADSUtil;

bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec);

/**************** MsgPacket class **************/
class MsgPacket
{
    public:
        MsgPacket (void);
        MsgPacket (char *pBuf, unsigned short usBufSize, int msgType, bool bDeleteWhenDone);
        ~MsgPacket (void);
        const char * getPacket (void);
        unsigned short getSize (void);
        int getMsgType (void);
    private:
        char *_pBuf;
        bool _bDeleteBuf; 
        unsigned short _usBufSize;
        int _msgType;
};

MsgPacket::MsgPacket (void)
{
    _usBufSize = 0;
    _pBuf = NULL;
    _msgType = 0;
}

MsgPacket::MsgPacket (char *pBuf, unsigned short usBufSize, int msgType, bool bDeleteWhenDone)
{
    _pBuf = pBuf;
    _usBufSize = usBufSize;
    _msgType = msgType;
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

int MsgPacket::getMsgType (void)
{
    return _msgType;
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
        // Create a message of type A
        buf = (char*) malloc(MSG_DIMENSION_STREAM_A);
        // Insert the ID of this type of msg, insert the msg ID insert timestamp
        ((int*)buf)[0] = TAG_SENSOR_A;
        ((int*)buf)[4] = i;
        ((int64*)buf)[8] = getTimeInMilliseconds();
        // Create the packet
        MsgPacket pPacket (buf, MSG_DIMENSION_STREAM_A, TAG_SENSOR_A, false);
        // Insert the message in the queue
        _pPacketQueue->enqueue(pPacket);
        
        i++;
        sleepForMilliseconds(MSG_FREQUENCY_STREAM_A);
    }
    printf ("Producer thread done producing!\n");
}
/*******************************************/

/************** Producer2 thread *************/
class Producer2 : public Thread
{
    public:
        Producer2 (int iterations, Queue<MsgPacket> *pQueue);
        void run (void);
    private:
        Queue<MsgPacket> *_pPacketQueue;
        int _iterations;
};

Producer2::Producer2 (int iterations, Queue<MsgPacket> *pQueue)
{
    _iterations = iterations;
    // Create the queue
    _pPacketQueue = pQueue;
}

void Producer2::run (void)
{
    int i = 0;
    char * buf = NULL;
    while (i<_iterations) {
        printf("Producer: iteration %d\n", i);
        // Create a message of type A
        buf = (char*) malloc(MSG_DIMENSION_STREAM_B);
        // Insert the ID of this type of msg, insert the msg ID insert timestamp
        ((int*)buf)[0] = TAG_SENSOR_B;
        ((int*)buf)[4] = i;
        ((int64*)buf)[8] = getTimeInMilliseconds();
        // Create the packet
        MsgPacket pPacket (buf, MSG_DIMENSION_STREAM_B, TAG_SENSOR_B, false);
        // Insert the message in the queue
        _pPacketQueue->enqueue(pPacket);
        
        i++;
        sleepForMilliseconds(MSG_FREQUENCY_STREAM_B);
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
    MessageSender senderA = pMocket->getSender (RELIABLE, SEQUENCED);
    // Reliable unsequenced service
    MessageSender senderB = pMocket->getSender (RELIABLE, UNSEQUENCED);
    // Create the producer A thread
    Producer *pProducerA = new Producer ((int)usIterations);
    // Get a reference to the queue to extract messages once they have been produced
    Queue<MsgPacket> *pPacketQueue = pProducerA->getQueue();
    // Create the producer B thread
    
    Producer2 *pProducerB = new Producer2 ((int)usIterations, pPacketQueue);
    //Start producer threads
    pProducerA->start();
    pProducerB->start();
    
 
    // TODO: Create mutex and condition variable to lock the queue
    
    MsgPacket *pPacket = new MsgPacket();
    int res;
    // Send loop
    while (true) {
        // Extract message from the queue
        res = pPacketQueue->dequeue(*pPacket);
        if (res == 0) {
            if (pPacket->getMsgType() == TAG_SENSOR_A) {
                // Send using the replace functionality for msg from sensor A
                senderA.replace (pPacket->getPacket(), pPacket->getSize(), TAG_SENSOR_A, TAG_SENSOR_A);
            }
            else {
                // Send using the replace functionality for msg from sensor B
                senderB.send (pPacket->getPacket(), pPacket->getSize(), TAG_SENSOR_B, PRIORITY_SENSOR_B);
            }
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

