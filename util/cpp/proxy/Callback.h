/* 
 * Callback.h
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
 * Author: Giacomo Benincasa   (gbenincasa@ihmc.us)
 * Created on February 18, 2015, 1:20 AM
 */

#ifndef INCL_CALLBACK_H
#define	INCL_CALLBACK_H

#include "StrClass.h"

namespace NOMADSUtil
{
    class SimpleCommHelper2;

    class Callback
    {
        public:
            Callback (const String &cbackName, const String &cbackRegistrationName);
            virtual ~Callback (void);

            virtual int marshal (SimpleCommHelper2 *pCommHelper) = 0;

        public:
            const String _cbackName;
            const String _cbackRegistrationName;
    };

    class CallbackExecutor
    {
        public:
            // Marshal the callback
            virtual int doCallback (Callback *pCBack) = 0;
    };

    inline Callback::Callback (const String &cbackName, const String &_cbackRegistrationName)
        : _cbackName (cbackName), _cbackRegistrationName (_cbackRegistrationName)
    {
    }

    inline Callback::~Callback (void)
    {
    }
}


#endif	/* INCL_CALLBACK_H */

