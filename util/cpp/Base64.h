/*
 * Base64.h
 *
 * This file is part of the IHMC Utility Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#include "StrClass.h"

namespace NOMADSUtil
{
    String base64_encode (unsigned char const *bytes_to_encode, unsigned int in_len);
    String base64_decode (String const &s);
}
