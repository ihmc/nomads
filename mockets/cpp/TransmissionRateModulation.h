#ifndef INCL_TRANSMISSION_RATE_MODULATION_H
#define	INCL_TRANSMISSION_RATE_MODULATION_H

/*
 * TransmissionRateModulation.h
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

#include "CongestionControl.h"
#include "Mocket.h"


class TransmissionRateModulation : public CongestionControl
{
    public:
        TransmissionRateModulation (Mocket *pMocket);
        ~TransmissionRateModulation (void);

        void update (void);

    private:
        bool _bInitializationPhase;
};

#endif	/* _TRANSMISSION_RATE_MODULATION_H */
