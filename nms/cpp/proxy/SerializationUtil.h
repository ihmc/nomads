/* 
 * SerializationUtil.h
 *
 * This file is part of the IHMC Network Message Service Library
 * Copyright (c) 1993-2014 IHMC.
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
 * Created on June 16, 2015, 12:25 PM
 */

#ifndef INCL_SERIALIZATION_UTIL_H
#define	INCL_SERIALIZATION_UTIL_H

#include "SimpleCommHelper2.h"

namespace NOMADSUtil
{
    class Reader;
    class Writer;

    char ** readStringArray (Reader *pReader, SimpleCommHelper2::Error &error);
    int writeStringArray (Writer *pWriter, const char **ppszStrings, SimpleCommHelper2::Error &error);
}

#endif	/* INCL_SERIALIZATION_UTIL_H */

