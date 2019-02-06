/*
 * Base64.h
 *
 * Binary-to-text encoding schema that represent binary data in an ASCII string format by translating it into a radix-64 representation.
 * This is usefull to leave the data unlikely to be modified in transit through information systems
 *
 * The following printable characters have been selected: 
 * ABCDEFGHIJKLMNOPQRSTUVWXYZ
 * abcdefghijklmnopqrstuvwxyz
 * 0123456789+/
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
    /**
    * Add methods
    * @param bytes_to_encode
    * @param in_len
    * @return A string containing the encoded bytes
    */
    String base64_encode (unsigned char const *bytes_to_encode, unsigned int in_len);
    
    /**
    * @param s: base64 encoded String
    * @return A string containing the decoded bytes
    */
    char * base64_decode (String const &s, unsigned int &buf_len);
}
