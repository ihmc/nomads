#include "Mocket.h"
#include "ServerMocket.h"

#include "Logger.h"
#include "Thread.h"

using namespace NOMADSUtil;

class Handler : public Thread
{
    public:
        Handler (Mocket *pm);
        void run (void);
    private:
        Mocket *_pm;
};

int main (int argc, char *argv[])
{
    pLogger = new Logger();
    pLogger->initLogFile ("TestServer.log", false);
    pLogger->enableFileOutput();
    pLogger->disableScreenOutput();
    pLogger->setDebugLevel (Logger::L_MediumDetailDebug);

    ServerMocket msm;
    msm.listen (1973);

    while (true) {
        Mocket *pm = msm.accept();
        printf ("received a connection - starting a handler\n");
        if (pm) {
            Handler *pHandler = new Handler (pm);
            pHandler->start();
        }
    }

    return 0;
}

Handler::Handler (Mocket *pm)
{
    _pm = pm;
}

void Handler::run (void)
{
    sleepForMilliseconds (5000);
    while (true) {
        char buf[1024];
        int rc = _pm->receive (buf, sizeof (buf));
        if (rc <= 0) {
            printf ("rc = %d; handler thread exiting\n", rc);
            break;
        }
        else {
            buf[rc] = '\0';
            printf ("received: <%s>\n", buf);
        }
    }
}
