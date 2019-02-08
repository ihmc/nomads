/*
 * Defs.h
 *
 * This file is part of the IHMC Misc Library
 * Copyright (c) 2010-2016 IHMC.
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
 * Created on July 26, 2011, 6:35 PM
 */

#ifndef INCL_DEFS_H
#define INCL_DEFS_H

#define checkAndLogMsg if (pLogger) pLogger->logMsg
#define memoryExhausted Logger::L_Warning, "Memory exhausted.\n"

#define serializationError Logger::L_Warning, "Serialization error.\n"
#define deserializationError Logger::L_Warning, "Deserialization error.\n"

//snprintf is not part of C89. It's standard only in C99. Microsoft does not support C99.
#if _MSC_VER<1900
    #define snprintf _snprintf
#endif

#endif	// INCL_DEFS_H

