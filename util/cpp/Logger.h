/*
 * Logger.h
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2014 IHMC.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 (GPLv3) as published by the Free Software Foundation.
 *
 * U.S. Government agencies and organizations may redistribute
 * and/or modify this program under terms equivalent to
 * "Government Purpose Rights" as defined by DFARS 
 * 252.227-7014(a)(12) (February 2014).
 *
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 */

#ifndef INCL_LOGGER_H
#define INCL_LOGGER_H

#include <stdio.h>
#include <stdarg.h>
#include <cstring>

#include "FTypes.h"
#include "ManageableThread.h"
#include "Mutex.h"
#include "NLFLib.h"

#if defined (WIN32)
    // Do not want to include windows.h here because it will in turn include winsock.h
    // which conflicts with winsock2.h that is included by other parts of the codebase
    typedef void * HANDLE;
#elif defined (UNIX)
    #if defined (OSX)
        #include <strings.h>
    #endif
    #define _stricmp strcasecmp
#endif

namespace NOMADSUtil
{
    class UDPDatagramSocket;

    class Logger
    {
        public:
            Logger (void);
            ~Logger (void);
            // Usable debug levels
            // 1 - Severe Error Message (Requiring termination)
            // 2 - Mild Error Message
            // 3 - Warning
            // 4 - Info
            // 5 - Net (Lower) Detail Debug Message (Lower than Low Detail - introduced to reduce the number of messages logged over the network)
            // 6 - Low Detail Debug Message
            // 7 - Medium Detail Debug Message
            // 8 - High Detail Debug Message
            enum Level {
                L_SevereError = 1,
                L_MildError = 2,
                L_Warning = 3,
                L_Info = 4,
                L_NetDetailDebug = 5,
                L_LowDetailDebug = 6,
                L_MediumDetailDebug = 7,
                L_HighDetailDebug = 8
            };

            static const int DEFAULT_NETWORK_LOGGING_PORT = 1300;

            int initLogFile (const char *pszFileName, bool bAppend = true);
            int initErrorLogFile (const char *pszFileName, bool bAppend = true);
            int initNetworkLogging (const char *pszDestIPAddr, uint16 ui16DestPort = DEFAULT_NETWORK_LOGGING_PORT);
            int setLogFileHandle (FILE *file);
            FILE * getLogFileHandle (void);
            void enableScreenOutput (void);
            void disableScreenOutput (void);
            int enableFileOutput (void);
            int disableFileOutput (void);
            int enableErrorLogFileOutput (void);
            int disableErrorLogFileOutput (void);
            // Display the time in HH:mm:ss
            int displayAbsoluteTime (void);
            // Display the time in milliseconds from the start
            int displayRelativeTime (void);
            int enableNetworkOutput (unsigned char uchLevel);
            void disableNetworkOutput (void);
            void setDebugLevel (unsigned char uchNewLevel);
            int setDebugLevel (const char *cchNewLevel);
            unsigned char getDebugLevel (void);
            int enableOSLogOutput (const char *pszAppName, unsigned char uchLevel);
            void disableOSLogOutput (void);
            int enableHistoryMode (unsigned long ulBufLen);
            void enableServiceDialogOutput (void);
            void disableServiceDialogOutput (void);
            int writeHistoryToScreen (void);
            int writeHistoryToFile (void);
            int indent (unsigned short usCount = 2);
            int outdent (unsigned short usCount = 2);
            int logMsg (const char *pszSource, unsigned char uchLevel, const char *pszMsg, ...);

        protected:
            void writeIndentSpace (FILE *file, unsigned short usCount);

        protected:
            Mutex _mLog;
            int64 _i64StartTime;
            FILE *_fileLog;
            FILE *_fileErrorLog;
            uint32 _ui32DestAddr;
            uint16 _ui16DestPort;
            UDPDatagramSocket *_pDGSocket;
            bool _bWriteToScreen;
            bool _bWriteToFile;
            bool _bWriteToErrorLogFile;
            bool _bWriteToOSLog;
            bool _bWriteToNetwork;
            unsigned char _uchDebugLevel;
            unsigned char _uchOSLogDebugLevel;
            unsigned char _uchNetworkLogDebugLevel;
            bool _bHistoryMode;
            unsigned long _ulHistoryBufLen;
            bool _bServiceDialogOutput;
            unsigned short _usCurrIndent;
            bool _bDisplayRelativeTime;
            #if defined (WIN32)
                HANDLE _eventLogHandle;
            #endif
    };

    class LoggerNetworkListener : public ManageableThread
    {
        public:
            LoggerNetworkListener (void);
            virtual ~LoggerNetworkListener (void);

            int init (uint16 ui16DestPort=Logger::DEFAULT_NETWORK_LOGGING_PORT);
            int init (uint16 ui16DestPort, uint32 ui32BindAddress);

            void run (void);

        private:
            UDPDatagramSocket *_pDGSocket;
            static const uint16 MAX_MESSAGE_SIZE = 65000;
    };

    extern Logger *pLogger;
    extern Logger *pNetLog;
    extern Logger *pTopoLog;

    inline void Logger::enableScreenOutput (void)
    {
        _bWriteToScreen = true;
    }

    inline void Logger::disableScreenOutput (void)
    {
        _bWriteToScreen = false;
    }

    inline int Logger::enableFileOutput (void)
    {
        if (_fileLog == NULL) {
            return -1;
        }
        _bWriteToFile = true;
        return 0;
    }

    inline int Logger::disableFileOutput (void)
    {
        _bWriteToFile = false;
        return 0;
    }

    inline int Logger::enableErrorLogFileOutput (void)
    {
        if (_fileErrorLog == NULL) {
            return -1;
        }
        _bWriteToErrorLogFile = true;
        return 0;
    }

    inline int Logger::disableErrorLogFileOutput (void)
    {
        _bWriteToErrorLogFile = false;
        return 0;
    }

    inline int Logger::displayAbsoluteTime (void)
    {
        _bDisplayRelativeTime = false;
        return 0;
    }

    inline int Logger::displayRelativeTime (void)
    {
        _bDisplayRelativeTime = true;
        return 0;
    }

    inline void Logger::setDebugLevel (unsigned char uchNewLevel)
    {
        _uchDebugLevel = uchNewLevel;
    }
    
    inline int Logger::setDebugLevel (const char *cchNewLevel)
    {
        if (!_stricmp(cchNewLevel, "L_SevereError")) {
            _uchDebugLevel = (unsigned char) L_SevereError;
        }
        else if (!_stricmp(cchNewLevel, "L_MildError")) {
            _uchDebugLevel = (unsigned char) L_MildError;
        }
        else if (!_stricmp(cchNewLevel, "L_Warning")) {
            _uchDebugLevel = (unsigned char) L_Warning;
        }
        else if (!_stricmp(cchNewLevel, "L_Info")) {
            _uchDebugLevel = (unsigned char) L_Info;
        }
        else if (!_stricmp(cchNewLevel, "L_NetDetailDebug")) {
            _uchDebugLevel = (unsigned char) L_NetDetailDebug;
        }
        else if (!_stricmp(cchNewLevel, "L_LowDetailDebug")) {
            _uchDebugLevel = (unsigned char) L_LowDetailDebug;
        }
        else if (!_stricmp(cchNewLevel, "L_MediumDetailDebug")) {
            _uchDebugLevel = (unsigned char) L_MediumDetailDebug;
        }
        else if (!_stricmp(cchNewLevel, "L_HighDetailDebug")) {
            _uchDebugLevel = (unsigned char) L_HighDetailDebug;
        }
        else {
            // No match found
            return -1;
        }

        return 0;
    }

    inline unsigned char Logger::getDebugLevel (void)
    {
        return _uchDebugLevel;
    }

    inline int Logger::indent (unsigned short usCount)
    {
        _usCurrIndent += usCount;
        return 0;
    }

    inline int Logger::outdent (unsigned short usCount)
    {
        if (_usCurrIndent < usCount) {
            _usCurrIndent = 0;
            return -1;
        }
        _usCurrIndent -= usCount;
        return 0;
    }

    inline void Logger::writeIndentSpace (FILE *file, unsigned short usCount)
    {
        //for (unsigned short us = 0; us < usCount; us++) {
        //    fputc (' ', file);
        //}
    }

}

#endif   // #ifndef INCL_LOGGER_H
