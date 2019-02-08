/*
 * SessionIdChecker.h
 *
 * This file is part of the IHMC DSPro Library/Component
 * Copyright (c) 2008-2016 IHMC.
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
 * Created on January 30, 2014, 2:08 AM
 */

#ifndef INCL_SESSION_ID_CHECKER_H
#define INCL_SESSION_ID_CHECKER_H

#include "StrClass.h"

namespace IHMC_ACI
{
    bool checkSessionId (const char *pszIncomingMsgSenderNodeId);
}

#endif    /* INCL_SESSION_ID_CHECKER_H */
