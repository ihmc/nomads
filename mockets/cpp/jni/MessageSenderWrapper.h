/*
 * MessageSenderWrapper.h
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#ifndef INCL_MESSAGE_SENDER_WRAPPER_H
#define INCL_MESSAGE_SENDER_WRAPPER_H

#include "jni.h"

#ifdef __cplusplus
    extern "C" {
#endif

JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_00024Sender_dispose (JNIEnv *pEnv, jobject joThis);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_00024Sender_sendNative (JNIEnv *pEnv, jobject joThis, jbyteArray jbABuffer,
                                                                      jint jiOffset, jint jiLength);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_00024Sender_gsend2Args (JNIEnv *pEnv, jobject joThis, jbyteArray jbABuffer1,
                                                                      jint jiOffset1, jint jiLength1, jbyteArray jbABuffer2,
                                                                      jint jiOffset2, jint jiLength2);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_00024Sender_gsend3Args (JNIEnv *pEnv, jobject joThis, jbyteArray jbABuffer1,
                                                                      jint jiOffset1, jint jiLength1, jbyteArray jbABuffer2,
                                                                      jint jiOffset2, jint jiLength2, jbyteArray jbABuffer3,
                                                                      jint jiOffset3, jint jiLength3);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_00024Sender_gsend4Args (JNIEnv *pEnv, jobject joThis, jbyteArray jbABuffer1,
                                                                      jint jiOffset1, jint jiLength1, jbyteArray jbABuffer2,
                                                                      jint jiOffset2, jint jiLength2, jbyteArray jbABuffer3,
                                                                      jint jiOffset3, jint jiLength3, jbyteArray jbABuffer4,
                                                                      jint jiOffset4, jint jiLength4);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_00024Sender_sendTagPrio (JNIEnv *pEnv,jobject joThis, jbyteArray jbABuffer,
                                                                      jint jiOffset, jint jiLength, jint jiTag, jshort jsPriority);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_00024Sender_sendParams (JNIEnv *pEnv, jobject joThis, jbyteArray jbABuffer,
                                                                      jint jiOffset, jint jiLength, jobject joParamsParameter);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_00024Sender_sendTagPrioEnqueRetr (JNIEnv *pEnv, jobject joThis, jbyteArray jbABuffer,
                                                                      jint jiOffset, jint jiLength, jint jiTag, jshort jsPriority,
                                                                      jlong jlEnqueueTimeout, jlong jlRetryTimeout);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_00024Sender_replaceNative (JNIEnv *pEnv, jobject joThis, jbyteArray jbABuffer,
                                                                      jint jiOffset, jint jiLength, jint jiOldTag, jint jiNewTag);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_00024Sender_replaceTagPar (JNIEnv *pEnv, jobject joThis, jbyteArray jbABuffer,
                                                                      jint jiOffset, jint jiLength, jint jiOldTag, jobject joParamsParameter);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_00024Sender_replaceTagPrioEnqueRetr (JNIEnv *pEnv, jobject joThis, jbyteArray jbABuffer,
                                                                      jint jiOffset, jint jiLength, jint jiOldTag, jint jiNewTag,
                                                                      jshort jsPriority, jlong jlEnqueueTimeout, jlong jlRetryTimeout);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_00024Sender_cancel (JNIEnv *pEnv, jobject joThis, jint jiTagId);
JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_00024Sender_setDefaultEnqueueTimeout (JNIEnv *pEnv, jobject joThis, jlong jlEnqueueTimeout);
JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_00024Sender_setDefaultRetryTimeout (JNIEnv *pEnv, jobject joThis, jlong jlRetryTimeout);

#ifdef __cplusplus
    }
#endif

#endif   // #ifndef INCL_MESSAGE_SENDER_WRAPPER_H

