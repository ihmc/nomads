/*
 * File.h
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

#include "StrClass.h"

namespace NOMADSUtil
{

    class File
    {
        public:
            File (const String &path);
            File (const String &parentDir, const String &fileName);
            ~File (void);

            bool exists (void) const;
            int64 getFileSize (void) const;

            String getExtension (void) const;

            /**
             * Returns the name of the file or directory denoted by this
             * abstract pathname.
             */
            String getName (bool bExcludeExtension = false) const;

            /**
             * Returns the pathname string of this abstract pathname's parent,
             * or null if this pathname does not name a parent directory.
             */
            String getParent (void) const;

            String getPath (void) const;

        private:
            const String _parentDir;
            const String _fileName;
    };
}

