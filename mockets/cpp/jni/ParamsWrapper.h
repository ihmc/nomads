/*
 * ParamsWrapper.h
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#ifndef INCL_PARAMS_WRAPPER_H
#define INCL_PARAMS_WRAPPER_H

#include "jni.h"

#ifdef __cplusplus
    extern "C" {
#endif


JNIEXPORT void JNICALL Java_us_ihmc_mockets_Params_init (JNIEnv *pEnv, jobject joThis, jint jiTag, jshort jsPriority,
                                                         jlong jlEnqueueTimeout, jlong jlRetryTimeout);
JNIEXPORT void JNICALL Java_us_ihmc_mockets_Params_dispose (JNIEnv *pEnv, jobject joThis);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Params_getTag (JNIEnv *pEnv, jobject joThis);
JNIEXPORT jshort JNICALL Java_us_ihmc_mockets_Params_getPriority (JNIEnv *pEnv, jobject joThis);
JNIEXPORT jlong JNICALL Java_us_ihmc_mockets_Params_getEnqueueTimeout (JNIEnv *pEnv, jobject joThis);
JNIEXPORT jlong JNICALL Java_us_ihmc_mockets_Params_getRetryTimeout (JNIEnv *pEnv, jobject joThis);

#ifdef __cplusplus
    }
#endif

#endif   // #ifndef INCL_PARAMS_WRAPPER_H

