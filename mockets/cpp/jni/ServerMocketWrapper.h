/*
 * ServerMocketWrapper.h
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#ifndef INCL_SERVER_MOCKET_WRAPPER_H
#define INCL_SERVER_MOCKET_WRAPPER_H

#include "jni.h"

#ifdef __cplusplus
    extern "C" {
#endif

JNIEXPORT void JNICALL Java_us_ihmc_mockets_ServerMocket_init (JNIEnv *pEnv, jobject joThis, jstring jsConfigFile);
JNIEXPORT void JNICALL Java_us_ihmc_mockets_ServerMocket_initDtls (JNIEnv *pEnv, jobject joThis, jstring jsConfigFile, jstring jsCertificatePath, jstring jsPrivateKeyPath);
JNIEXPORT void JNICALL Java_us_ihmc_mockets_ServerMocket_dispose (JNIEnv *pEnv, jobject joThis);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_ServerMocket_listen (JNIEnv *pEnv, jobject joThis, jint jiPort);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_ServerMocket_listenAddr (JNIEnv *pEnv, jobject joThis, jint jiPort, jstring jsListenAddr);
JNIEXPORT jobject JNICALL Java_us_ihmc_mockets_ServerMocket_acceptNoPort (JNIEnv *pEnv, jobject joThis);
JNIEXPORT jobject JNICALL Java_us_ihmc_mockets_ServerMocket_acceptWPort (JNIEnv *pEnv, jobject joThis, jint jiPort);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_ServerMocket_close (JNIEnv *pEnv, jobject joThis);
JNIEXPORT void JNICALL Java_us_ihmc_mockets_ServerMocket_setIdentifier (JNIEnv *pEnv, jobject joThis, jstring jsIdentifier);
JNIEXPORT jstring JNICALL Java_us_ihmc_mockets_ServerMocket_getIdentifier (JNIEnv *pEnv, jobject joThis);

#ifdef __cplusplus
    }
#endif

#endif   // #ifndef INCL_SERVER_MOCKET_WRAPPER_H
