/*
 * UtilWrapper.h
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#ifndef INCL_UTIL_WRAPPER_H
#define INCL_UTIL_WRAPPER_H

#include "jni.h"

class UtilWrapper
{
    public:
        static jlong toJLong (void *p);
        static void * toPtr (jlong jlValue);
};

inline jlong UtilWrapper::toJLong (void *p)
{
    union {
        jlong jlValue;
        void *p;
    } u;
    u.p = p;
    return u.jlValue;
}

inline void * UtilWrapper::toPtr (jlong jlValue)
{
    union {
        jlong jlValue;
        void *p;
    } u;
    u.jlValue = jlValue;
    return u.p;
}

#endif 
