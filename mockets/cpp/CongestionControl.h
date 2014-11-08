#ifndef INCL_CONGESTION_CONTROL_H
#define	INCL_CONGESTION_CONTROL_H

/*
 * CongestionControl.h
 * 
 * This file is part of the IHMC Mockets Library/Component
 * Copyright (c) 2002-2014 IHMC.
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

#include "Mocket.h"


class CongestionControl
{
    public :
        CongestionControl (void);
        CongestionControl (Mocket *pMocket);
        virtual void update (void) =0;
        virtual uint32 adaptToCongestionWindow (uint32 ui32SpaceAvailable);
        virtual void reactToLosses (uint8 ui8Code);

    protected:
        Mocket *_pMocket;

};

inline CongestionControl::CongestionControl (void)
{

}

inline CongestionControl::CongestionControl (Mocket *pMocket)
{
    _pMocket = pMocket;
}

inline uint32 CongestionControl::adaptToCongestionWindow (uint32 ui32SpaceAvailable)
{
    return ui32SpaceAvailable;
}

inline void CongestionControl::reactToLosses (uint8 ui8Code)
{
    return;
}

#endif	/* _CONGESTION_CONTROL_H */
