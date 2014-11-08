/*
 * StatisticsWrapper.h
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#ifndef INCL_STATISTICS_WRAPPER_H
#define INCL_STATISTICS_WRAPPER_H

#include "jni.h"

#ifdef __cplusplus
    extern "C" {
#endif

JNIEXPORT jlong JNICALL Java_us_ihmc_mockets_Mocket_00024Statistics_getRetransmittedPacketCount (JNIEnv *pEnv, jobject joThis);
JNIEXPORT jlong JNICALL Java_us_ihmc_mockets_Mocket_00024Statistics_getSentPacketCount (JNIEnv *pEnv, jobject joThis);
JNIEXPORT jlong JNICALL Java_us_ihmc_mockets_Mocket_00024Statistics_getSentByteCount (JNIEnv *pEnv, jobject joThis);
JNIEXPORT jlong JNICALL Java_us_ihmc_mockets_Mocket_00024Statistics_getReceivedPacketCount (JNIEnv *pEnv, jobject joThis);
JNIEXPORT jlong JNICALL Java_us_ihmc_mockets_Mocket_00024Statistics_getReceivedByteCount (JNIEnv *pEnv, jobject joThis);
//JNIEXPORT jlong JNICALL Java_us_ihmc_mockets_Mocket_00024Statistics_getDiscardedPacketCount (JNIEnv *pEnv, jobject joThis);
JNIEXPORT jlong JNICALL Java_us_ihmc_mockets_Mocket_00024Statistics_getDuplicatedDiscardedPacketCount (JNIEnv *pEnv, jobject joThis);
JNIEXPORT jlong JNICALL Java_us_ihmc_mockets_Mocket_00024Statistics_getNoRoomDiscardedPacketCount (JNIEnv *pEnv, jobject joThis);

#ifdef __cplusplus
    }
#endif

#endif   // #ifndef INCL_STATISTICS_WRAPPER_H

