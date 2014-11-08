/*
 * StreamServerMocketWrapper.h
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#ifndef INCL_STREAM_SERVER_MOCKET_WRAPPER
#define INCL_STREAM_SERVER_MOCKET_WRAPPER

#include "StreamServerMocket.h"

#include "jni.h"

#ifdef __cplusplus
    extern "C" {
#endif

JNIEXPORT void JNICALL Java_us_ihmc_mockets_StreamServerMocket_init (JNIEnv *pEnv, jobject joThis);
JNIEXPORT void JNICALL Java_us_ihmc_mockets_StreamServerMocket_listen (JNIEnv *pEnv, jobject joThis);
JNIEXPORT jobject JNICALL Java_us_ihmc_mockets_StreamServerMocket_accept (JNIEnv *pEnv, jobject joThis);
JNIEXPORT void JNICALL Java_us_ihmc_mockets_StreamServerMocket_close (JNIEnv *pEnv, jobject joThis);
JNIEXPORT void JNICALL Java_us_ihmc_mockets_StreamServerMocket_dispose (JNIEnv *pEnv, jobject joThis);

#ifdef __cplusplus
    }
#endif


#endif //INCL_STREAM_SERVER_MOCKET_WRAPPER

