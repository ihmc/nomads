/*
 * MimeUtils.h
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
 * Created on July 27, 2011, 11:57 PM
 */

#ifndef INCL_MIME_UTILS_H
#define	INCL_MIME_UTILS_H

#include "Chunker.h"

#include "StrClass.h"

#include <stdio.h>

namespace IHMC_MISC
{
    class MimeUtils
    {
        public:
            static const char * DEFAULT_MIME_TYPE;

            static NOMADSUtil::String getMimeType (const char *pszFileName);
            static Chunker::Type mimeTypeToFragmentType (const char *pszMimeType);
            static Chunker::Type toType (const NOMADSUtil::String &extension);
            static NOMADSUtil::String toExtesion (Chunker::Type extension);
            static NOMADSUtil::String toMimeType (Chunker::Type extension);
    };
}

#endif	// #ifndef INCL_MIME_UTILS_H

