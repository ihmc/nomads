/*
 * OSThread.cpp
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

#include "OSThread.h"

#if defined (WIN32)
#define NOMINMAX
#include <winsock2.h>
    #include <windows.h>
#elif defined (UNIX)
    #include <unistd.h>
    #include <sys/errno.h>
    #include <sys/time.h>
    #include <sys/resource.h>
#endif

#include <stdarg.h>

#include "Logger.h"
#include "NLFLib.h"

using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

#if defined (WIN32)
    ThreadLocalStorage OSThread::_tls;
#endif

#if defined (WIN32)
    ThreadLocalStorage::ThreadLocalStorage (void)
    {
        dwTLSIndex = TlsAlloc();
    }

    ThreadLocalStorage::~ThreadLocalStorage (void)
    {
        TlsFree (dwTLSIndex);
    }
#endif

// This is a cut-and-paste from the Visual Studio Help
#ifdef _MSC_VER
#define MS_VC_EXCEPTION 0x406D1388

    typedef struct tagTHREADNAME_INFO {
        DWORD dwType; // Must be 0x1000.
        LPCSTR szName; // Pointer to name (in user addr space).
        DWORD dwThreadID; // Thread ID (-1=caller thread).
        DWORD dwFlags; // Reserved for future use, must be zero.
    } THREADNAME_INFO;

    void SetVCDebuggerThreadName( int dwThreadID, const char* szThreadName) {
        THREADNAME_INFO info;
        info.dwType = 0x1000;
        info.szName = szThreadName;
        info.dwThreadID = dwThreadID;
        info.dwFlags = 0;

        __try {
            RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(DWORD), (const ULONG_PTR *)(&info) );
        }
        __except(EXCEPTION_CONTINUE_EXECUTION) {
        }
    }
#endif
    // End cut-and-paste

OSThread::OSThread (void)
{
    #if defined (WIN32)
        _hThread = NULL;
        _dwThreadId = 0;
        _i64TimeWhenLastQueriedUtilization = 0;
        _i64LastQueriedCPUTime = 0;
    #elif defined (UNIX)
        _pid = 0;
        _tid = 0;
        _bIsDetachedMode = false;
    #endif
    _bStarted = false;
}

OSThread::~OSThread (void)
{
    #if defined (WIN32)
        if (_hThread) {
            CloseHandle (_hThread);
            _hThread = NULL;
        }
    #elif defined (UNIX)
        // Do not call pthread_join() since the thread is now being
        // created in a detached state
    #endif
}

#if defined (WIN32)

    int OSThread::start (void (*pFn) (void *pArg), void *pArg)
    {
        if (NULL == (_hThread = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE)pFn, pArg, 0, &_dwThreadId))) {
            return -1;
        }
        return 0;
    }

#elif defined (UNIX)

    int OSThread::start (void (*pFn) (void *pArg), void *pArg, bool bDetached)
    {
        pthread_attr_t ta;
        pthread_attr_init (&ta);
        pthread_attr_setscope (&ta, PTHREAD_SCOPE_SYSTEM);
        _bIsDetachedMode = bDetached;
        if (bDetached) {
            pthread_attr_setdetachstate (&ta, PTHREAD_CREATE_DETACHED);
        }
        ThreadArg *pTA = new ThreadArg;
        pTA->pThis = this;
        pTA->pFn = pFn;
        pTA->pArg = pArg;
        if (pthread_create (&_tid, &ta, ((void*(*)(void*))execUserFunc), pTA)) {
            pthread_attr_destroy (&ta);
            return -1;
        }
        pthread_attr_destroy (&ta);
        _bStarted = true;
        return 0;
    }

#endif

OSThread * OSThread::createOSThreadForCurrentThread (void)
{
    OSThread *pOSThread = new OSThread();
    #if defined (WIN32)
        pOSThread->_hThread = GetCurrentThread();
        pOSThread->_dwThreadId = GetCurrentThreadId();
    #elif defined (UNIX)
        pOSThread->_tid = pthread_self();
    #endif
    return pOSThread;
}

void OSThread::yield (void)
{
    #if defined (WIN32)
        Sleep(0);
    #elif defined (UNIX)
        sched_yield();
    #endif
}

int OSThread::suspend (void)
{
    return 0;
}

int OSThread::resume (void)
{
    return 0;
}

int OSThread::setPriority (int priority)
{
    #if defined (WIN32)
        int winPriority;
        switch (priority) {
        case 1: case 2:
            winPriority = THREAD_PRIORITY_LOWEST;
            break;
        case 3: case 4:
            winPriority = THREAD_PRIORITY_BELOW_NORMAL;
            break;
        case 5: case 6:
            winPriority = THREAD_PRIORITY_NORMAL;
            break;
        case 7: case 8:
            winPriority = THREAD_PRIORITY_ABOVE_NORMAL;
            break;
        case 9: case 10:
            winPriority = THREAD_PRIORITY_HIGHEST;
            break;
        default:
            if (priority < 0) {
                winPriority = THREAD_PRIORITY_LOWEST;
            }
            else if (priority > 10) {
                winPriority = THREAD_PRIORITY_HIGHEST;
            }
            break;
        }
        if (SetThreadPriority (_hThread, winPriority) == 0) {
            return -1;
        }
    #elif defined (UNIX)
        if (_tid == 0) {
            // Thread has not been created - return an error
            return -1;
        }

/*
        struct sched_param sp;
        sp.sched_priority = priority;
        int rc = pthread_setschedparam (_tid, policy, &sp);
        if (rc != 0) {
            return -2;
        }
*/

        if (_pid == 0) {
            //we're going to wait for a little while, until the
            //execUserFunc() method gets called by the pthread library
            //and sets the _pid variable on this thread.

            int msToSleep = 2;
            int maxMSToSleep = 20;
            while ( (_pid == 0) && (maxMSToSleep > 0) ) {
                sleepForMilliseconds(msToSleep);
                maxMSToSleep -= msToSleep;
            }
            if (_pid == 0) {
                return -2;
            }
        }

        int rc = setpriority(PRIO_PROCESS, _pid, priority);
        if (rc != 0) {
            return -3;
        }

    #endif

    return 0;
}

int OSThread::getPriority (void)
{
    #if defined (WIN32)
        return -1;
    #elif defined (UNIX)
/*
        struct sched_param sp;
        int policy;
        if (pthread_getschedparam (_tid, &policy, &sp) != 0) {
            return -1;
        }
        return sp.sched_priority;
*/
        if (_pid == 0) {
            return -201;
        }

        errno = 0;
        int prio = getpriority (PRIO_PROCESS, _pid);
        if (errno != 0) {
            return -200;
        }

        return prio;
    #endif
}

int OSThread::terminate (void)
{
    #if defined (WIN32)
        if (0 == TerminateThread (_hThread, 0)) {
            return -1;
        }
        if (0 == CloseHandle (_hThread)) {
            return -2;
        }
        _hThread = NULL;
        _dwThreadId = 0;
    #elif defined (ANDROID) //Android NDK doesn't support pthread_cancel
        if (pthread_kill (_tid, 0)) {
            return -1;
        }
    #elif defined (UNIX)
        if (pthread_cancel (_tid)) {
            return -1;
        }
    #endif
    return 0;
}

void OSThread::setThreadLocalData (void *pData)
{
    #if defined (WIN32)
        TlsSetValue (_tls.dwTLSIndex, pData);
    #elif defined (UNIX)
    #endif
}

void * OSThread::getThreadLocalData (void)
{
    #if defined (WIN32)
        return TlsGetValue (_tls.dwTLSIndex);
    #elif defined (UNIX)
        return NULL;
    #endif
}

int OSThread::waitForThreadToTerminate (uint64 ui64TimeOutInMillis)
{
    #if defined (WIN32)
        DWORD dwReturnValue;
        DWORD dwTimeOut;
        if (ui64TimeOutInMillis == 0) {
            dwTimeOut = INFINITE;
        }
        else if (ui64TimeOutInMillis >= MAXDWORD) {
            fprintf (stderr, "waitForThreadToTerminate: timeout is too long, using INFINITE");
            dwTimeOut = INFINITE;
        }
        else {
            dwTimeOut = (DWORD)ui64TimeOutInMillis;
        }
        if ((dwReturnValue = WaitForSingleObject (_hThread, dwTimeOut)) == WAIT_FAILED) {
            return -1;
        }
        else if (dwReturnValue == WAIT_TIMEOUT) {
            return 1;
        }
    #elif defined (UNIX)
        if (!_bIsDetachedMode && _bStarted) {
            if (ui64TimeOutInMillis == 0) {
                /*!!*/  // NOTE: This will not work if the thread is being created
                        // in a detached state
                if (pthread_join (_tid, NULL)) {
                    return -1;
                }
            }
            #if !defined (ANDROID) && !defined (OSX)
                else {
                    int iReturnValue;
                    struct timeval tv;
                    if (gettimeofday (&tv,0) < 0) {
                        return -2;
                    }
                    struct timespec ts;
                    ts.tv_nsec = (ui64TimeOutInMillis%1000UL) * 1000000UL + tv.tv_usec * 1000UL;
                    ts.tv_sec = ui64TimeOutInMillis/1000UL + tv.tv_sec + ts.tv_nsec/1000000000UL;
                    ts.tv_nsec %= 1000000000UL;

                    if ((iReturnValue = pthread_timedjoin_np (_tid, NULL, &ts)) == ETIMEDOUT) {
                        return 1;
                    }
                    else if (iReturnValue) {
                        return -1;
                    }
                }
            #endif
        }
        else if (!_bStarted) {
            return -3;
        }
    #endif

    return 0;
}

float OSThread::getCPUUtilization (void)
{
    #if defined (WIN32)

        FILETIME ftCreation, ftExit, ftKernel, ftUser;
        int64 i64CreationTime, i64KernelTime, i64UserTime, i64CPUTime, i64CurrentTime;

        i64CurrentTime = getTimeInMilliseconds();
        GetThreadTimes (_hThread, &ftCreation, &ftExit, &ftKernel, &ftUser);

        i64CreationTime = ftCreation.dwHighDateTime;
        i64CreationTime <<= 32;
        i64CreationTime |= ftCreation.dwLowDateTime;

        i64KernelTime = ftKernel.dwHighDateTime;
        i64KernelTime <<= 32;
        i64KernelTime |= ftKernel.dwLowDateTime;

        i64UserTime = ftUser.dwHighDateTime;
        i64UserTime <<= 32;
        i64UserTime |= ftUser.dwLowDateTime;

        i64CPUTime = i64UserTime + i64KernelTime;

        if (_i64TimeWhenLastQueriedUtilization == 0) {
            _i64TimeWhenLastQueriedUtilization = i64CurrentTime;
            _i64LastQueriedCPUTime = i64CPUTime;
            return 0.0f;
        }
        else {
            int64 i64CPUTimeConsumed = i64CPUTime - _i64LastQueriedCPUTime;
            float fAverage = (float) (i64CPUTimeConsumed / ((i64CurrentTime - _i64TimeWhenLastQueriedUtilization) * 100.0));
            _i64LastQueriedCPUTime = i64CPUTime;
            _i64TimeWhenLastQueriedUtilization = i64CurrentTime;
            if (fAverage > 100.0) {
                fAverage = 100.0;
            }
            return fAverage;
        }

    #elif defined (UNIX)

        return 0.0f;

    #endif
}

void OSThread::setThreadName (const char *format, ...)
{
    va_list pvar;
    char fmtMsg[256];

    va_start(pvar, format);
    vsnprintf(fmtMsg, 255, format, pvar);
    va_end(pvar);

    fmtMsg[255] = '\0'; // ensure null terminator is present
    #ifdef _MSC_VER
        SetVCDebuggerThreadName(_dwThreadId, fmtMsg);
    #else
        // do nothing right now
    #endif
}

#if defined (UNIX)

    void OSThread::execUserFunc (void *pArg)
    {
        ThreadArg *pTA = (ThreadArg*) pArg;
        pTA->pThis->_pid = getpid();
        #if !defined (ANDROID)
            pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
        #endif
        (pTA->pFn) (pTA->pArg);
        delete pTA;      /*!!*/ // NOTE: This is a memory leak if the thread
                                // cancelled while running, which means the delete
                                // will never be invoked
    }

#endif

#if defined (LINUX)

    float OSThread::getLinuxCPUUtilization (void)
    {
        timeval time;
        static timeval oldtime;
        struct timezone timez;
        static int ut0,st0;
        static bool first=true;
        int ut,st;
        float ret;
        int ej;
        FILE *fd;

        gettimeofday (&time, &timez);
        ej = (time.tv_sec - oldtime.tv_sec) * 100 + (time.tv_usec - oldtime.tv_usec) / 10000;
        oldtime.tv_sec = time.tv_sec;
        oldtime.tv_usec = time.tv_usec;

        static char pfn [12+sizeof(pthread_t)*3];
        if (first) {
            sprintf (pfn,"/proc/%d/stat", _pid);
        }
        if ((fd = fopen(pfn,"r")) == 0) {
            perror ("couldn't open OASIS proc file");
            return -1;
        }
        if (fscanf (fd, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %d %d", &ut,&st) != 2) {
            perror ("couldn't read OASIS proc file");
            fclose (fd);
            return -1;
        }
        if(first) {
            ret = 0.;
            first = false;
        }
        else {
            ret=100.*(ut-ut0+st-st0)/(float)ej;
        }
        ut0 = ut;
        st0 = st;
        fclose (fd);
        return ret>100. ? 100. : (ret<0. ? 0. : ret);
    }

#endif
