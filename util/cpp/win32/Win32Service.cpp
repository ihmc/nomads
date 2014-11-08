#include "Win32Service.h"

#include "Logger.h"

using namespace NOMADSUtil;

Win32Service * Win32Service::_pThis;

Logger logger;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

#define stricmp _stricmp

int main (int argc, char *argv[])
{
    // Initialize the logger
    pLogger = &logger;
    pLogger->enableScreenOutput();
    pLogger->setDebugLevel (Logger::L_MediumDetailDebug);
    pLogger->disableFileOutput();
    pLogger->disableOSLogOutput();

    // Instantiate the service class
    Win32Service *pService = createInstance();
    if (pService == NULL) {
        checkAndLogMsg ("main", Logger::L_SevereError,
                        "failed to get instance of service class\n");
        return -1;
    }

    // Compute and set the home directory for the program
    pService->setProgHomeDir (getProgHomeDir (argv[0]));

    // Initialize file logging if required
    const char *pszLogFilePath = pService->getLogFilePath();
    if (pszLogFilePath) {
        if (pLogger->initLogFile (pszLogFilePath, true)) {
            checkAndLogMsg ("main", Logger::L_MildError,
                            "failed to initialize logging to file <%s>\n", pszLogFilePath);
        }
        else {
            pLogger->enableFileOutput();
        }
    }

    if (argc >= 2) {
        if (0 == stricmp (argv[1], "-install")) {
            // Installs this service
            int rc;
            rc = pService->beforeInstall();
            if (rc != 0) {
                checkAndLogMsg ("main", Logger::L_SevereError,
                                "beforeInstall() failed on the service with rc = %d\n", rc);
                return -2;
            }
            rc = pService->installService (pService->getServiceName(),
                                           pService->getServiceDescription(),
                                           getProgPath(argv[0]));
            if (rc != 0) {
                checkAndLogMsg ("main", Logger::L_SevereError,
                                "installService() failed with rc = %d\n", rc);
                return -3;
            }
            rc = pService->afterInstall();
            if (rc != 0) {
                checkAndLogMsg ("main", Logger::L_SevereError,
                                "afterInstall() failed on the service with rc = %d\n", rc);
                return -4;
            }

			return 0;
        }
        else if ((0 == stricmp (argv[1], "-remove")) || (0 == stricmp (argv[1], "-uninstall"))) {
            // Removes this service
            int rc;
            rc = pService->beforeRemove();
            if (rc != 0) {
                checkAndLogMsg ("main", Logger::L_SevereError,
                                "beforeRemove() failed on the service with rc = %d\n", rc);
                return -5;
            }
            rc = pService->removeService (pService->getServiceName());
            if (rc != 0) {
                checkAndLogMsg ("main", Logger::L_SevereError,
                                "removeService() failed with rc = %d\n", rc);
                return -6;
            }
            rc = pService->afterRemove();
            if (rc != 0) {
                checkAndLogMsg ("main", Logger::L_SevereError,
                                "afterRemove() failed on the service with rc = %d\n", rc);
                return -7;
            }

            return 0;
        }
        else if (0 == stricmp (argv[1], "-start")) {
            // Start up the service
            int rc;
            rc = pService->startService (pService->getServiceName());
            if (rc != 0) {
                checkAndLogMsg ("main", Logger::L_SevereError,
                                "startService() failed with rc = %d\n", rc);
                return -8;
            }
            return 0;
        }
        else if (0 == stricmp (argv[1], "-stop")) {
            // Stop the service
            int rc;
            rc = pService->stopService (pService->getServiceName());
            if (rc != 0) {
                checkAndLogMsg ("main", Logger::L_SevereError,
                                "stopService() failed with rc = %d\n", rc);
                return -9;
            }
            return 0;
        }
        else if (0 == stricmp (argv[1], "-run")) {
            // Just runs the process directly - not as a service
            checkAndLogMsg ("main", Logger::L_Warning,
                            "********** Starting a new Session (Not As A Service) **********\n");

            int rc;
            char **apszNewArgv = new char* [argc-1];
            apszNewArgv[0] = argv[0];
            for (int i = 2; i < argc; i++) {
                apszNewArgv[i-1] = argv[i];
            }
            rc = pService->beforeStart (argc-1, apszNewArgv);
            delete[] apszNewArgv;
            if (rc != 0) {
                checkAndLogMsg ("main", Logger::L_SevereError,
                                "beforeStart() failed on the service with rc = %d\n", rc);
                return -10;
            }
            pService->run();
            return 0;
        }
    }
    else if (argc == 1) {
        // Assume that the process has been invoked by the Service Control Manager
        // Otherwise, the call to runService below will fail
        int rc;
        pLogger->setDebugLevel (Logger::L_LowDetailDebug);
        pLogger->disableScreenOutput();
        pLogger->enableOSLogOutput (pService->getServiceName(), Logger::L_LowDetailDebug);

        checkAndLogMsg ("main", Logger::L_Warning,
                        "********** Starting a new Session **********\n");

        rc = pService->runService();

        if (rc != 0) {
            if (pLogger) {
                pLogger->enableScreenOutput();
                pLogger->disableOSLogOutput();
                pLogger->logMsg ("main", Logger::L_SevereError, "failed to start service; rc = %d\n", rc);
                pLogger->logMsg ("main", Logger::L_SevereError, "usage for executing directly:\n");
                pLogger->logMsg ("main", Logger::L_SevereError, "    %s -install\n", argv[0]);
                pLogger->logMsg ("main", Logger::L_SevereError, "    %s -remove|-uninstall\n", argv[0]);
                pLogger->logMsg ("main", Logger::L_SevereError, "    %s -run [args]\n", argv[0]);
                pLogger->logMsg ("main", Logger::L_SevereError, "    %s -start\n", argv[0]);
                pLogger->logMsg ("main", Logger::L_SevereError, "    %s -stop\n", argv[0]);
            }
            return -11;
        }
        return 0;
    }
    checkAndLogMsg ("main", Logger::L_SevereError, "usage:\n");
    checkAndLogMsg ("main", Logger::L_SevereError, "    %s -install\n", argv[0]);
    checkAndLogMsg ("main", Logger::L_SevereError, "    %s -remove|-uninstall\n", argv[0]);
    checkAndLogMsg ("main", Logger::L_SevereError, "    %s -run [args]\n", argv[0]);
    checkAndLogMsg ("main", Logger::L_SevereError, "    %s -start\n", argv[0]);
    checkAndLogMsg ("main", Logger::L_SevereError, "    %s -stop\n", argv[0]);
    return -12;
}

Win32Service::Win32Service (void)
{
    _statusHandle = NULL;
}

Win32Service::~Win32Service (void)
{
}

const char * Win32Service::getLogFilePath (void)
{
    return NULL;
}

int Win32Service::beforeInstall (void)
{
    return 0;
}

int Win32Service::afterInstall (void)
{
    return 0;
}

int Win32Service::beforeRemove (void)
{
    return 0;
}

int Win32Service::afterRemove (void)
{
    return 0;
}

const char * Win32Service::getProgHomeDir (void)
{
    return _progHomeDir;
}

int Win32Service::setServiceStatus (DWORD dwStatus)
{
    if (_statusHandle == NULL) {
        return -1;
    }
    _status.dwCurrentState = dwStatus;
    if (!SetServiceStatus (_statusHandle, &_status)) {
        checkAndLogMsg ("Win32Service::setServiceStatus", Logger::L_SevereError,
                         "failed to set service status: %s\n", getLastOSErrorAsString());
        return -2;
    }
    return 0;
}

int Win32Service::installService (const char *pszServiceName, const char *pszServiceDesc, const char *pszPathToSvcExe)
{
    HKEY hKey;
    char szServiceKeyPath[] = "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\";
    char *pszServiceKey = new char [strlen (szServiceKeyPath) + strlen (pszServiceName) + 1];
    strcpy (pszServiceKey, szServiceKeyPath);
    strcat (pszServiceKey, pszServiceName);
    if (RegCreateKey (HKEY_LOCAL_MACHINE, pszServiceKey, &hKey)) {
        checkAndLogMsg ("Win32Service::installService", Logger::L_SevereError,
                        "failed to make event log registry key: %s\n", getLastOSErrorAsString());
        delete[] pszServiceKey;
        return -1;
    }
    delete[] pszServiceKey;

    if (RegSetValueEx (hKey,
                       "EventMessageFile",
                       0,
                       REG_EXPAND_SZ,
                       (LPBYTE) pszPathToSvcExe,
                       (DWORD) strlen(pszPathToSvcExe) + 1)) {
        checkAndLogMsg ("Win32Service::installService", Logger::L_SevereError,
                        "failed to set event log registry key: %s\n", getLastOSErrorAsString());
        RegCloseKey (hKey);
        return -2;
    }

    DWORD dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;

    if (RegSetValueEx (hKey,
                       "TypesSupported",
                       0,
                       REG_DWORD,
                       (LPBYTE) &dwData,
                       sizeof(DWORD))) {
        checkAndLogMsg ("Win32Service::installService", Logger::L_SevereError,
                        "failed to set event log registry key 2: %s\n", getLastOSErrorAsString());
        RegCloseKey (hKey);
        return -3;
    }
    RegCloseKey (hKey);

    SC_HANDLE schManager = OpenSCManager (NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (schManager == NULL) {
        checkAndLogMsg ("Win32Service::installService", Logger::L_SevereError,
                        "failed to open service control manager: %s\n", getLastOSErrorAsString());
        return -4;
    }

    char szProgPath[MAX_PATH];
    strcpy (szProgPath, "\"");
    strcat (szProgPath, pszPathToSvcExe);
    strcat (szProgPath, "\"");
    SC_HANDLE schService = CreateService (schManager,
                                          pszServiceName,
                                          pszServiceDesc,
                                          SC_MANAGER_CREATE_SERVICE,
                                          SERVICE_WIN32_OWN_PROCESS,
                                          SERVICE_AUTO_START,
                                          SERVICE_ERROR_NORMAL,
                                          szProgPath,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL);
    if (schService == NULL) {
        checkAndLogMsg ("Win32Service::installService", Logger::L_SevereError,
                        "failed to create service: %s\n", getLastOSErrorAsString());
        CloseHandle (schManager);
        return -5;
    }
    CloseHandle (schService);
    CloseHandle (schManager);

    checkAndLogMsg ("Win32Service::installService", Logger::L_LowDetailDebug,
                    "successfully installed service %s\n", pszServiceName);

    return 0;
}

int Win32Service::removeService (const char *pszServiceName)
{
    SC_HANDLE schManager = OpenSCManager (NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (schManager == NULL) {
        checkAndLogMsg ("Win32Service::removeService", Logger::L_SevereError,
                        "failed to open service control manager: %s\n", getLastOSErrorAsString());
        return -1;
    }

    SC_HANDLE schService = OpenService (schManager, pszServiceName, SERVICE_ALL_ACCESS);
    if (schService == NULL) {
        checkAndLogMsg ("Win32Service::removeService", Logger::L_SevereError,
                        "failed to open service %s: %s\n",
                        pszServiceName, getLastOSErrorAsString());
        CloseHandle (schManager);
        return -2;
    }
    if (!DeleteService (schService)) {
        checkAndLogMsg ("Win32Service::removeService", Logger::L_SevereError,
                        "failed to delete service %s: %s\n",
                        pszServiceName, getLastOSErrorAsString());
        CloseHandle (schService);
        CloseHandle (schManager);
        return -3;
    }
    CloseHandle (schService);
    CloseHandle (schManager);

    char szServiceKeyPath[] = "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\";
    char *pszServiceKey = new char [strlen (szServiceKeyPath) + strlen (pszServiceName) + 1];
    strcpy (pszServiceKey, szServiceKeyPath);
    strcat (pszServiceKey, pszServiceName);
    if (RegDeleteKey (HKEY_LOCAL_MACHINE, pszServiceKey)) {
        checkAndLogMsg ("Win32Service::removeService", Logger::L_SevereError,
                        "failed to delete event log registry key: %s\n", getLastOSErrorAsString());
        delete[] pszServiceKey;
        return -4;
    }
    delete[] pszServiceKey;

    checkAndLogMsg ("Win32Service::removeService", Logger::L_LowDetailDebug,
                    "successfully removed service %s\n", pszServiceName);

    return 0;
}

int Win32Service::startService (const char *pszServiceName)
{
    SC_HANDLE schSCManager = OpenSCManager (NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (schSCManager == 0) {
        checkAndLogMsg ("Win32Service::startService", Logger::L_MildError,
                        "OpenSCManager failed: %s\n", getLastOSErrorAsString());
        return -1;
    }

    SC_HANDLE schService = OpenService (schSCManager, pszServiceName, SERVICE_ALL_ACCESS);
    if (schService == 0) {
        checkAndLogMsg ("Win32Service::startService", Logger::L_MildError,
                        "OpenService failed: %s\n", getLastOSErrorAsString());
        CloseServiceHandle (schSCManager);
        return -2;
    }

    if (!StartService (schService, 0, NULL)) {
        checkAndLogMsg ("Win32Service::startService", Logger::L_MildError,
                        "StartService failed; %s\n", getLastOSErrorAsString());
        CloseServiceHandle (schService);
        CloseServiceHandle (schSCManager);
        return -3;
    }

    CloseServiceHandle (schService); 
    CloseServiceHandle (schSCManager);

    checkAndLogMsg ("Win32Service::startService", Logger::L_LowDetailDebug,
                    "successfully started service %s\n", pszServiceName);

    return 0;
}

int Win32Service::stopService (const char *pszServiceName)
{
    SC_HANDLE schSCManager = OpenSCManager (NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (schSCManager == 0) {
        checkAndLogMsg ("Win32Service::stopService", Logger::L_MildError,
                        "OpenSCManager failed: %s\n", getLastOSErrorAsString());
        return -1;
    }

    SC_HANDLE schService = OpenService (schSCManager, pszServiceName, SERVICE_ALL_ACCESS);
    if (schService == 0) {
        checkAndLogMsg ("Win32Service::stopService", Logger::L_MildError,
                        "OpenService failed: %s\n", getLastOSErrorAsString());
        CloseServiceHandle (schSCManager);
        return -2;
    }

    SERVICE_STATUS status;
    if (!ControlService (schService, SERVICE_CONTROL_STOP, &status)) {
        checkAndLogMsg ("Win32Service::stopService", Logger::L_MildError,
                        "ControlService failed: %s\n", getLastOSErrorAsString());
        CloseServiceHandle (schService); 
        CloseServiceHandle (schSCManager);
        return -3;
    }

    CloseServiceHandle (schService); 
    CloseServiceHandle (schSCManager);

    checkAndLogMsg ("Win32Service::startService", Logger::L_LowDetailDebug,
                    "successfully stopped service %s\n", pszServiceName);

    return 0;
}

int Win32Service::setProgHomeDir (const char *pszProgHomeDir)
{
    _progHomeDir = pszProgHomeDir;
    return 0;
}

int Win32Service::runService (void)
{
    _pThis = this;
    SERVICE_TABLE_ENTRY aste[] = {(char*)getServiceName(), Win32Service::callServiceMain, NULL, NULL};
    if (!StartServiceCtrlDispatcher (aste)) {
        checkAndLogMsg ("runService", Logger::L_SevereError,
                        "StartServiceCtrlDispatcher failed: %s\n", getLastOSErrorAsString());
        return -1;
    }

    checkAndLogMsg ("Win32Service::runService", Logger::L_Warning,
                    "service terminating\n");
    return 0;
}

void WINAPI Win32Service::callServiceMain (DWORD dwArgc, LPTSTR* lpszArgv)
{
    _pThis->serviceMain (dwArgc, lpszArgv);
}

void Win32Service::serviceMain (int argc, char *argv[])
{
    _statusHandle = RegisterServiceCtrlHandler (getServiceName(), callServiceCtrlHandler);
    if (_statusHandle == NULL) {
        checkAndLogMsg ("Win32Service::serviceMain", Logger::L_SevereError,
                        "failed to register service control handler: %s\n", getLastOSErrorAsString());
        return;
    }

    // Initialize the SERVICE_STATUS struct
    _status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    _status.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_STOP;
    _status.dwWin32ExitCode = 0;
    _status.dwServiceSpecificExitCode = 0;
    _status.dwCheckPoint = 0;
    _status.dwWaitHint = 0;

    checkAndLogMsg ("Win32Service::serviceMain", Logger::L_LowDetailDebug,
                    "service starting\n");
    setServiceStatus (SERVICE_START_PENDING);

    int rc = beforeStart (argc, argv);
    if (rc < 0) {
        checkAndLogMsg ("Win32Service::serviceMain", Logger::L_SevereError,
                        "beforeStart() failed on the service; rc = %d\n", rc);
        setServiceStatus (SERVICE_STOPPED);
    }

    setServiceStatus (SERVICE_START_PENDING);

    start();
}

void WINAPI Win32Service::callServiceCtrlHandler (DWORD dwControl)
{
    _pThis->serviceCtrlHandler (dwControl);
}

void Win32Service::serviceCtrlHandler (DWORD dwControl)
{
    int rc;
    switch (dwControl) {
        case SERVICE_CONTROL_INTERROGATE:
            checkAndLogMsg ("Win32Service::serviceCtrlHandler", Logger::L_MediumDetailDebug,
                            "serviceCtrlHandler invoked with param INTERROGATE\n");
            break;
        case SERVICE_CONTROL_PARAMCHANGE:
            checkAndLogMsg ("Win32Service::serviceCtrlHandler", Logger::L_MediumDetailDebug,
                            "serviceCtrlHandler invoked with param PARAMCHANGE\n");
            break;
        case SERVICE_CONTROL_PAUSE:
            checkAndLogMsg ("Win32Service::serviceCtrlHandler", Logger::L_MediumDetailDebug,
                            "serviceCtrlHandler invoked with param PAUSE\n");
            break;
        case SERVICE_CONTROL_CONTINUE:
            checkAndLogMsg ("Win32Service::serviceCtrlHandler", Logger::L_MediumDetailDebug,
                            "serviceCtrlHandler invoked with param CONTINUE\n");
            break;
        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            checkAndLogMsg ("Win32Service::serviceCtrlHandler", Logger::L_MediumDetailDebug,
                            "serviceCtrlHandler invoked with param STOP or SHUTDOWN\n");
            if (0 != (rc = stop())) {
                checkAndLogMsg ("Win32Service::serviceCtrlHandler", Logger::L_MildError,
                                "stop() on the service failed with rc = %d\n", rc);
            }
            break;
        default:
            checkAndLogMsg ("Win32Service::serviceCtrlHandler", Logger::L_LowDetailDebug,
                            "serviceCtrlHandler invoked with param = %d\n", dwControl);
            break;
    }
}
