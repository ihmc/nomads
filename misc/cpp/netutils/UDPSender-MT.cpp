#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ConditionVariable.h"
#include "FTypes.h"
#include "InetAddr.h"
#include "Mutex.h"
#include "NLFLib.h"
#include "Thread.h"
#include "UDPDatagramSocket.h"

using namespace NOMADSUtil;

NOMADSUtil::Mutex _m;
NOMADSUtil::ConditionVariable _cv (&_m);

class SenderThread : public Thread
{
    public:
        SenderThread (UDPDatagramSocket *pudps, uint32 ui32DestIP, uint16 ui16DestPort, uint8 *pui8Buf, uint16 ui16PacketLength, uint8 ui8id);
        void run (void);
    private:
        UDPDatagramSocket *_pSenderUDPs;
        uint32 _ui32DestIP;
        uint16 _ui16DestPort;
        uint8 *_pui8Buf;
        uint16 _ui16PacketLength;
        uint8 _ui8id;
};

SenderThread::SenderThread (UDPDatagramSocket *pudps, uint32 ui32DestIP, uint16 ui16DestPort, uint8 *pui8Buf, uint16 ui16PacketLength, uint8 ui8id)
{
    _pSenderUDPs = pudps;
    _ui32DestIP = ui32DestIP;
    _ui16DestPort = ui16DestPort;
    _pui8Buf = pui8Buf;
    _ui16PacketLength = ui16PacketLength;
    _ui8id = ui8id;
}

void SenderThread::run (void)
{
    //printf ("start Sender %d\n", _ui8id);
    int rc = 0;
    while (true) {
       _m.lock(); 
       if ((rc = _pSenderUDPs->sendTo (_ui32DestIP, _ui16DestPort, _pui8Buf, _ui16PacketLength)) != _ui16PacketLength) {
            fprintf (stderr, "error: UDPDatagramSocket::sendTo() failed with rc = %d\n", rc);
            free (_pui8Buf);
            return;
        }
        //printf("Sender %d\n", _ui8id);
        //sleepForMilliseconds(1000);
        _cv.notifyAll();
        _cv.wait();
        _m.unlock();
    }
}

int main (int argc, const char *argv[])
{
    if (argc < 3) {
        fprintf (stderr, "usage: %s [-len <packet length>] [-srcIP <SourceIPAddr>] [-srcPort <SourcePort>] <DestIPAddr> <DestPort>\n",
                 argv[0]);
        return -1;
    }
    // Assign default values and read mandatory input values
    const char *pszPacketLength = "1024";
    const char *pszSourceIP = NULL;
    const char *pszSourcePort = NULL;
    const char *pszDestIP = argv[argc-2];
    const char *pszDestPort = argv[argc-1];

    // Parse input values
    int i = 1;
    if (0 == strcmp (argv[i], "-len")) {
        pszPacketLength = argv[++i];
        i++;
    }
    if (0 == strcmp (argv[i], "-int")) {
        i++;
    }
    if (0 == strcmp (argv[i], "-srcIP")) {
        pszSourceIP = argv[++i];
        i++;
    }
    if (0 == strcmp (argv[i], "-srcPort")) {
        pszSourcePort = argv[++i];
        i++;
    }

    printf ("Packet length set to %s\n", pszPacketLength);
    if (pszSourceIP) {
        printf ("SourceIPAddr set to %s\n", pszSourceIP);
    }
    if (pszSourcePort) {
        printf ("SourcePort set to %s\n", pszSourcePort);
    }

    UDPDatagramSocket *pudps = new UDPDatagramSocket();

    uint32 ui32SourceIP = 0;
    if (pszSourceIP) {
        ui32SourceIP = InetAddr(pszSourceIP).getIPAddress();
    }
    uint16 ui16SourcePort = 0;
    if (pszSourcePort) {
        ui16SourcePort = (uint16) atoi (pszSourcePort);
    }

    int rc;
    if (0 != (rc = pudps->init (ui16SourcePort, ui32SourceIP))) {
        fprintf (stderr, "error: could not bind UDP socket to <%s>:<%d>; rc = %d\n",
                 InetAddr(ui32SourceIP).getIPAsString(), (int) ui16SourcePort, rc);
        return -2;
    }

    uint16 ui16PacketLength = (uint16) atoi (pszPacketLength);
    uint8 *pui8Buf = (uint8*) malloc (ui16PacketLength);
    
    for (uint16 ui16 = 0; ui16 < ui16PacketLength; ui16++) {
        pui8Buf[ui16] = (uint8) (ui16 % 255);
    }

    uint32 ui32DestIP = InetAddr(pszDestIP).getIPAddress();
    uint16 ui16DestPort = (uint16) atoi (pszDestPort);
    
    SenderThread *pFirstSender = new SenderThread (pudps, ui32DestIP, ui16DestPort, pui8Buf, ui16PacketLength, (uint8) 1);
    pFirstSender->start();
    
    SenderThread *pSecondSender = new SenderThread (pudps, ui32DestIP, ui16DestPort, pui8Buf, ui16PacketLength, (uint8) 2);
    pSecondSender->run();
    
    
    free (pui8Buf);
    
    return 0;
}
