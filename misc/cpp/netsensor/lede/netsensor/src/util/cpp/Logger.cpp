/*
 * Logger.cpp
 *
 * This file is part of the IHMC Utility Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

#include "Logger.h"
#include "HTTPClient.h"
#include "Json.h"
#include "InetAddr.h"
#include "UDPDatagramSocket.h"
#include "StrClass.h"
#include "ConfigManager.h"

#if defined (WIN32)
    #define NOMINMAX
    #include <winsock2.h>
    #include <windows.h>
    #define PATH_MAX _MAX_PATH
    #if _MCS_VER<1900
        #define snprintf _snprintf    
    #endif
#elif defined (UNIX)
    #include <syslog.h>
    #if defined (OSX)
        #include <sys/syslimits.h>
        #define	INADDR_NONE    0xffffffff
    #endif
    #if defined (ANDROID)
        #include <android/log.h>
    #endif
#endif

using namespace NOMADSUtil;

namespace LOGGER
{
#if defined (ANDROID)
    int toAndroidLoggingLevel (uint8 ui8DbgDetLevel)
    {
        switch (ui8DbgDetLevel) {
            case Logger::L_SevereError:
            case Logger::L_MildError:
                return ANDROID_LOG_ERROR;
            case Logger::L_Warning:
                return ANDROID_LOG_WARN;
            case Logger::L_Info:
                return ANDROID_LOG_INFO;
            case Logger::L_NetDetailDebug:
            case Logger::L_LowDetailDebug:
                return ANDROID_LOG_DEBUG;
            case Logger::L_MediumDetailDebug:
            case Logger::L_HighDetailDebug:
            default:
                return ANDROID_LOG_VERBOSE;
        }
    }
#endif

    const char * toSplunkLoggingLevelAsString (uint8 ui8DbgDetLevel)
    {
        /*DEBUG, INFO, NOTICE, WARN, ERROR, CRIT, ALERT, FATAL, and EMERG*/
        switch (ui8DbgDetLevel) {
            case Logger::L_SevereError:
                return "CRIT";
            case Logger::L_MildError:
                return "ERROR";
            case Logger::L_Warning:
                return "WARN";
            case Logger::L_Info:
                return "INFO";
            default:
                return "DEBUG";
        }
    }

    String toSplunkEvent (int64 iTimestamp, uint8 ui8DbgDetLevel, const char *pszHost, const char *pszSource, const char *pszMsg)
    {
        // http://docs.splunk.com/Documentation/Splunk/7.0.2/Data/FormatEventsforHTTPEventCollector
        /*
        {
            "time": 1437522387,
            "host": "dataserver992.example.com",
            "source": "testapp",
            "event": {
            "message": "Something happened",
                "severity": "INFO"
            }
        }
        */
        char buffer[1024];
        sprintf (buffer, "{\"time\": %lld, \"host\": %s, \"source\": %s, \"event\": { \"message\": %s, \"severity\": %s }}",
                 iTimestamp, pszHost, pszSource, pszMsg, toSplunkLoggingLevelAsString (ui8DbgDetLevel));
        return String (buffer);
    }

}

namespace NOMADSUtil
{
    Logger *pLogger;
}

using namespace LOGGER;

const char * Logger::LOGGING_ENABLED_PROPERTY = "util.logger.enabled";
const char * Logger::SCREEN_LOGGING_PROPERTY = "util.logger.out.screen.enabled";
const char * Logger::FILE_LOGGING_PROPERTY = "util.logger.out.file.enabled";
const char * Logger::ERROR_FILE_LOGGING_PROPERTY = "util.logger.error.file.path";
const char * Logger::LOGGING_LEVEL_PROPERTY = "util.logger.detail";

Logger::Logger (void)
    : _i64StartTime (getTimeInMilliseconds())
{
    _fileLog = NULL;
    _fileErrorLog = NULL;
    _ui32DestAddr = 0;
    _ui16DestPort = 0;
    _ui16SplunkSrvPort = 0;
    _pDGSocket = NULL;
    _splunkSrvIPAddr = "";
    _bWriteToScreen = false;
    _bWriteToFile = false;
    _bWriteToErrorLogFile = false;
    _bWriteToOSLog = false;
    _bWriteToNetwork = false;
    _bWriteToSplunk = false;
    _localHost = "unknown";
    _uchDebugLevel = 0;
    _uchOSLogDebugLevel = 0;
    _uchNetworkLogDebugLevel = 0;
    _uchSplunkLogDebugLevel = 0;
    _bHistoryMode = false;
    _ulHistoryBufLen = 0;
    _bServiceDialogOutput = false;
    _usCurrIndent = 0;
    _bDisplayRelativeTime = true;
    #if defined (WIN32)
        _eventLogHandle = NULL;
    #endif
}

Logger::~Logger (void)
{
    if (_fileLog) {
        fclose (_fileLog);
        _fileLog = NULL;
    }
    if (_fileErrorLog) {
        fclose (_fileErrorLog);
        _fileErrorLog = NULL;
    }
    if (_pDGSocket) {
        delete _pDGSocket;
        _pDGSocket = NULL;
    }
}
        
int Logger::configure (ConfigManager *pCfgMgr)
{
    if (pCfgMgr == NULL) {
        return -1;
    }
    if (pCfgMgr->getValueAsBool (LOGGING_ENABLED_PROPERTY, true)) {
        if (pLogger == NULL) {
            pLogger = new Logger();
        }
        if (pCfgMgr->getValueAsBool (SCREEN_LOGGING_PROPERTY, true)) {
            pLogger->enableScreenOutput();
        }
        if (pCfgMgr->hasValue (FILE_LOGGING_PROPERTY)) {
            if (pLogger->initLogFile (pCfgMgr->getValue (FILE_LOGGING_PROPERTY, "application.log")) < 0) {
                return -2;
            }
            if (pLogger->enableFileOutput() < 0) {
                return -3;
            }
        }
        if (pCfgMgr->hasValue (ERROR_FILE_LOGGING_PROPERTY)) {
            if (pLogger->initErrorLogFile (pCfgMgr->getValue (ERROR_FILE_LOGGING_PROPERTY)) < 0) {
                return -4;
            }
            if (pLogger->enableErrorLogFileOutput() < 0) {
                return -5;
            }
        }
        const uint8 ui8DbgDetLevel = (uint8) pCfgMgr->getValueAsInt (LOGGING_LEVEL_PROPERTY, Logger::L_LowDetailDebug);
        switch (ui8DbgDetLevel) {
            case Logger::L_SevereError:
            case Logger::L_MildError:
            case Logger::L_Warning:
            case Logger::L_Info:
            case Logger::L_NetDetailDebug:
            case Logger::L_LowDetailDebug:
            case Logger::L_MediumDetailDebug:
            case Logger::L_HighDetailDebug:
                pLogger->setDebugLevel (ui8DbgDetLevel);
                pLogger->logMsg ("Logger::configure", Logger::L_Info,
                                 "Setting debug level to %d\n", ui8DbgDetLevel);
                break;
            default:
                pLogger->setDebugLevel(Logger::L_LowDetailDebug);
                pLogger->logMsg ("Logger::configure", Logger::L_Warning,
                                 "Invalid Logger detail debug level. Setting it to %d\n",
                                 Logger::L_LowDetailDebug);
        }
    }
    else if (pLogger != NULL) {
        delete pLogger;
        pLogger = NULL;
    }
    return 0;
}

int Logger::initLogFile (const char *pszFileName, bool bAppendMode)
{
    if (_fileLog) {
        fclose (_fileLog);
        _fileLog = NULL;
        _bWriteToFile = false;
    }
    if (NULL == (_fileLog = fopen (pszFileName, bAppendMode?"a":"w"))) {
        return -1;
    }
    return 0;
}

int Logger::initErrorLogFile (const char *pszFileName, bool bAppendMode)
{
    if (_fileErrorLog) {
        fclose (_fileErrorLog);
        _fileErrorLog = NULL;
        _bWriteToErrorLogFile = false;
    }
    if (NULL == (_fileErrorLog = fopen (pszFileName, bAppendMode?"a":"w"))) {
        return -1;
    }
    return 0;
}

int Logger::initNetworkLogging (const char *pszDestIPAddr, uint16 ui16DestPort)
{
    if (_pDGSocket == NULL) {
        _pDGSocket = new UDPDatagramSocket();
        if (0 != _pDGSocket->init (0)) {
            return -1;
        }
    }
    if ((_ui32DestAddr = inet_addr (pszDestIPAddr)) == INADDR_NONE) {
        return -2;
    }
    _ui16DestPort = ui16DestPort;
    return 0;
}

int Logger::initSplunkLogging (const char *pszLocalHost, const char *pszSplunkSrvIPAddr, uint16 ui16SplunkSrvPort)
{
    if (pszSplunkSrvIPAddr == NULL) {
        return - 1;
    }
    _ui16SplunkSrvPort = ui16SplunkSrvPort;
    _splunkSrvIPAddr = pszSplunkSrvIPAddr;
    if (pszLocalHost != NULL) {
        _localHost = pszLocalHost;
    }
}

int Logger::setLogFileHandle (FILE *file)
{
    if (_fileLog) {
        fclose (_fileLog);
        _bWriteToFile = false;
    }
    _fileLog = file;
    return 0;
}

FILE * Logger::getLogFileHandle (void)
{
    return _fileLog;
}

int Logger::enableHistoryMode (unsigned long ulBufLen)
{
    return -1;
}

int Logger::enableNetworkOutput (unsigned char uchLevel)
{
    if ((_pDGSocket == NULL) || (_ui32DestAddr == 0) || (_ui16DestPort == 0)) {
        return -1;
    }
    _uchNetworkLogDebugLevel = uchLevel;
    _bWriteToNetwork = true;
    return 0;
}

void Logger::disableNetworkOutput (void)
{
    _bWriteToNetwork = false;
}

int Logger::enableSplunkOutput (unsigned char uchLevel)
{
    if ((_splunkSrvIPAddr.length() <= 0) || (_ui16SplunkSrvPort == 0)) {
        return - 1;
    }
    _uchSplunkLogDebugLevel = uchLevel;
    _bWriteToSplunk = true;
}

void Logger::disableSplunkOutput (void)
{
    _bWriteToSplunk = false;
}

int Logger::enableOSLogOutput (const char *pszAppName, unsigned char uchLevel)
{
    #if defined (WIN32)
        _eventLogHandle = RegisterEventSource (NULL, pszAppName);
        if (_eventLogHandle == NULL) {
            logMsg ("Logger::enableOSLogOutput", Logger::L_MildError,
                    "failed to register event source: %s\n", getLastOSErrorAsString());
            return -1;
        }
    #elif defined (UNIX)
        openlog (pszAppName, LOG_NDELAY | LOG_PID, LOG_DAEMON);
    #endif

    _uchOSLogDebugLevel = uchLevel;
    _bWriteToOSLog = true;
    return 0;
}

void Logger::enableServiceDialogOutput (void)
{
    _bServiceDialogOutput = true;
}

void Logger::disableServiceDialogOutput (void)
{
    _bServiceDialogOutput = false;
}

void Logger::disableOSLogOutput (void)
{
    #if defined (WIN32)
        DeregisterEventSource (_eventLogHandle);
    #elif defined (UNIX)
        closelog();
    #endif

    _bWriteToOSLog = false;
}

int Logger::writeHistoryToScreen (void)
{
    return -1;
}

int Logger::writeHistoryToFile (void)
{
    return -1;
}

int Logger::logMsg (const char *pszSource, unsigned char uchLevel, const char *pszMsg, ...)
{
    if (uchLevel > _uchDebugLevel) {
        return 0;
    }
    va_list vargs;

    const int64 i64Now = getTimeInMilliseconds();
    uint32 ui32ElapsedTime = (uint32) (i64Now - _i64StartTime);
    _mLog.lock();
    if (_bWriteToScreen) {
        if (_usCurrIndent > 0) {
            writeIndentSpace (stdout, _usCurrIndent);
        }
#if defined (ANDROID)
        va_start (vargs, pszMsg);
        __android_log_vprint (toAndroidLoggingLevel (uchLevel), pszSource, pszMsg, vargs);
        va_end (vargs);
#else
        if (_bDisplayRelativeTime) {
            fprintf (stdout, "%lu - %s %d ", ui32ElapsedTime, pszSource, (int) uchLevel);
        }
        else {
            time_t now = time (NULL);
            struct tm *ptm = localtime (&now);
            fprintf (stdout, "%02d:%02d:%02d - ", ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
            fprintf (stdout, "%s %d ", pszSource, (int) uchLevel);

        }
        va_start (vargs, pszMsg);
        vfprintf (stdout, pszMsg, vargs);
        va_end (vargs);
#endif
    }
    if (_bServiceDialogOutput) {
        char szMsg[PATH_MAX];
        sprintf (szMsg, "Source: %s\nDebug level: %d\nMessage: %s\n", pszSource, uchLevel, pszMsg);
        #if defined (WIN32)
            MessageBox (NULL, szMsg, "warning", MB_OK);
        #else
            fprintf (stderr, szMsg);
        #endif
    }
    if (_bWriteToFile) {
        if (_usCurrIndent > 0) {
            writeIndentSpace (_fileLog, _usCurrIndent);
        }

        if (_bDisplayRelativeTime) {
            fprintf (_fileLog, "%lu - %s %d ", ui32ElapsedTime, pszSource, (int) uchLevel);
        }
        else {
            time_t now = time (NULL);
            struct tm *ptm = localtime (&now);
            fprintf (_fileLog, "%02d:%02d:%02d - ", ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
            fprintf (_fileLog, "%s %d ", pszSource, (int) uchLevel);
        }
        va_start (vargs, pszMsg);
        vfprintf (_fileLog, pszMsg, vargs);
        fflush (_fileLog);
        va_end (vargs);
    }
    if ((_bWriteToErrorLogFile) && (uchLevel <= Logger::L_Warning)) {
        if (_usCurrIndent > 0) {
            writeIndentSpace (_fileErrorLog, _usCurrIndent);
        }

        if (_bDisplayRelativeTime) {
            fprintf (_fileErrorLog, "%lu - %s %d ", ui32ElapsedTime, pszSource, (int) uchLevel);
        }
        else {
            time_t now = time (NULL);
            struct tm *ptm = localtime (&now);
            fprintf (_fileErrorLog, "%02d:%02d:%02d - ", ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
            fprintf (_fileErrorLog, "%s %d ", pszSource, (int) uchLevel);
        }
        va_start (vargs, pszMsg);
        vfprintf (_fileErrorLog, pszMsg, vargs);
        fflush (_fileErrorLog);
        va_end (vargs);
    }
    if (_bWriteToNetwork) {
        char szBuf[65535];
        snprintf (szBuf, sizeof (szBuf), "%s %d ", pszSource, (int) uchLevel);
        szBuf[sizeof(szBuf)-1] = '\0';
        size_t prefixLen = strlen (szBuf);
        va_start (vargs, pszMsg);
        vsnprintf (szBuf+prefixLen, sizeof (szBuf) - prefixLen, pszMsg, vargs);
        va_end (vargs);
        szBuf[sizeof(szBuf)-1] = '\0';
        _pDGSocket->sendTo (_ui32DestAddr, _ui16DestPort, szBuf, (int) strlen (szBuf));
    }
    if (_bWriteToSplunk) {
        char szBuf[65535];
        va_start (vargs, pszMsg);
        vsnprintf (szBuf, sizeof (szBuf), pszMsg, vargs);
        va_end (vargs);
        szBuf[sizeof (szBuf) - 1] = '\0';
        String json (toSplunkEvent (i64Now, uchLevel, _localHost, pszSource, szBuf));
        HTTPClient::postData (_splunkSrvIPAddr, _ui16DestPort, "/services/collector/event", json);
    }
    if ((_bWriteToOSLog) && (uchLevel <= _uchOSLogDebugLevel)) {
        #if defined (WIN32)
            WORD wType;
            switch (uchLevel) {
                case L_SevereError:
                case L_MildError:
                    wType = EVENTLOG_ERROR_TYPE;
                    break;
                case L_Warning:
                    wType = EVENTLOG_WARNING_TYPE;
                    break;
                default:
                    wType = EVENTLOG_INFORMATION_TYPE;
                    break;
            }
            char szBuf[1024];
            va_start (vargs, pszMsg);
            _vsnprintf (szBuf, sizeof(szBuf), pszMsg, vargs);
            va_end (vargs);
            LPCTSTR* lpStrings = new LPCTSTR [1];
            lpStrings [0] = szBuf;
            if (!ReportEvent (_eventLogHandle, wType, 0, 1, NULL, 1, 0, lpStrings, NULL)) {
            }
        #elif defined (LINUX)
            int iPriority;
            switch (uchLevel) {
                case L_SevereError:
                    iPriority = LOG_CRIT;
                    break;
                case L_MildError:
                    iPriority = LOG_ERR;
                    break;
                case L_Warning:
                    iPriority = LOG_WARNING;
                    break;
                case L_Info:
                    iPriority = LOG_INFO;
                    break;
                default:
                    iPriority = LOG_DEBUG;
                    break;
            }
            va_start (vargs, pszMsg);
            vsyslog (iPriority, pszMsg, vargs);
            va_end (vargs);
        #endif
    }
    _mLog.unlock();
    return 0;
}

LoggerNetworkListener::LoggerNetworkListener()
{
    _pDGSocket = new UDPDatagramSocket();
}

LoggerNetworkListener::~LoggerNetworkListener()
{
    delete _pDGSocket;
}
int LoggerNetworkListener::init (uint16 ui16DestPort)
{
    return init (ui16DestPort, INADDR_ANY);
}

int LoggerNetworkListener::init (uint16 ui16DestPort, uint32 ui32BindAddress)
{
    if (_pDGSocket->init (ui16DestPort, ui32BindAddress) < 0){
        return -1;
    }
    return 0;
}

void LoggerNetworkListener::run (void)
{
    const char *pszMethodName = "LoggerListener::run";
    started();
    char achBuf [MAX_MESSAGE_SIZE];
    InetAddr *pRemoteAddr = new InetAddr;
    while (!terminationRequested()) {
        memset (achBuf, 0, MAX_MESSAGE_SIZE);
        //max returned value should be about 65,000 (udp mtu), but we can have negative values
        int32 iReturned = _pDGSocket->receive (achBuf, MAX_MESSAGE_SIZE, pRemoteAddr);

        if (iReturned == 0) {
            printf ("%s %d %s", pszMethodName, Logger::L_HighDetailDebug, "Socket timed out.\n");
        }
        else if (iReturned < 0) {
            printf ("%s %d %s %d", pszMethodName, Logger::L_SevereError,
                    "Error receiving packets\n", iReturned);
        }
        else {
            String s = achBuf;
            printf ("%s", s.c_str());
        }
    }
    terminating();
}

