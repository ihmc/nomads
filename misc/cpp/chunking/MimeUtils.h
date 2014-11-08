/*
 * MimeUtils.h
 *
 * This file is part of the IHMC Misc Library
 * Copyright (c) 2010-2014 IHMC.
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

namespace IHMC_MISC
{
    class MimeUtils
    {
        public:
            static const char * DEFAULT_MIME_TYPE;

            /**
             * NOTE: the caller has to deallocate the returned string.
             */
            static char * getMimeType (const char *pszFileName);
            static char * getMimeType (FILE *pFile);
            static char * getMimeType (const void *pBuf, uint64 ui64Len);
            static Chunker::Type mimeTypeToFragmentType (const char *pszMimeType);

        private:
            /*static magic_t getPredictor (void);
            static void releasePredictor (magic_t predictor);*/
    };
}

#endif	// #ifndef INCL_MIME_UTILS_H

