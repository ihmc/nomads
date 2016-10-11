/* 
 * SigFaultHanlder.cpp
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
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on February 11, 2015, 1:48 PM
 */

#include "SigFaultHandler.h"

#include <stdlib.h>
#include <string.h>

#ifdef UNIX
    #include <execinfo.h>
    #include <cxxabi.h>
#endif

namespace NOMADSUtil
{
    /* This structure mirrors the one found in /usr/include/asm/ucontext.h */
    #ifdef UNIX
        typedef struct _sig_ucontext {
            unsigned long     uc_flags;
            struct ucontext   *uc_link;
            stack_t           uc_stack;
            struct sigcontext uc_mcontext;
            sigset_t          uc_sigmask;
        } sig_ucontext_t;
    #endif
}

using namespace NOMADSUtil;

FILE * SigFaultHandler::_pOutput = stderr;
String SigFaultHandler::_execName = "executable";

SigFaultHandler::SigFaultHandler (const String execName, FILE *pOutput)
{
    _execName = execName;
    if (pOutput != NULL) {
        _pOutput = pOutput;
    }

    #ifdef UNIX
        signal (SIGPIPE, SIG_IGN);

        struct sigaction sigact;
        sigact.sa_sigaction = SigFaultHandler::handle;
        sigact.sa_flags = SA_RESTART | SA_SIGINFO;

        if (sigaction (SIGSEGV, &sigact, (struct sigaction *)NULL) != 0) {
            fprintf (stderr, "error registering handler for %d (%s)\n",
            SIGSEGV, strsignal (SIGSEGV));
            exit (EXIT_FAILURE);
        }
    #endif
}

SigFaultHandler::~SigFaultHandler()
{
}

void SigFaultHandler::printStackTrace (void **ppTrace, char **ppszMessages, int iSize)
{
    if (SigFaultHandler::_pOutput == NULL) {
        return;
    }

    #ifdef UNIX
        // skip first stack frame (points here)
        for (int i = 1; i < iSize && ppszMessages != NULL; ++i) {
            char *pszMangledName = NULL;
            char *pszOffsetBegin = NULL;
            char *pszOffsetEnd = NULL;

            // find parentheses and +address offset surrounding mangled name
            for (char *p = ppszMessages[i]; *p; ++p) {
                if (*p == '(') {
                    pszMangledName = p; 
                }
                else if (*p == '+')  {
                    pszOffsetBegin = p;
                }
                else if (*p == ')') {
                    pszOffsetEnd = p;
                    break;
                }
            }

            // if the line could be processed, attempt to de-mangle the symbol
            if (pszMangledName && pszOffsetBegin && pszOffsetEnd && pszMangledName < pszOffsetBegin) {
                *pszMangledName++ = '\0';
                *pszOffsetBegin++ = '\0';
                *pszOffsetEnd++ = '\0';

                int status;
                char *real_name = abi::__cxa_demangle (pszMangledName, 0, 0, &status);
                if (status == 0) {
                    // if de-mangling is successful, output the de-mangled function name
                    fprintf (SigFaultHandler::_pOutput, "[bt]: (%d) %s: %s + %s %s\n", i, ppszMessages[i], real_name, pszOffsetBegin, pszOffsetEnd);
                }
                else {
                    // otherwise, output the mangled function name
                    fprintf (SigFaultHandler::_pOutput, "[bt]: (%d) %s: %s + %s %s\n", i, ppszMessages[i], pszMangledName, pszOffsetBegin, pszOffsetEnd);
                }

                char syscom[256];
                sprintf (syscom, "addr2line %p -e %s", ppTrace[i], _execName.c_str());
                if (system (NULL)) {
                    system (syscom);
                }

                if (real_name != NULL) {
                    free (real_name);
                }
            }
            else {
                fprintf (stderr, "[bt]: (%d) %s\n", i, ppszMessages[i]);
            }   
        }

        if (SigFaultHandler::_pOutput != NULL) {
            fflush (SigFaultHandler::_pOutput);
        }
    #endif
}

void SigFaultHandler::handle (int sig_num, siginfo_t * info, void * ucontext)
{    
    // Inspired by from http://stackoverflow.com/questions/77005/how-to-generate-a-stacktrace-when-my-gcc-c-app-crashes
    sig_ucontext_t *   uc = (sig_ucontext_t *)ucontext;

    /* Get the address at the time the signal was raised */
    void *caller_address;
    #if defined (ANDROID)
        caller_address = (void *) uc->uc_mcontext.arm_pc; // ARM specific
    #elif defined(__i386__) // gcc specific
        caller_address = (void *) uc->uc_mcontext.eip; // EIP: x86 specific
    #elif defined(__x86_64__) // gcc specific
        caller_address = (void *) uc->uc_mcontext.rip; // RIP: x86_64 specific
    #else
        #error Unsupported architecture. // TODO: Add support for other arch.
    #endif

    #if defined (UNIX)
        fprintf (stderr, "signal %d (%s), address is %p from %p\n", sig_num,
                 strsignal(sig_num), info->si_addr, (void *) caller_address);

        void *trace[50];
        int iSize = backtrace (trace, 50);

        // overwrite sigaction with caller's address
        trace[1] = caller_address;
        char **ppszMessages = backtrace_symbols (trace, iSize);
        if (ppszMessages != NULL) {
            printStackTrace (trace, ppszMessages, iSize);
            free (ppszMessages);
        }
    #endif

    exit (EXIT_FAILURE);
}

