/*
 * StreamMocketWrapper.h
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#ifndef INCL_STREAM_MOCKET_WRAPPER_H
#define INCL_STREAM_MOCKET_WRAPPER_H

#include "jni.h"

#ifdef __cplusplus
    extern "C" {
#endif

JNIEXPORT void JNICALL Java_us_ihmc_mockets_StreamMocket_init (JNIEnv *pEnv, jobject joThis);
JNIEXPORT void JNICALL Java_us_ihmc_mockets_StreamMocket_dispose (JNIEnv *pEnv, jobject joThis);
JNIEXPORT void JNICALL Java_us_ihmc_mockets_StreamMocket_setDataBufferingTime (JNIEnv *pEnv, jobject joThis, jint jiMillis);
JNIEXPORT void JNICALL Java_us_ihmc_mockets_StreamMocket_bind (JNIEnv *pEnv, jobject joThis, jstring jsAddress, jint jiPort);
JNIEXPORT void JNICALL Java_us_ihmc_mockets_StreamMocket_close (JNIEnv *pEnv, jobject joThis);
JNIEXPORT void JNICALL Java_us_ihmc_mockets_StreamMocket_connect (JNIEnv *pEnv, jobject joThis, jstring jsHost, jint jiPort, jint jiTimeout);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_StreamMocket_getDataBufferingTime (JNIEnv *pEnv, jobject joThis);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_StreamMocket_getLocalPort (JNIEnv *pEnv, jobject joThis);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_StreamMocket_getRemotePort (JNIEnv *pEnv, jobject joThis);
JNIEXPORT jstring JNICALL Java_us_ihmc_mockets_StreamMocket_getLocalAddress (JNIEnv *pEnv, jobject joThis);
JNIEXPORT jstring JNICALL Java_us_ihmc_mockets_StreamMocket_getRemoteAddress (JNIEnv *pEnv, jobject joThis);
JNIEXPORT void JNICALL Java_us_ihmc_mockets_StreamMocket_registerStatusListener (JNIEnv *pEnv, jobject joThis, jobject joListener);

#ifdef __cplusplus
    }
#endif

#endif //INCL_STREAM_MOCKET_WRAPPER_H

