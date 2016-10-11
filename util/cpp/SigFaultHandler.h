/* 
 * SigFaultHanlder.h
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

#ifndef INCL_SIG_FAULT_HANLDER_H
#define	SIGFAULTHANLDER_H

#include <stdio.h>
#include <signal.h>

#include "StrClass.h"

namespace NOMADSUtil
{
    class SigFaultHandler
    {
        public:
            /**
             * If pOutput is set to NULL, standard error will be used
             */
            SigFaultHandler (const String execName, FILE *pOutput = NULL);
            ~SigFaultHandler (void);

        private:
            static void handle (int sig_num, siginfo_t * info, void * ucontext);
            static void printStackTrace (void **ppTrace, char **ppszMessages, int iSize);

        private:
            static FILE *_pOutput;
            static String _execName;

    };
}

#endif	/* SIGFAULTHANLDER_H */

