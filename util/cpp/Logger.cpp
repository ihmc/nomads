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
#include "InetAddr.h"
#include "UDPDatagramSocket.h"
#include "StrClass.h"

#if defined (WIN32)
    #include <windows.h>
    #define PATH_MAX _MAX_PATH
    #define snprintf _snprintf
#elif defined (UNIX)
    #include <syslog.h>
    #if defined (OSX)
        #include <sys/syslimits.h>
	    #define	INADDR_NONE		0xffffffff
    #endif
#endif


namespace NOMADSUtil
{
    Logger *pLogger;
    Logger *pNetLog;
    Logger *pTopoLog;
}

using namespace NOMADSUtil;

Logger::Logger (void)
{
    _i64StartTime = getTimeInMilliseconds();
    _fileLog = NULL;
    _fileErrorLog = NULL;
    _ui32DestAddr = 0;
    _ui16DestPort = 0;
    _pDGSocket = NULL;
    _bWriteToScreen = false;
    _bWriteToFile = false;
    _bWriteToErrorLogFile = false;
    _bWriteToOSLog = false;
    _bWriteToNetwork = false;
    _uchDebugLevel = 0;
    _uchOSLogDebugLevel = 0;
    _uchNetworkLogDebugLevel = 0;
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

    _mLog.lock();
    uint32 ui32ElapsedTime = (uint32) (getTimeInMilliseconds() - _i64StartTime);
    if (_bWriteToScreen) {
        if (_usCurrIndent > 0) {
            writeIndentSpace (stdout, _usCurrIndent);
        }

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

