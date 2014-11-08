#ifndef INCL_SERVICE_HELPER_H
#define INCL_SERVICE_HELPER_H

#include <windows.h>

#include "StrClass.h"

namespace NOMADSUtil
{

    typedef int (*MainFnPtr) (int argc, char *argv[]);

    class ServiceHelper
    {
        public:
            static int startService (const char *pszServiceName, MainFnPtr pFn);
            static int installService (const char *pszServiceName, const char *pszServiceDesc, const char *pszPathToSvcExe);
            static int removeService (const char *pszServiceName);
            static int setServiceStatus (DWORD dwStatus);
            static bool stopService (const char *pszServiceName);
        protected:
            static void WINAPI serviceMain (DWORD dwArgc, LPTSTR* lpszArgv);
            static void WINAPI serviceCtrlHandler (DWORD fdwControl);
        protected:
            static MainFnPtr _pMainFn;
            static SERVICE_STATUS _status;
            static SERVICE_STATUS_HANDLE _statusHandle;
            static String _serviceName;
    };

}

#endif   // #ifndef INCL_SERVICE_HELPER_H
