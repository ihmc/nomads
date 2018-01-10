#ifndef INCL_WIN32_SERVICE_H
#define INCL_WIN32_SERVICE_H
#define NOMINMAX
#include <winsock2.h>
#include <windows.h>

#include "StrClass.h"
#include "Thread.h"

/*
 * The Win32Service class provides an abstraction to implement a service on Win32 platforms.
 *
 * Applications that wish to run as services should implement a subclass of this class and
 * override the necessary member functions.
 *
 * Functions that must be overridden are declared as pure virtual functions. These are
 * invoked by the Win32Service class as necessary on the application subclass.
 * The functions are:
 *     getServiceName (void)
 *     getServiceDescription (void)
 *     beforeStart (int argc, char *argv[])
 *     stop (void)
 *     run (void)
 *
 * Functions that are optional to override are declared as virtual functions. These are also
 * invoked by the Win32Service class as necessary on the application subclass. However, default
 * implementations for the functions imply that the subclass does not have to override them
 * except to change the default behavior (which is usually to do nothing).
 * The functions are:
 *     getLogFileName (const char *pszHomeDir)
 *     beforeInstall (void)
 *     afterInstall (void)
 *     beforeRemove (void)
 *     afterRemove (void)
 *
 * Some functions are used to provide a capability to the subclass and may be invoked as
 * necessary by the application code. The functions are:
 *     getHomeDirectory (void)
 *     setServiceStatus (DWORD dwStatus)
 */
namespace NOMADSUtil
{

    class Win32Service : public Thread
    {
        public:
            Win32Service (void);
            virtual ~Win32Service (void);

            // Functions that are invoked by the Win32Service class on the application subclass

            virtual const char * getServiceName (void) = 0;
            virtual const char * getServiceDescription (void) = 0;

            virtual const char * getLogFilePath (void);

            virtual int beforeInstall (void);
            virtual int afterInstall (void);

            virtual int beforeRemove (void);
            virtual int afterRemove (void);

            virtual int beforeStart (int argc, char *argv[]) = 0;

            virtual int stop (void) = 0;

            // Functions that may invoked by the application subclass on the Win32Service class

            enum ServiceStatus {
                ServiceStopped = SERVICE_STOPPED,
                ServiceStartPending = SERVICE_START_PENDING,
                ServiceStopPending = SERVICE_STOP_PENDING,
                ServiceRunning = SERVICE_RUNNING,
                ServiceContinuePending = SERVICE_CONTINUE_PENDING,
                ServicePausePending = SERVICE_PAUSE_PENDING,
                ServicePaused = SERVICE_PAUSED
            };

            // Returns the home directory for the service
            // NOTE: This is the directory where the executable for the service is located
            const char * getProgHomeDir (void);

            int setServiceStatus (DWORD dwStatus);

        public:
            // Some utility functions
            static int installService (const char *pszServiceName, const char *pszServiceDesc, const char *pszPathToSvcExe);
            static int removeService (const char *pszServiceName);
            static int startService (const char *pszServiceName);
            static int stopService (const char *pszServiceName);

        public:
            // These functions should not be called by any user code
            int setProgHomeDir (const char *pszProgHomeDir);
            int runService (void);
            static void WINAPI callServiceMain (DWORD dwArgc, LPTSTR* lpszArgv);
            static void WINAPI callServiceCtrlHandler (DWORD dwControl);

        private:
            void serviceMain (int argc, char *argv[]);
            void serviceCtrlHandler (DWORD dwControl);

        private:
            static Win32Service *_pThis;
            SERVICE_STATUS _status;
            SERVICE_STATUS_HANDLE _statusHandle;
            String _progHomeDir;
    };

    Win32Service * createInstance (void);

    int main (int argc, char *argv[]);

}

#endif   // #ifndef INCL_WIN32_SERVICE_H
