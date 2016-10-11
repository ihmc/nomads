/* 
 * StubCallbackHandler.h
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
 * Created on February 27, 2015, 2:58 PM
 */

#ifndef INCL_STUB_CALLBACK_HANDLER_H
#define	INCL_STUB_CALLBACK_HANDLER_H

#include "Unmarshaller.h"

namespace NOMADSUtil
{
    class Stub;
    class SimpleCommHelper2;

    class StubCallbackHandler : public ManageableThread
    {
        public:
            StubCallbackHandler (Stub *pStub, SimpleCommHelper2 *pCommHelper, StubUnmarshalFnPtr pUnmarshaller);
            virtual ~StubCallbackHandler (void);

            void run (void);

            virtual void requestTermination (void);
            virtual void requestTerminationAndWait (void);

        private:
            StubUnmarshalFnPtr _pUnmarshaller;
            Stub *_pStub;
            SimpleCommHelper2 *_pCommHelper;
    };
}

#endif	/* INCL_STUB_CALLBACK_HANDLER_H */

