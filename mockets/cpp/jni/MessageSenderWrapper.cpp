/*
 * MessageSenderWrapper.cpp
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#include <stdio.h>
#include "MessageSenderWrapper.h"
#include "MessageSender.h"
#include "UtilWrapper.h"


JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_00024Sender_dispose (JNIEnv *pEnv, jobject joThis)
{
    jclass jcMessageSender = pEnv->GetObjectClass (joThis);
    jfieldID jfMessageSender = pEnv->GetFieldID (jcMessageSender, "_messageSender", "J");
    jlong jlMessageSender = pEnv->GetLongField (joThis, jfMessageSender);
    MessageSender *pSender = (MessageSender *) UtilWrapper::toPtr (jlMessageSender);
    if (pSender == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MessageSenderWrapper not initialized - sender is null");
        return;
    }
    delete pSender;
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_00024Sender_sendNative (JNIEnv *pEnv, jobject joThis, jbyteArray jbABuffer,
                                                                           jint jiOffset, jint jiLength)
{
    jclass jcMessageSender = pEnv->GetObjectClass (joThis);
    jfieldID jfMessageSender = pEnv->GetFieldID (jcMessageSender, "_messageSender", "J");
    jlong jlMessageSender = pEnv->GetLongField (joThis, jfMessageSender);
    MessageSender *pSender = (MessageSender *) UtilWrapper::toPtr (jlMessageSender);
    if (pSender == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MessageSenderWrapper not initialized - sender is null");
        return -10;
    }
    jbyte *pjbBuffer = pEnv->GetByteArrayElements (jbABuffer, NULL);
    if (pjbBuffer == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MessageSenderWrapper not initialized - buffer is null");
        pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
        return -11;
    }
    pjbBuffer = pjbBuffer + jiOffset;
    int rc = pSender->send (pjbBuffer, jiLength);
    if (rc < 0) {
        char szErrMsg [80];
        sprintf (szErrMsg, "send failed with rc = %d; length = %d", rc, jiLength);
        pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), szErrMsg);
        pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
        return rc;
    }
    else if (!rc) {
        pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
        return rc;
    }
    pEnv->ThrowNew (pEnv->FindClass ( "java/io/IOException"), "returned value from send unknown");
    pEnv->ReleaseByteArrayElements(jbABuffer, pjbBuffer, JNI_ABORT);
    return rc;
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_00024Sender_gsend2Args (JNIEnv *pEnv, jobject joThis, jbyteArray jbABuffer1,
                                                                           jint jiOffset1, jint jiLength1, jbyteArray jbABuffer2,
                                                                           jint jiOffset2, jint jiLength2)
{
    jclass jcMessageSender = pEnv->GetObjectClass (joThis);
    jfieldID jfMessageSender = pEnv->GetFieldID (jcMessageSender, "_messageSender", "J");
    jlong jlMessageSender = pEnv->GetLongField (joThis, jfMessageSender);
    MessageSender *pSender = (MessageSender *) UtilWrapper::toPtr (jlMessageSender);
    if (pSender == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MessageSenderWrapper not initialized - sender is null");
        return -1;
    }
    jbyte *pjbBuffer1 = pEnv->GetByteArrayElements (jbABuffer1, NULL);
    if (pjbBuffer1 == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MessageSenderWrapper not initialized - buffer is null");
        pEnv->ReleaseByteArrayElements (jbABuffer1, pjbBuffer1, JNI_ABORT);
        return -1;
    }
    pjbBuffer1 = pjbBuffer1 + jiOffset1;
    jbyte *pjbBuffer2 = pEnv->GetByteArrayElements (jbABuffer2, NULL);
    if (pjbBuffer2 == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MessageSenderWrapper not initialized - buffer is null");
        pEnv->ReleaseByteArrayElements (jbABuffer1, pjbBuffer1, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer2, pjbBuffer2, JNI_ABORT);
        return -1;
    }
    pjbBuffer2 = pjbBuffer2 + jiOffset2;
    int rc = pSender->gsend (pjbBuffer1, jiLength1, pjbBuffer2, jiLength2);
    if (rc < 0) {
        char szErrMsg[80];
        sprintf (szErrMsg, "gsend with 2 buffers failed with rc = %d", rc);
        pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), szErrMsg);
        pEnv->ReleaseByteArrayElements (jbABuffer1, pjbBuffer1, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer2, pjbBuffer2, JNI_ABORT);
        return rc;
    }
    else if (!rc) {
        pEnv->ReleaseByteArrayElements (jbABuffer1, pjbBuffer1, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer2, pjbBuffer2, JNI_ABORT);
        return rc;
    }
    pEnv->ThrowNew(pEnv->FindClass ("java/io/IOException"), "returned value from send unknown");
    pEnv->ReleaseByteArrayElements (jbABuffer1, pjbBuffer1, JNI_ABORT);
    pEnv->ReleaseByteArrayElements (jbABuffer2, pjbBuffer2, JNI_ABORT);
    return rc;
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_00024Sender_gsend3Args (JNIEnv *pEnv, jobject joThis, jbyteArray jbABuffer1,
                                                                           jint jiOffset1, jint jiLength1, jbyteArray jbABuffer2,
                                                                           jint jiOffset2, jint jiLength2, jbyteArray jbABuffer3,
                                                                           jint jiOffset3, jint jiLength3)
{
    jclass jcMessageSender = pEnv->GetObjectClass (joThis);
    jfieldID jfMessageSender = pEnv->GetFieldID (jcMessageSender, "_messageSender", "J");
    jlong jlMessageSender = pEnv->GetLongField (joThis, jfMessageSender);
    MessageSender *pSender = (MessageSender *) UtilWrapper::toPtr (jlMessageSender);
    if (pSender == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MessageSenderWrapper not initialized - sender is null");
        return -1;
    }
    jbyte *pjbBuffer1 = pEnv->GetByteArrayElements (jbABuffer1, NULL);
    if (pjbBuffer1 == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MessageSenderWrapper not initialized - buffer is null");
        pEnv->ReleaseByteArrayElements (jbABuffer1, pjbBuffer1, JNI_ABORT);
        return -1;
    }
    pjbBuffer1 = pjbBuffer1 + jiOffset1;
    jbyte *pjbBuffer2 = pEnv->GetByteArrayElements (jbABuffer2, NULL);
    if (pjbBuffer2 == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MessageSenderWrapper not initialized - buffer is null");
        pEnv->ReleaseByteArrayElements (jbABuffer1, pjbBuffer1, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer2, pjbBuffer2, JNI_ABORT);
        return -1;
    }
    pjbBuffer2 = pjbBuffer2 + jiOffset2;
    jbyte *pjbBuffer3 = pEnv->GetByteArrayElements (jbABuffer3, NULL);
    if (pjbBuffer3 == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MessageSenderWrapper not initialized - buffer is null");
        pEnv->ReleaseByteArrayElements (jbABuffer1, pjbBuffer1, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer2, pjbBuffer2, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer3, pjbBuffer3, JNI_ABORT);
        return -1;
    }
    pjbBuffer3 = pjbBuffer3 + jiOffset3;
    int rc = pSender->gsend (pjbBuffer1, jiLength1, pjbBuffer2, jiLength2, pjbBuffer3, jiLength3);
    if (rc < 0) {
        char szErrMsg[80];
        sprintf (szErrMsg, "gsend with 3 buffers failed with rc = %d", rc);
        pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), szErrMsg);
        pEnv->ReleaseByteArrayElements (jbABuffer1, pjbBuffer1, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer2, pjbBuffer2, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer3, pjbBuffer3, JNI_ABORT);
        return rc;
    }
    else if (!rc) {
        pEnv->ReleaseByteArrayElements (jbABuffer1, pjbBuffer1, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer2, pjbBuffer2, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer3, pjbBuffer3, JNI_ABORT);
        return rc;
    }

    pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), "returned value from send unknown");
    pEnv->ReleaseByteArrayElements (jbABuffer1, pjbBuffer1, JNI_ABORT);
    pEnv->ReleaseByteArrayElements (jbABuffer2, pjbBuffer2, JNI_ABORT);
    pEnv->ReleaseByteArrayElements (jbABuffer3, pjbBuffer3, JNI_ABORT);
    return rc;
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_00024Sender_gsend4Args (JNIEnv *pEnv, jobject joThis, jbyteArray jbABuffer1,
                                                                           jint jiOffset1, jint jiLength1, jbyteArray jbABuffer2,
                                                                           jint jiOffset2, jint jiLength2, jbyteArray jbABuffer3,
                                                                           jint jiOffset3, jint jiLength3, jbyteArray jbABuffer4,
                                                                           jint jiOffset4, jint jiLength4)
{
    jclass jcMessageSender = pEnv->GetObjectClass (joThis);
    jfieldID jfMessageSender = pEnv->GetFieldID (jcMessageSender, "_messageSender", "J");
    jlong jlMessageSender = pEnv->GetLongField (joThis, jfMessageSender);
    MessageSender *pSender = (MessageSender *) UtilWrapper::toPtr (jlMessageSender);
    if (pSender == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MessageSenderWrapper not initialized - sender is null");
        return -1;
    }
    jbyte *pjbBuffer1 = pEnv->GetByteArrayElements (jbABuffer1, NULL);
    if (pjbBuffer1 == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MessageSenderWrapper not initialized - buffer is null");
        pEnv->ReleaseByteArrayElements (jbABuffer1, pjbBuffer1, JNI_ABORT);
        return -1;
    }
    pjbBuffer1 = pjbBuffer1 + jiOffset1;
    jbyte *pjbBuffer2 = pEnv->GetByteArrayElements (jbABuffer2, NULL);
    if (pjbBuffer2 == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MessageSenderWrapper not initialized - buffer is null");
        pEnv->ReleaseByteArrayElements (jbABuffer1, pjbBuffer1, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer2, pjbBuffer2, JNI_ABORT);
        return -1;
    }
    pjbBuffer2 = pjbBuffer2 + jiOffset2;
    jbyte *pjbBuffer3 = pEnv->GetByteArrayElements (jbABuffer3, NULL);
    if (pjbBuffer3 == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MessageSenderWrapper not initialized - buffer is null");
        pEnv->ReleaseByteArrayElements (jbABuffer1, pjbBuffer1, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer2, pjbBuffer2, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer3, pjbBuffer3, JNI_ABORT);
        return -1;
    }
    pjbBuffer3 = pjbBuffer3 + jiOffset3;
    jbyte *pjbBuffer4 = pEnv->GetByteArrayElements (jbABuffer4, NULL);
    if (pjbBuffer4 == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MessageSenderWrapper not initialized - buffer is null");
        pEnv->ReleaseByteArrayElements (jbABuffer1, pjbBuffer1, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer2, pjbBuffer2, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer3, pjbBuffer3, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer4, pjbBuffer4, JNI_ABORT);
        return -1;
    }
    pjbBuffer4 = pjbBuffer4 + jiOffset4;
    int rc = pSender->gsend (pjbBuffer1, jiLength1, pjbBuffer2, jiLength2, pjbBuffer3, jiLength3, pjbBuffer4, jiLength4);
    if (rc < 0) {
        char szErrMsg[80];
        sprintf (szErrMsg, "gsend with 4 buffers failed with rc = %d", rc);
        pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), szErrMsg);
        pEnv->ReleaseByteArrayElements (jbABuffer1, pjbBuffer1, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer2, pjbBuffer2, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer3, pjbBuffer3, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer4, pjbBuffer4, JNI_ABORT);
        return rc;
    }
    else if (!rc) {
        pEnv->ReleaseByteArrayElements (jbABuffer1, pjbBuffer1, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer2, pjbBuffer2, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer3, pjbBuffer3, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer4, pjbBuffer4, JNI_ABORT);
        return rc;
    }
    pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), "returned value from send unknown");
    pEnv->ReleaseByteArrayElements (jbABuffer1, pjbBuffer1, JNI_ABORT);
    pEnv->ReleaseByteArrayElements (jbABuffer2, pjbBuffer2, JNI_ABORT);
    pEnv->ReleaseByteArrayElements (jbABuffer3, pjbBuffer3, JNI_ABORT);
    pEnv->ReleaseByteArrayElements (jbABuffer4, pjbBuffer4, JNI_ABORT);
    return rc;
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_00024Sender_sendTagPrio (JNIEnv *pEnv,jobject joThis, jbyteArray jbABuffer,
                                                                            jint jiOffset, jint jiLength, jint jiTag, jshort jsPriority)
{
    if (jiTag < 0) {
        pEnv->ThrowNew(pEnv->FindClass ("java/lang/IllegalArgumentException"),"illegal argument: Tag cannot be negative");
        return -10;
    }
    if (jsPriority < 0) {
        pEnv->ThrowNew(pEnv->FindClass ("java/lang/IllegalArgumentException"),"illegal argument: Priority cannot be negative");
        return -10;
    }
    jclass jcMessageSender = pEnv->GetObjectClass (joThis);
    jfieldID jfMessageSender = pEnv->GetFieldID (jcMessageSender, "_messageSender", "J");
    jlong jlMessageSender = pEnv->GetLongField (joThis, jfMessageSender);
    MessageSender *pSender = (MessageSender *) UtilWrapper::toPtr (jlMessageSender);
    if (pSender == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MessageSenderWrapper not initialized - sender is null");
        return -11;
    }
    jbyte *pjbBuffer = pEnv->GetByteArrayElements (jbABuffer, NULL);
    if (pjbBuffer == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MessageSenderWrapper not initialized - buffer is null");
        pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
        return -12;
    }
    pjbBuffer = pjbBuffer + jiOffset;
    int rc = pSender->send (pjbBuffer, jiLength, (uint16) jiTag, (uint8) jsPriority);
    if (rc < 0) {
        char szErrMsg[255];
        sprintf (szErrMsg, "send failed with rc = %d; length = %d, tag = %d, priority = %d", rc, jiLength, jiTag, (int) jsPriority);
        pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), szErrMsg);
        pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
        return rc;
    }
    else if (!rc) {
        pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
        return rc;
    }
    pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), "returned value from send unknown");
    pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
    return rc;
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_00024Sender_sendParams (JNIEnv *pEnv, jobject joThis, jbyteArray jbABuffer,
                                                                           jint jiOffset, jint jiLength, jobject joParamsParameter)
{
    jclass jcMessageSender = pEnv->GetObjectClass (joThis);
    jfieldID jfMessageSender = pEnv->GetFieldID (jcMessageSender, "_messageSender", "J");
    jlong jlMessageSender = pEnv->GetLongField (joThis, jfMessageSender);
    MessageSender *pSender = (MessageSender *) UtilWrapper::toPtr (jlMessageSender);
    if (pSender == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MessageSenderWrapper not initialized - sender is null");
        return -10;
    }
    jbyte *pjbBuffer = pEnv->GetByteArrayElements (jbABuffer, NULL);
    if (pjbBuffer == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MessageSenderWrapper not initialized - buffer is null");
        return -11;
    }
    pjbBuffer = pjbBuffer + jiOffset;
    jclass jcParams = pEnv->GetObjectClass (joParamsParameter);
    jfieldID jfParams = pEnv->GetFieldID (jcParams, "_params", "J");
    jint jiParams = pEnv->GetIntField (joParamsParameter, jfParams);
    MessageSender::Params *pParams = (MessageSender::Params *) UtilWrapper::toPtr (jiParams);
    if (pParams == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MessageSenderWrapper not initialized - params is null");
        pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
        return -12;
    }
    int rc = pSender->send (pjbBuffer, jiLength, pParams);
    if (rc < 0) {
        char szErrMsg[80];
        sprintf (szErrMsg, "send with params failed with rc = %di; length = %d", rc, jiLength);
        pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), szErrMsg);
        pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
        return rc;
    }
    else if (!rc) {
        pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
        return rc;
    }
    pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), "returned value from send unknown");
    pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
    return rc;
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_00024Sender_sendTagPrioEnqueRetr (JNIEnv *pEnv, jobject joThis, jbyteArray jbABuffer,
                                                                                     jint jiOffset, jint jiLength, jint jiTag, jshort jsPriority,
                                                                                     jlong jlEnqueueTimeout, jlong jlRetryTimeout)
{
    if (jiTag < 0) {
        pEnv->ThrowNew(pEnv->FindClass ("java/lang/IllegalArgumentException"),"illegal argument: Tag cannot be negative");
        return -10;
    }
    if (jsPriority < 0) {
        pEnv->ThrowNew(pEnv->FindClass ("java/lang/IllegalArgumentException"),"illegal argument: Priority cannot be negative");
        return -10;
    }
    if (jlEnqueueTimeout < 0) {
        pEnv->ThrowNew(pEnv->FindClass ("java/lang/IllegalArgumentException"),"illegal argument: EnqueueTimeout cannot be negative");
        return -10;
    }
    if (jlRetryTimeout < 0) {
        pEnv->ThrowNew(pEnv->FindClass ("java/lang/IllegalArgumentException"),"illegal argument: RetryTimeout cannot be negative");
        return -10;
    }
    jclass jcMessageSender = pEnv->GetObjectClass (joThis);
    jfieldID jfMessageSender = pEnv->GetFieldID (jcMessageSender, "_messageSender", "J");
    jlong jlMessageSender = pEnv->GetLongField (joThis, jfMessageSender);
    MessageSender *pSender = (MessageSender *) UtilWrapper::toPtr (jlMessageSender);
    if (pSender == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MessageSenderWrapper not initialized - sender is null");
        return -11;
    }
    jbyte *pjbBuffer = pEnv->GetByteArrayElements (jbABuffer, NULL);
    if (pjbBuffer == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MessageSenderWrapper not initialized - buffer is null");
        pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
        return -12;
    }
    pjbBuffer = pjbBuffer + jiOffset;
    int rc = pSender->send (pjbBuffer, jiLength, (uint16) jiTag, (uint8) jsPriority, (uint32) jlEnqueueTimeout, (uint32) jlRetryTimeout);
    if (rc < 0) {
        char szErrMsg[255];
        sprintf (szErrMsg, "send failed with rc = %d; length = %d, tag = %d, priority = %d, enqueue timeout = %lu, retry timeout = %lu",
                 rc, jiLength, jiTag, (int) jsPriority, (uint32) jlEnqueueTimeout, (uint32) jlRetryTimeout);
        pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), szErrMsg);
        pEnv->ReleaseByteArrayElements(jbABuffer, pjbBuffer, JNI_ABORT);
        return rc;
    }
    else if (!rc) {
        pEnv->ReleaseByteArrayElements(jbABuffer, pjbBuffer, JNI_ABORT);
        return rc;
    }
    pEnv->ThrowNew(pEnv->FindClass ("java/io/IOException"), "returned value from send unknown");
    pEnv->ReleaseByteArrayElements(jbABuffer, pjbBuffer, JNI_ABORT);
    return rc;
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_00024Sender_replaceNative (JNIEnv *pEnv, jobject joThis, jbyteArray jbABuffer,
                                                                      jint jiOffset, jint jiLength, jint jiOldTag, jint jiNewTag)
{
    if (jiOldTag < 0) {
        pEnv->ThrowNew(pEnv->FindClass ("java/lang/IllegalArgumentException"),"illegal argument: OldTag cannot be negative");
        return -10;
    }
    if (jiNewTag < 0) {
        pEnv->ThrowNew(pEnv->FindClass ("java/lang/IllegalArgumentException"),"illegal argument: NewTag cannot be negative");
        return -10;
    }
    jclass jcMessageSender = pEnv->GetObjectClass (joThis);
    jfieldID jfMessageSender = pEnv->GetFieldID (jcMessageSender, "_messageSender", "J");
    jlong jlMessageSender = pEnv->GetLongField (joThis, jfMessageSender);
    MessageSender *pSender = (MessageSender *) UtilWrapper::toPtr (jlMessageSender);
    if (pSender == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MessageSenderWrapper not initialized - sender is null");
        return -11;
    }
    jbyte *pjbBuffer = pEnv->GetByteArrayElements (jbABuffer, NULL);
    if (pjbBuffer == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MessageSenderWrapper not initialized - buffer is null");
        pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
        return -12;
    }
    pjbBuffer = pjbBuffer + jiOffset;
    int rc = pSender->replace (pjbBuffer, jiLength, (uint16) jiOldTag, (uint16) jiNewTag);
    if (rc < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), "replace failed");
        pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
        return rc;
    }
    else if (!rc) {
        pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
        return rc;
    }
    pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), "returned value from replace unknown");
    pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
    return rc;
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_00024Sender_replaceTagPar (JNIEnv *pEnv, jobject joThis, jbyteArray jbABuffer,
                                                                              jint jiOffset, jint jiLength, jint jiOldTag, jobject joParamsParameter)
{
    if (jiOldTag < 0) {
        pEnv->ThrowNew(pEnv->FindClass ("java/lang/IllegalArgumentException"),"illegal argument: OldTag cannot be negative");
        return -10;
    }
    jclass jcMessageSender = pEnv->GetObjectClass (joThis);
    jfieldID jfMessageSender = pEnv->GetFieldID (jcMessageSender, "_messageSender", "J");
    jlong jlMessageSender = pEnv->GetLongField (joThis, jfMessageSender);
    MessageSender *pSender = (MessageSender *) UtilWrapper::toPtr (jlMessageSender);
    if (pSender == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MessageSenderWrapper not initialized - sender is null");
        return -11;
    }
    jbyte *pjbBuffer = pEnv->GetByteArrayElements (jbABuffer, NULL);
    if (pjbBuffer == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MessageSenderWrapper not initialized - buffer is null");
        pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
        return -12;
    }
    pjbBuffer = pjbBuffer + jiOffset;
    jclass jcParams = pEnv->GetObjectClass (joParamsParameter);
    jfieldID jfParams = pEnv->GetFieldID (jcParams, "_params", "J");
    jlong jlParams = pEnv->GetLongField (joParamsParameter, jfParams);
    MessageSender::Params *pParams = (MessageSender::Params *) UtilWrapper::toPtr (jlParams);
    if (pParams == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MessageSenderWrapper not initialized - params is null");
        return -13;
    }
    int rc = pSender->replace (pjbBuffer, jiLength, (uint16) jiOldTag, pParams);
    if (rc < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), "replace failed");
        pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
        return rc;
    }
    else if (!rc) {
        pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
        return rc;
    }
    pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), "returned value from replace unknown");
    pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
    return rc;
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_00024Sender_replaceTagPrioEnqueRetr (JNIEnv *pEnv, jobject joThis, jbyteArray jbABuffer,
                                                                      jint jiOffset, jint jiLength, jint jiOldTag, jint jiNewTag,
                                                                      jshort jsPriority, jlong jlEnqueueTimeout, jlong jlRetryTimeout)
{
    if (jiOldTag < 0) {
        pEnv->ThrowNew(pEnv->FindClass ("java/lang/IllegalArgumentException"),"illegal argument: OldTag cannot be negative");
        return -10;
    }
    if (jiNewTag < 0) {
        pEnv->ThrowNew(pEnv->FindClass ("java/lang/IllegalArgumentException"),"illegal argument: NewTag cannot be negative");
        return -10;
    }
    if (jsPriority < 0) {
        pEnv->ThrowNew(pEnv->FindClass ("java/lang/IllegalArgumentException"),"illegal argument: Priority cannot be negative");
        return -10;
    }
    if (jlEnqueueTimeout < 0) {
        pEnv->ThrowNew(pEnv->FindClass ("java/lang/IllegalArgumentException"),"illegal argument: EnqueueTimeout cannot be negative");
        return -10;
    }
    if (jlRetryTimeout < 0) {
        pEnv->ThrowNew(pEnv->FindClass ("java/lang/IllegalArgumentException"),"illegal argument: RetryTimeout cannot be negative");
        return -10;
    }
    jclass jcMessageSender = pEnv->GetObjectClass (joThis);
    jfieldID jfMessageSender = pEnv->GetFieldID (jcMessageSender, "_messageSender", "J");
    jlong jlMessageSender = pEnv->GetLongField (joThis, jfMessageSender);
    MessageSender *pSender = (MessageSender *) UtilWrapper::toPtr (jlMessageSender);
    if (pSender == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MessageSenderWrapper not initialized - sender is null");
        return -11;
    }
    jbyte *pjbBuffer = pEnv->GetByteArrayElements (jbABuffer, NULL);
    if (pjbBuffer == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MessageSenderWrapper not initialized - buffer is null");
        pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
        return -12;
    }
    pjbBuffer = pjbBuffer + jiOffset;
    int rc = pSender->replace (pjbBuffer, jiLength, (uint16) jiOldTag, (uint16) jiNewTag, (uint8) jsPriority, (uint32) jlEnqueueTimeout, (uint32) jlRetryTimeout);
    if (rc < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), "replace failed");
        pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
        return rc;
    }
    else if (!rc) {
        pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
        return rc;
    }
    pEnv->ThrowNew(pEnv->FindClass ("java/io/IOException"), "returned value from replace unknown");
    pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
    return rc;
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_00024Sender_cancel (JNIEnv *pEnv, jobject joThis, jint jiTagId)
{
    if (jiTagId < 0) {
        pEnv->ThrowNew(pEnv->FindClass ("java/lang/IllegalArgumentException"),"illegal argument: TagId cannot be negative");
        return -10;
    }
    jclass jcMessageSender = pEnv->GetObjectClass (joThis);
    jfieldID jfMessageSender = pEnv->GetFieldID (jcMessageSender, "_messageSender", "J");
    jlong jlMessageSender = pEnv->GetLongField (joThis, jfMessageSender);
    MessageSender *pSender = (MessageSender *) UtilWrapper::toPtr (jlMessageSender);
    if (pSender == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MessageSenderWrapper not initialized - sender is null");
        return -11;
    }
    return pSender->cancel((uint16) jiTagId);
}

JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_00024Sender_setDefaultEnqueueTimeout (JNIEnv *pEnv, jobject joThis, jlong jlEnqueueTimeout)
{
    if (jlEnqueueTimeout < 0) {
        pEnv->ThrowNew(pEnv->FindClass ("java/lang/IllegalArgumentException"),"illegal argument: EnqueueTimeout cannot be negative");
        return;
    }
    jclass jcMessageSender = pEnv->GetObjectClass (joThis);
    jfieldID jfMessageSender = pEnv->GetFieldID (jcMessageSender, "_messageSender", "J");
    jlong jlMessageSender = pEnv->GetLongField (joThis, jfMessageSender);
    MessageSender *pSender = (MessageSender *) UtilWrapper::toPtr (jlMessageSender);
    if (pSender == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MessageSenderWrapper not initialized - sender is null");
        return;
    }
    pSender->setDefaultEnqueueTimeout ((uint32) jlEnqueueTimeout);
}

JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_00024Sender_setDefaultRetryTimeout (JNIEnv *pEnv, jobject joThis, jlong jlRetryTimeout)
{
    if (jlRetryTimeout < 0) {
        pEnv->ThrowNew(pEnv->FindClass ("java/lang/IllegalArgumentException"),"illegal argument: RetryTimeout cannot be negative");
        return;
    }
    jclass jcMessageSender = pEnv->GetObjectClass (joThis);
    jfieldID jfMessageSender = pEnv->GetFieldID (jcMessageSender, "_messageSender", "J");
    jlong jlMessageSender = pEnv->GetLongField (joThis, jfMessageSender);
    MessageSender *pSender = (MessageSender *) UtilWrapper::toPtr (jlMessageSender);
    if (pSender == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MessageSenderWrapper not initialized - sender is null");
        return;
    }
    pSender->setDefaultRetryTimeout ((uint32) jlRetryTimeout);
}
