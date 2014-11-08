#include "Logger.h"
#include "TapInterfaceTestAndConfiguration.h"
#include "StrClass.h"
#define stricmp _stricmp

using namespace NOMADSUtil;
using namespace ACMNetProxy;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

int main (int argc, char *argv[])
{
    pLogger = new Logger();
    pLogger->enableScreenOutput();      // any errors occurring before reading configuration file are logged to screen
    pLogger->setDebugLevel (Logger::L_Info);

    TapInterfaceTestAndConfiguration newTapTestObj;

    int rc;
    if (1 == argc) {
        checkAndLogMsg ("TAP INTERFACE TEST - ", Logger::L_MildError, "No input parameters\n");
        return -26;
    }

    if (0 == stricmp (argv[1], "-testtap")) {
        rc = newTapTestObj.runTapInterfaceTest();
    }
    else if (0 == stricmp (argv[1], "-updateIP")) {
        rc = newTapTestObj.updateIPAddress (argc, argv);
    }
    else if (0 == stricmp (argv[1], "-updateMac")) {
        rc = newTapTestObj.updateMacAddress (argc, argv);
    }
    else {
        checkAndLogMsg ("TAP INTERFACE TEST - ", Logger::L_MildError, "first parameter must be -testtap, -updateIP or -updateMac\n");
        rc = -25;
    }
    return rc;
}
