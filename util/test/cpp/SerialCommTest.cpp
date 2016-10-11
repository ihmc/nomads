#include "Serial.h"

#include "Logger.h"
#include "Thread.h"

#include <conio.h>
#include <stdio.h>

using namespace NOMADSUtil;

class KBWriter : public Thread
{
    public:
        KBWriter (Serial *pSerial);
        void run (void);

    private:
        Serial *_pSerial;
};

int main (int argc, char *argv[])
{
    int rc;

    pLogger = new Logger();
    pLogger->initLogFile ("SerialCommTest.log", false);
    pLogger->enableScreenOutput();
    pLogger->enableFileOutput();
    pLogger->setDebugLevel (Logger::L_MediumDetailDebug);

    Serial serial;

    if (0 != (rc = serial.init ("COM2", 9600, 'N', 8, 1))) {
        pLogger->logMsg ("main", Logger::L_SevereError,
                         "Serial::init failed with rc = %d\n", rc);
        return -1;
    }

    KBWriter writer (&serial);
    writer.start();

    //char ch;
    //while ((rc = serial.read (&ch, 1)) == 1) {
    //    putchar (ch);
    //}
    while (true) {
        int bufSize = serial.getReceiveBufferSize();
        int bytesAvail = serial.getBytesAvailable();
        printf ("Bytes available = %d; buffer size = %d\n", bytesAvail, bufSize);
        sleepForMilliseconds (1000);
    }
    pLogger->logMsg ("main", Logger::L_Info,
                     "Serial::read returned %d; terminating\n", rc);
    return 0;
}

KBWriter::KBWriter (Serial *pSerial)
{
    _pSerial = pSerial;
}

void KBWriter::run (void)
{
    int rc;
    while (1) {
        int c = _getch();
        if (c <= 0) {
            pLogger->logMsg ("KBWriter::run", Logger::L_Info,
                             "getchar() returned <= 0 - terminating\n");
            exit (0);
        }
        char ch = (char) c;
        putchar (ch);
        if (1 != (rc = _pSerial->write (&ch, 1))) {
            pLogger->logMsg ("KBWriter::run", Logger::L_SevereError,
                             "Serial::write failed with rc = %d; terminating\n", rc);
            exit (-1);
        }
    }
}
