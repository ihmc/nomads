/*
 * UUID.h
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
 */

#ifndef INCL_UUID_H
#define INCL_UUID_H

namespace NOMADSUtil
{
    class UUID
    {
        public:
            UUID (void);
            UUID (const unsigned char *puchUUID);
            UUID (const UUID &src);

            int generate (void);

            const unsigned char * get (void);

            void set (const unsigned char *puchUUID);

            const char * getAsString (void);

            bool operator == (const UUID &rhsUUID);

            UUID & operator = (const UUID &src);

        private:
            unsigned char auchUUID[16];
            //char szUUID[33];
            // the size of this buffer has been changed as workaround
            // in order to accomodate some "random generated char"
            char szUUID[50];
    };

    inline const unsigned char * UUID::get (void)
    {
        return auchUUID;
    }
    
    inline const char * UUID::getAsString (void)
    {
        return szUUID;
    }
}

#endif   // #ifndef INCL_UUID_H
