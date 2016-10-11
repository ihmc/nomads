/*
 * ProcessExecutor.cpp
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

#include "ProcessExecutor.h"

#include <errno.h>
#include <stdarg.h>

#if defined (WIN32)
    #include <io.h>
    #include <process.h>
#include <winsock2.h>
    #include <windows.h>

    #define strdup _strdup
#elif defined (UNIX)
    #include <string.h>
    #include <signal.h>
    #include <sys/wait.h>
    #include <unistd.h>
#else
    #error Must Define WIN32 or UNIX!
#endif

#include "Logger.h"
#include "NLFLib.h"

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace NOMADSUtil;

ProcessExecutor::ProcessExecutor (void)
{
    _iNumArgs = 0;
    _apszArgs = NULL;
    _iNumEnvVars = 0;
    _apszEnvVars = NULL;
    _pet = NULL;
}

ProcessExecutor::~ProcessExecutor (void)
{
    if (_pet) {
        if (_pet->isRunning()) {
            _pet->kill (1000);
        }
        delete _pet;
        _pet = NULL;
    }

    if (_apszArgs) {
        for (int i = 0; i < _iNumArgs; i++) {
            free (_apszArgs[i]);
        }
        delete[] _apszArgs;
    }
    if (_apszEnvVars) {
        for (int i = 0; i < _iNumEnvVars; i++) {
            free (_apszEnvVars[i]);
        }
        delete[] _apszEnvVars;
    }
}

int ProcessExecutor::configure (const char *pszExecutablePath, ...)
{
    if (_apszArgs) {
        for (int i = 0; i < _iNumArgs; i++) {
            free (_apszArgs[i]);
        }
        delete[] _apszArgs;
        _apszArgs = NULL;
        _iNumArgs = 0;
    }
    
    if (_apszEnvVars) {
        for (int i = 0; i < _iNumEnvVars; i++) {
            free (_apszEnvVars[i]);
        }
        delete[] _apszEnvVars;
        _apszEnvVars = NULL;
        _iNumEnvVars = 0;
    }

    executablePath = pszExecutablePath;

    va_list argList;
    va_start (argList, pszExecutablePath);
    const char *pszArg;
    while (NULL != (pszArg = va_arg (argList, const char *))) {
        _iNumArgs++;
    }
    va_end (argList);

    _apszArgs = new char * [_iNumArgs+1];      // Need to include NULL
    va_start (argList, pszExecutablePath);
    for (int i = 0; i < _iNumArgs; i++) {
        _apszArgs[i] = strdup (va_arg (argList, char *));
    }
    _apszArgs[_iNumArgs] = NULL;
    va_end (argList);

    return 0;
}

int ProcessExecutor::configure (const char *pszExecutablePath, char **apszArgs, char **apszEnvVars)
{
    int i;
    if (_apszArgs) {
        for (i = 0; i < _iNumArgs; i++) {
            free (_apszArgs[i]);
        }
        delete[] _apszArgs;
        _apszArgs = NULL;
        _iNumArgs = 0;
    }

    if (_apszEnvVars) {
        for (i = 0; i < _iNumEnvVars; i++) {
            free (_apszEnvVars[i]);
        }
        delete[] _apszEnvVars;
        _apszEnvVars = NULL;
        _iNumEnvVars = 0;
    }

    executablePath = pszExecutablePath;

    while (apszArgs[_iNumArgs]) {
        _iNumArgs++;
    }

    _apszArgs = new char * [_iNumArgs + 1];      // Need to include NULL
    for (i = 0; i < _iNumArgs; i++) {
        _apszArgs[i] = strdup (apszArgs[i]);
    }
    _apszArgs[_iNumArgs] = NULL;

    if (apszEnvVars) {
        while (apszEnvVars[_iNumEnvVars]) {
            _iNumEnvVars++;
        }
        _apszEnvVars = new char * [_iNumEnvVars + 1];   // Need to include NULL
        for (i = 0; i < _iNumEnvVars; i++) {
            _apszEnvVars[i] = strdup (apszEnvVars[i]);
        }
        _apszEnvVars[_iNumEnvVars] = NULL;
    }

    return 0;
}

int ProcessExecutor::configure (const char *pszExecutablePath, DArray2<String> &args)
{
    int i;
    if (_apszArgs) {
        for (i = 0; i < _iNumArgs; i++) {
            free (_apszArgs[i]);
        }
        delete[] _apszArgs;
        _apszArgs = NULL;
        _iNumArgs = 0;
    }

    if (_apszEnvVars) {
        for (i = 0; i < _iNumEnvVars; i++) {
            free (_apszEnvVars[i]);
        }
        delete[] _apszEnvVars;
        _apszEnvVars = NULL;
        _iNumEnvVars = 0;
    }

    executablePath = pszExecutablePath;

    _iNumArgs = args.size();
    _apszArgs = new char * [_iNumArgs + 1];
    for (i = 0; i < _iNumArgs; i++) {
        _apszArgs[i] = strdup (args[i]);
    }
    _apszArgs[_iNumArgs] = NULL;

    return 0;
}

int ProcessExecutor::configure (const char *pszExecutablePath, DArray2<String> &args, DArray2<String> &envVars)
{
    int i;
    if (_apszArgs) {
        for (i = 0; i < _iNumArgs; i++) {
            free (_apszArgs[i]);
        }
        delete[] _apszArgs;
        _apszArgs = NULL;
        _iNumArgs = 0;
    }

    if (_apszEnvVars) {
        for (i = 0; i < _iNumEnvVars; i++) {
            free (_apszEnvVars[i]);
        }
        delete[] _apszEnvVars;
        _apszEnvVars = NULL;
        _iNumEnvVars = 0;
    }

    executablePath = pszExecutablePath;

    _iNumArgs = args.size();
    _apszArgs = new char * [_iNumArgs + 1];
    for (i = 0; i < _iNumArgs; i++) {
        _apszArgs[i] = strdup (args[i]);
    }
    _apszArgs[_iNumArgs] = NULL;
    
    _iNumEnvVars = envVars.size();
    _apszEnvVars = new char * [_iNumEnvVars + 1];
    for (i = 0; i < _iNumEnvVars; i++) {
        _apszEnvVars[i] = strdup (envVars[i]);
    }
    _apszEnvVars[_iNumEnvVars] = NULL;

    return 0;
}

int ProcessExecutor::start (void)
{
    if (_pet) {
        if (_pet->isRunning()) {
            return -1;
        }
        else {
            delete _pet;
            _pet = NULL;
        }
    }
    _pet = new ExecutorThread (this);
    if (_pet->start()) {
        return -2;
    }
    sleepForMilliseconds (1000);
    return 0;
}

int ProcessExecutor::getPID (void)
{
    if (_pet) {
        return _pet->getPID();
    }
    else {
        return -1;
    }
}

bool ProcessExecutor::isRunning (void)
{
    if (_pet) {
        return _pet->isRunning();
    }
    else {
        return false;
    }
}

int ProcessExecutor::kill (unsigned long ulTimeOut)
{
    if (_pet) {
        if (_pet->isRunning()) {
            return _pet->kill (ulTimeOut);
        }
        return 0;
    }
    else {
        return -1;
    }
}

ProcessExecutor::ExecutorThread::ExecutorThread (ProcessExecutor *ppe)
    : _cv (&_m)
{
    _ppe = ppe;
    _bRunning = false;
}

void ProcessExecutor::ExecutorThread::run (void)
{
    #if defined (WIN32)
        int i;
        int iLen = 0;
        for (i = 0; i < _ppe->_iNumArgs; i++) {
            iLen += (int) strlen (_ppe->_apszArgs[i]);
            iLen += 3;     // For the enclosing quotes and the space
        }
        char *pszCmdLine = new char [iLen+1];
        pszCmdLine[0] = '\0';
        for (i = 0; i < _ppe->_iNumArgs; i++) {
            if (i > 0) {
                strcat (pszCmdLine, " \"");
            }
            else {
                strcat (pszCmdLine, "\"");
            }
            strcat (pszCmdLine, _ppe->_apszArgs[i]);
            strcat (pszCmdLine, "\"");
        };
        STARTUPINFO si;
        ZeroMemory (&si, sizeof (si));
        ZeroMemory (&_procInfo, sizeof (_procInfo));
        if (!CreateProcess (NULL, pszCmdLine,
                            NULL, NULL, FALSE,
                            0,
                            NULL, NULL,
                            &si, &_procInfo)) {
            checkAndLogMsg ("ProcessExecutor::ExecutorThread::run", Logger::L_MildError,
                            "CreateProcess failed; os error = %d\n", GetLastError());
            return;
        }
    #elif defined (UNIX)
        int i;
        _iPID = fork();
        if (_iPID < 0) {
            checkAndLogMsg ("ProcessExecutor::ExecutorThread::run", Logger::L_MildError,
                            "failed to fork() process; os error = %d\n", errno);
            return;
        }
        else if (_iPID == 0) {
            // This is the child process 
            // Setup any environment variables
            for (i = 0; i < _ppe->_iNumEnvVars; i++) {
                if (putenv (_ppe->_apszEnvVars[i])) {
                    checkAndLogMsg ("ProcessExecutor::ExecutorThread::run", Logger::L_MildError,
                                    "failed to set environment variable <%s>; os error = %d\n",
                                    _ppe->_apszEnvVars[i], errno);
                }
            }

            // Call exec            
            if (execv (_ppe->executablePath, _ppe->_apszArgs)) {
                checkAndLogMsg ("ProcessExecutor::ExecutorThread::run", Logger::L_MildError,
                                "failed to exec process; os error = %d\n", errno);
                return;
            }
        }
    #endif

    _bRunning = true;

    #if defined (WIN32)
        if (WAIT_FAILED == WaitForSingleObject (_procInfo.hProcess, INFINITE)) {
            checkAndLogMsg ("ProcessExecutor::ExecutorThread::run", Logger::L_MildError,
                            "WaitForSingleObject failed; os error = %d\n", GetLastError());
        }
        CloseHandle (_procInfo.hProcess);
        CloseHandle (_procInfo.hThread);
    #elif defined (UNIX)
        if (waitpid (_iPID, NULL, 0) < 0) {
            checkAndLogMsg ("ProcessExecutor::ExecutorThread::run", Logger::L_MildError,
                            "failed to wait on child process; os error = %d\n", errno);
        }
    #endif

    _bRunning = false;
    _m.lock();
    _cv.notifyAll();
    _m.unlock();
}

int ProcessExecutor::ExecutorThread::getPID (void)
{
    #if defined (WIN32)
        return _procInfo.dwProcessId;
    #elif defined (UNIX)
        return _iPID;
    #endif
}

bool ProcessExecutor::ExecutorThread::isRunning (void)
{
    return _bRunning;
}

int ProcessExecutor::ExecutorThread::kill (unsigned long ulTimeOut)
{
    _m.lock();

    #if defined (WIN32)
        if (!TerminateProcess (_procInfo.hProcess, 0)) {
            checkAndLogMsg ("SpringExecutor::run", Logger::L_SevereError,
                            "failed to kill JRE process; os error = %d\n", GetLastError());
            _m.unlock();
            return -1;
        }
    #elif defined (UNIX)
        if (::kill (_iPID, SIGINT)) {
            checkAndLogMsg ("SpringExecutor::run", Logger::L_SevereError,
                            "failed to kill JRE process; os error = %d\n", errno);
        }
    #endif

    if (ulTimeOut > 0) {
        _cv.wait (ulTimeOut);
    }
    else {
        _cv.wait();
    }
    if (_bRunning == true) {
        _m.unlock();
        return -2;
    }
    _m.unlock();
    return 0;
}
