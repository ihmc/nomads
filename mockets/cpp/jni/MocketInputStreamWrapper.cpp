/*
 * MocketInputStreamWrapper.cpp
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#include "MocketInputStreamWrapper.h"

#include "StreamMocket.h"
#include "UtilWrapper.h"

JNIEXPORT void JNICALL Java_us_ihmc_mockets_MocketInputStream_init (JNIEnv *pEnv, jobject joThis, jobject joMocket)
{
    jclass jcMocket = pEnv->GetObjectClass (joMocket);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joMocket, jfMocket);
    StreamMocket *pMocket = (StreamMocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException;"), "MessageSenderMocket not initialized - Mocket is null");
        return;
    }

    jclass jcMIS = pEnv->GetObjectClass (joThis);
    jfMocket = pEnv->GetFieldID (jcMIS, "_mocket", "J");
    pEnv->SetLongField (joThis, jfMocket, jlMocket);
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_MocketInputStream_read (JNIEnv *pEnv, jobject joThis)
{
    jclass jcMIS = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMIS, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    StreamMocket *pMocket = (StreamMocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException;"), "MessageSenderMocket not initialized - ServerMocket is null");
        return -1;
    }
    char buf[1];
    int rc = pMocket->receive (buf, 1, 0);
	if (rc == 0) {
        return -1;
    }
    else if (rc < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException;"), "Error in receive");
        return -1;
    }

    return (jint) buf[0];
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_MocketInputStream_nativeRead (JNIEnv *pEnv, jobject joThis, 
                                                                          jbyteArray jbABuffer, jint jiOffset, jint jiLength)
{
    jclass jcMIS = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMIS, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    StreamMocket *pMocket = (StreamMocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException;"), "MocketsJavaWrapper not initialized - mocket is null");
        return -1;
    }

    jbyte *pjbBuffer = pEnv->GetByteArrayElements (jbABuffer, NULL);
    if (pjbBuffer == NULL) {
        pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException;"), "fail while copying string RemoteHost");
        return -1;
    }

    pjbBuffer = pjbBuffer + jiOffset;
    int rc = pMocket->receive (pjbBuffer, jiLength, 0);
    if (rc < -1) {
        pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException;"), "receive failed");
        pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
        return rc;
    }
    else {
        pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, 0);
        return rc;
    }
}
