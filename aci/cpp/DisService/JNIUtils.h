/*
 * JNIUtils.h
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2016 IHMC.
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

#ifndef INCL_JNI_UTILS_H
#define INCL_JNI_UTILS_H

#include "jni.h"

#include "FTypes.h"

namespace IHMC_ACI
{

    class JNIUtils
    {
        public:
            static jlong toJLong (void *p);
            static void * toPtr (jlong jlValue);
            static jobject createListFromStringArray (JNIEnv *pEnv, const char **ppszStrings);
            static jbyteArray createByteArrayFromBuffer (JNIEnv *pEnv, const void *pBuf, uint32 ui32BufLen);
    };

    inline jlong JNIUtils::toJLong (void *p)
    {
        union {
            jlong jlValue;
            void *p;
        } u;
        u.p = p;
        return u.jlValue;
    }

    inline void * JNIUtils::toPtr (jlong jlValue)
    {
        union {
            jlong jlValue;
            void *p;
        } u;
        u.jlValue = jlValue;
        return u.p;
    }

}

#endif   // #ifndef INCL_JNI_UTILS_H
