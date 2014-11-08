/*
 * JNIUtils.cpp
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2014 IHMC.
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

#include "JNIUtils.h"

using namespace IHMC_ACI;

#include <string.h>

jobject JNIUtils::createListFromStringArray (JNIEnv *pEnv, const char **ppszStrings)
{
    jclass jcList = pEnv->FindClass ("java/util/ArrayList");
    if (jcList == NULL) {
        return NULL;
    }
    jmethodID jmConstructor = pEnv->GetMethodID (jcList, "<init>", "()V");
    jmethodID jmAdd = pEnv->GetMethodID (jcList, "add", "(Ljava/lang/Object;)Z");
    if ((jmConstructor == NULL) || (jmAdd == NULL)) {
        return NULL;
    }
    jobject joList = pEnv->NewObject (jcList, jmConstructor);
    if (joList == NULL) {
        return NULL;
    }
    if (ppszStrings == NULL) {
        // Empty array
        return joList;
    }
    for (int i = 0; ppszStrings[i] != NULL; i++) {
        jstring jsItem = pEnv->NewStringUTF (ppszStrings[i]);
        if (jsItem == NULL) {
            return NULL;
        }
        jboolean jb = pEnv->CallBooleanMethod (joList, jmAdd, jsItem);
        if (jb == false) {
            pEnv->ThrowNew (pEnv->FindClass ("java/lang/RuntimeException"), "could not add item to list");
            return NULL;
        }
    }
    return joList;
}

jbyteArray JNIUtils::createByteArrayFromBuffer (JNIEnv *pEnv, const void *pBuf, uint32 ui32BufLen)
{
    jbyteArray jba = pEnv->NewByteArray (ui32BufLen);
    if (jba == NULL) {
        return NULL;
    }
    if (ui32BufLen == 0) {
        return jba;
    }
    jbyte *pjb = pEnv->GetByteArrayElements (jba, NULL);
    if (pjb == NULL) {
        return NULL;
    }
    memcpy (pjb, pBuf, ui32BufLen);
    pEnv->ReleaseByteArrayElements (jba, pjb, 0);
    return jba;
}
