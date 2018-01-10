/*
 * ProcessExecutor.h
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
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

#ifndef INCL_PROCESS_EXECUTOR_H
#define INCL_PROCESS_EXECUTOR_H

#include "ConditionVariable.h"
#include "DArray2.h"
#include "Mutex.h"
#include "StrClass.h"
#include "Thread.h"

#if defined (WIN32)
	#define NOMINMAX
	#include <winsock2.h>
	#include <windows.h>
#elif defined (UNIX)
#else
    #error Must Define WIN32 or UNIX!
#endif

namespace NOMADSUtil
{
    class ProcessExecutor
    {
        public:
            ProcessExecutor (void);
            ~ProcessExecutor (void);

            int configure (const char *pszExecutablePath, ...);
            int configure (const char *pszExecutablePath, char**apszArgs, char **apszEnvVars = NULL);
            int configure (const char *pszExecutablePath, DArray2<String> &args);
            int configure (const char *pszExecutablePath, DArray2<String> &args, DArray2<String> &envVars);
            int redirectOutput (const char *pszFile);
            int start (void);
            int getPID (void);
            bool isRunning (void);
            int kill (unsigned long ulTimeOut);

        private:
            class ExecutorThread : public Thread
            {
                public:
                    ExecutorThread (ProcessExecutor *ppe);

                    void run (void);
                    int getPID (void);
                    bool isRunning (void);
                    int kill (unsigned long ulTimeOut);

                private:
                    #if defined (WIN32)
                        PROCESS_INFORMATION _procInfo;
                    #elif defined (UNIX)
                        int _iPID;
                    #endif
                    bool _bRunning;
                    ProcessExecutor *_ppe;
                    Mutex _m;
                    ConditionVariable _cv;
            };
            friend class ExecutorThread;

        private:
            String executablePath;
            String consoleOutputFile;
            int _iNumArgs;
            char **_apszArgs;
            int _iNumEnvVars;
            char **_apszEnvVars;
            ExecutorThread *_pet;

    };
}

#endif   // #ifndef INCL_PROCESS_EXECUTOR_H
