/*
 * MessageMocketWrapper.h
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#ifndef INCL_MSG_MOCKET_WRAPPER_H
#define INCL_MSG_MOCKET_WRAPPER_H

#include "jni.h"

#ifdef __cplusplus
    extern "C" {
#endif


JNIEXPORT jint JNICALL JNI_OnLoad (JavaVM *pJvm, void * pReserved);
        
JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_init (JNIEnv *pEnv, jobject joThis, jstring jsConfigFile);
JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_dispose (JNIEnv *pEnv, jobject joThis);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_connectNative (JNIEnv *pEnv, jobject joThis, jstring jsRemoteHost,
                                                                   jint jiRemotePort);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_connectAsyncNative (JNIEnv *pEnv, jobject joThis, jstring jsRemoteHost,
                                                                   jint jiRemotePort);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_connectTimeout (JNIEnv *pEnv, jobject joThis, 
                                                                          jstring jiRemoteHost, jint jiRemotePort, jlong jlTimeout);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_connectExchangeKeysTimeout (JNIEnv *pEnv, jobject joThis,
                                                                          jstring jiRemoteHost, jint jiRemotePort, jboolean jbPreExchangeKeys, jlong jlTimeout);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_finishConnectNative (JNIEnv *pEnv, jobject joThis);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_bindNative (JNIEnv *pEnv, jobject joThis, jstring jsRemoteHost, jint jiRemotePort);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_reEstablishConn (JNIEnv *pEnv, jobject joThis);
JNIEXPORT jlong JNICALL Java_us_ihmc_mockets_Mocket_getRemoteAddress (JNIEnv *pEnv, jobject joThis);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_getRemotePort (JNIEnv *pEnv, jobject joThis);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_close (JNIEnv *pEnv, jobject joThis);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_suspend (JNIEnv *pEnv, jobject joThis);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_getStateNative (JNIEnv *pEnv, jobject joThis, jobject joOutputStream);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_enableCrossSequecing (JNIEnv *pEnv, jobject joThis,
                                                                               jboolean jbEnable);
JNIEXPORT jboolean JNICALL Java_us_ihmc_mockets_Mocket_isCrossSequencingEnabled (JNIEnv *pEnv, jobject joThis);
JNIEXPORT jobject JNICALL Java_us_ihmc_mockets_Mocket_getSenderNative (JNIEnv *pEnv, jobject joThis,
                                                                        jboolean jbReliable, jboolean jbSequenced,
                                                                        jobject joSender);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_sendNative (JNIEnv *pEnv, jobject joThis,jboolean jbReliable,
                                                                      jboolean jbSequenced, jbyteArray jbABuffer, jint jiOffset,
                                                                      jint jiLength, jint jiTag, jshort jsPriority,
                                                                      jlong jlEnqueueTimeout, jlong jlRetryTimeout);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_gsendNative (JNIEnv *pEnv, jobject joThis,jboolean jbReliable,
                                                                       jboolean jbSequenced, jbyteArray jbABuffer, jint jiOffset,
                                                                       jint jiLength, jint jiTag, jshort jsPriority,
                                                                       jlong jlEnqueueTimeout, jlong jlRetryTimeout,
                                                                       jstring jsValist1, jstring jsValist2);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_getNextMessageSize (JNIEnv *pEnv, jobject joThis, jlong jTimeout);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_receiveNative (JNIEnv *pEnv, jobject joThis, jbyteArray jbABuffer,
                                                                         jint jiOffset, jint jiLength, jlong jTimeout);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_sreceive2Args (JNIEnv *pEnv, jobject joThis, jbyteArray jbABuffer1,
                                                                      jint jiOffset1, jint jiLength1, jbyteArray jbABuffer2,
                                                                      jint jiOffset2, jint jiLength2, jlong jlTimeout);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_sreceive3Args (JNIEnv *pEnv, jobject joThis, jbyteArray jbABuffer1,
                                                                      jint jiOffset1, jint jiLength1, jbyteArray jbABuffer2,
                                                                      jint jiOffset2, jint jiLength2, jbyteArray jbABuffer3,
                                                                      jint jiOffset3, jint jiLength3, jlong jlTimeout);
JNIEXPORT jbyteArray JNICALL Java_us_ihmc_mockets_Mocket_receiveBuffer (JNIEnv *pEnv, jobject joThis, jlong jlTimeout);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_sreceive4Args (JNIEnv *pEnv, jobject joThis, jbyteArray jbABuffer1,
                                                                      jint jiOffset1, jint jiLength1, jbyteArray jbABuffer2,
                                                                      jint jiOffset2, jint jiLength2, jbyteArray jbABuffer3,
                                                                      jint jiOffset3, jint jiLength3, jbyteArray jbABuffer4,
                                                                      jint jiOffset4, jint jiLength4, jlong jlTimeout);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_replaceNative (JNIEnv *pEnv, jobject joThis, jboolean jbReliable,
                                                                         jboolean jbSequenced, jbyteArray jbABuffer,
                                                                         jint jiOldTag, jint jiOffset, jint jiLength,
                                                                         jint jiNewTag, jshort jsPriority,
                                                                         jlong jlEnqueueTimeout, jlong jlRetryTimeout);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_cancel (JNIEnv *pEnv, jobject joThis, jboolean jbReliable,
                                                                  jboolean jbSequenced, jint jiTagId);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_setConnectionLingerTime (JNIEnv *pEnv, jobject joThis, jlong jlLingerTime);
JNIEXPORT jlong JNICALL Java_us_ihmc_mockets_Mocket_getConnectionLingetTime (JNIEnv *pEnv, jobject joThis);
JNIEXPORT jobject JNICALL Java_us_ihmc_mockets_Mocket_getStatisticsNative (JNIEnv *pEnv, jobject joThis, jobject joStatistics);
JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_registerStatusListener (JNIEnv *pEnv, jobject joThis, jobject joListener);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_getMaximumMTU (JNIEnv *pEnv, jobject joThis);
JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_getPeerNameNative (JNIEnv *pEnv, jobject joThis);
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_activateCongestionControl (JNIEnv *pEnv, jobject joThis);
JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_debugStateCapture (JNIEnv *pEnv, jobject joThis);

JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_useTwoWayHandshake (JNIEnv *pEnv, jobject joThis);
//JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_useTransmissionRateModulation (JNIEnv *pEnv, jobject joThis);
JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_setKeepAliveTimeout (JNIEnv *pEnv, jobject joThis, jint jiTimeout);
JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_disableKeepAlive (JNIEnv *pEnv, jobject joThis);
JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_setInitialAssumedRTT (JNIEnv *pEnv, jobject joThis, jlong jlRTT);
JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_setMaximumRTO (JNIEnv *pEnv, jobject joThis, jlong jlRTO);
JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_setMinimumRTO (JNIEnv *pEnv, jobject joThis, jlong jlRTO);
JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_setRTOFactor (JNIEnv *pEnv, jobject joThis, jdouble jdRTOFactor);
JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_setRTOConstant (JNIEnv *pEnv, jobject joThis, jint jiRTOConst);
JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_disableRetransmitCountFactorInRTO (JNIEnv *pEnv, jobject joThis);
JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_setMaximumWindowSize (JNIEnv *pEnv, jobject joThis, jlong jlWindSize);
JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_setSAckTransmitTimeout (JNIEnv *pEnv, jobject joThis, jint jiSackTransTO);
JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_setConnectTimeout (JNIEnv *pEnv, jobject joThis, jlong jlConnectTO);
JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_setUDPReceiveConnectionTimeout (JNIEnv *pEnv, jobject joThis, jint jiUDPConnectTO);
JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_setUDPReceiveTimeout (JNIEnv *pEnv, jobject joThis, jint jiUDPRecTO);

JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_setIdentifier (JNIEnv *pEnv, jobject joThis, jstring jsIdentifier);
JNIEXPORT jstring JNICALL Java_us_ihmc_mockets_Mocket_getIdentifier (JNIEnv *pEnv, jobject joThis);

//JNIEXPORT jobject JNICALL Java_us_ihmc_mockets_Mocket_getNotification (JNIEnv *pEnv, jobject joThis);
//JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_abortConnection (JNIEnv *pEnv, jobject joThis);


#ifdef __cplusplus
    }
#endif

#endif   // #ifndef INCL_MSG_MOCKET_WRAPPER_H

