#include "ServiceHelper.h"

#include "Logger.h"
#include "NLFLib.h"

using namespace NOMADSUtil;

MainFnPtr ServiceHelper::_pMainFn;
SERVICE_STATUS ServiceHelper::_status;
SERVICE_STATUS_HANDLE ServiceHelper::_statusHandle;
String ServiceHelper::_serviceName;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

int ServiceHelper::startService (const char *pszServiceName, MainFnPtr pFn)
{
    _serviceName = pszServiceName;
    _pMainFn = pFn;
    SERVICE_TABLE_ENTRY aste[] = {(char*)pszServiceName, ServiceHelper::serviceMain, NULL, NULL};
    if (!StartServiceCtrlDispatcher (aste)) {
        checkAndLogMsg ("ServiceHelper::startService", Logger::L_SevereError,
                        "StartServiceCtrlDispatcher failed: %s\n", getLastOSErrorAsString());
        return -1;
    }

    checkAndLogMsg ("ServiceHelper::StartService", Logger::L_Warning,
                    "service terminating\n");
    return 0;
}

void WINAPI ServiceHelper::serviceMain (DWORD dwArgc, LPTSTR* lpszArgv)
{
    _statusHandle = RegisterServiceCtrlHandler (_serviceName, serviceCtrlHandler);
    if (_statusHandle == NULL) {
        checkAndLogMsg ("ServiceHelper::serviceMain", Logger::L_SevereError,
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

    checkAndLogMsg ("ServiceHelper::serviceMain", Logger::L_LowDetailDebug,
                    "service starting\n");
    setServiceStatus (SERVICE_RUNNING);

    int rc = (*_pMainFn) (dwArgc, lpszArgv);
    if (rc < 0) {
        checkAndLogMsg ("ServiceHelper::serviceMain", Logger::L_SevereError,
                        "failed to start service\n");
        setServiceStatus (SERVICE_STOPPED);
    }
}

void WINAPI ServiceHelper::serviceCtrlHandler (DWORD fdwControl)
{
    switch (fdwControl) {
        case SERVICE_CONTROL_INTERROGATE:
            checkAndLogMsg ("ServiceHelper::serviceCtrlHandler", Logger::L_MediumDetailDebug,
                            "serviceCtrlHandler invoked with param INTERROGATE\n");
            break;
        case SERVICE_CONTROL_PARAMCHANGE:
            checkAndLogMsg ("ServiceHelper::serviceCtrlHandler", Logger::L_MediumDetailDebug,
                            "serviceCtrlHandler invoked with param PARAMCHANGE\n");
            break;
        case SERVICE_CONTROL_PAUSE:
            checkAndLogMsg ("ServiceHelper::serviceCtrlHandler", Logger::L_MediumDetailDebug,
                            "serviceCtrlHandler invoked with param PAUSE\n");
            break;
        case SERVICE_CONTROL_CONTINUE:
            checkAndLogMsg ("ServiceHelper::serviceCtrlHandler", Logger::L_MediumDetailDebug,
                            "serviceCtrlHandler invoked with param CONTINUE\n");
            break;
        case SERVICE_CONTROL_STOP:
            setServiceStatus (SERVICE_STOPPED);
            break;
        case SERVICE_CONTROL_SHUTDOWN:
            setServiceStatus (SERVICE_STOPPED);
            break;
        default:
            checkAndLogMsg ("ServiceHelper::serviceCtrlHandler", Logger::L_LowDetailDebug,
                            "serviceCtrlHandler invoked with param = %d\n", fdwControl);
            break;
    }
}

int ServiceHelper::installService (const char *pszServiceName, const char *pszServiceDesc, const char *pszPathToSvcExe)
{
    HKEY hKey;
    char szServiceKeyPath[] = "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\";
    char *pszServiceKey = new char [strlen (szServiceKeyPath) + strlen (pszServiceName) + 1];
    strcpy (pszServiceKey, szServiceKeyPath);
    strcat (pszServiceKey, pszServiceName);
    if (RegCreateKey (HKEY_LOCAL_MACHINE, pszServiceKey, &hKey)) {
        checkAndLogMsg ("ServiceHelper::installService", Logger::L_SevereError,
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
                       (DWORD)strlen(pszPathToSvcExe) + 1)) {   // choy - jan 4 2007
        checkAndLogMsg ("ServiceHelper::installService", Logger::L_SevereError,
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
        checkAndLogMsg ("ServiceHelper::installService", Logger::L_SevereError,
                        "failed to set event log registry key 2: %s\n", getLastOSErrorAsString());
        RegCloseKey (hKey);
        return -3;
    }
    RegCloseKey (hKey);

    SC_HANDLE schManager = OpenSCManager (NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (schManager == NULL) {
        checkAndLogMsg ("ServiceHelper::installService", Logger::L_SevereError,
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
        checkAndLogMsg ("ServiceHelper::installService", Logger::L_SevereError,
                        "failed to create service: %s\n", getLastOSErrorAsString());
        CloseHandle (schManager);
        return -5;
    }
    CloseHandle (schService);
    CloseHandle (schManager);

    checkAndLogMsg ("ServiceHelper::installService", Logger::L_LowDetailDebug,
                    "successfully installed service %s\n", pszServiceName);

    return 0;
}

int ServiceHelper::removeService (const char *pszServiceName)
{
    SC_HANDLE schManager = OpenSCManager (NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (schManager == NULL) {
        checkAndLogMsg ("ServiceHelper::removeService", Logger::L_SevereError,
                        "failed to open service control manager: %s\n", getLastOSErrorAsString());
        return -1;
    }

    SC_HANDLE schService = OpenService (schManager, pszServiceName, SERVICE_ALL_ACCESS);
    if (schService == NULL) {
        checkAndLogMsg ("ServiceHelper::removeService", Logger::L_SevereError,
                        "failed to open service %s: %s\n", pszServiceName, getLastOSErrorAsString());
        CloseHandle (schManager);
        return -2;
    }
    if (!DeleteService (schService)) {
        checkAndLogMsg ("ServiceHelper::removeService", Logger::L_SevereError,
                        "failed to delete service %s: %s\n", pszServiceName, getLastOSErrorAsString());
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
        checkAndLogMsg ("ServiceHelper::removeService", Logger::L_SevereError,
                        "failed to delete event log registry key: %s\n", getLastOSErrorAsString());
        delete[] pszServiceKey;
        return -4;
    }
    delete[] pszServiceKey;

    checkAndLogMsg ("ServiceHelper::removeService", Logger::L_LowDetailDebug,
                    "successfully removed service %s\n", pszServiceName);

    return 0;
}

int ServiceHelper::setServiceStatus (DWORD dwStatus)
{
    _status.dwCurrentState = dwStatus;
    if (!SetServiceStatus (_statusHandle, &_status)) {
        checkAndLogMsg ("ServiceHelper::setServiceStatus", Logger::L_SevereError,
                        "failed to set service status: %s\n", getLastOSErrorAsString());
        return -1;
    }
    return 0;
}

bool ServiceHelper::stopService (const char *pszServiceName) 
{ 
    SC_HANDLE schSCManager = OpenSCManager (NULL, NULL, SC_MANAGER_ALL_ACCESS); 

    if (schSCManager == 0) {
        checkAndLogMsg ("ServiceHelper::stopService", Logger::L_MildError, "OpenSCManager failed, error code = %d\n", GetLastError());
    }
    else {
        SC_HANDLE schService = OpenService (schSCManager, pszServiceName, SERVICE_ALL_ACCESS);
        if (schService == 0) {
            checkAndLogMsg ("ServiceHelper::stopService", Logger::L_MildError, "OpenService failed, error code = %d\n", GetLastError());
        }
        else {
            SERVICE_STATUS status;
            if (ControlService (schService, SERVICE_CONTROL_STOP, &status)) {
                CloseServiceHandle (schService); 
                CloseServiceHandle (schSCManager); 
                return true;
            }
            else {
                checkAndLogMsg ("ServiceHelper::stopService", Logger::L_MildError, "ControlService failed, error code = %d\n", GetLastError());
            }
            CloseServiceHandle (schService); 
        }
        CloseServiceHandle (schSCManager); 
    }
    return false;
}