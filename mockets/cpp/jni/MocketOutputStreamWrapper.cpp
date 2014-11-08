/*
 * MocketOutputStreamWrapper.cpp
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#include "MocketOutputStreamWrapper.h"

#include "StreamMocket.h"
#include "UtilWrapper.h"


JNIEXPORT void JNICALL Java_us_ihmc_mockets_MocketOutputStream_init (JNIEnv *pEnv, jobject joThis, jobject joMocket)
{
    jclass jcMocket = pEnv->GetObjectClass (joMocket);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joMocket, jfMocket);
    StreamMocket *pMocket = (StreamMocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException;"), "MocketOutputStream not initialized - Mocket is null");
        return;
    }

    jclass jcMOS = pEnv->GetObjectClass (joThis);
    jfMocket = pEnv->GetFieldID (jcMOS, "_mocket", "J");
    pEnv->SetLongField (joThis, jfMocket, jlMocket);
}

JNIEXPORT void JNICALL Java_us_ihmc_mockets_MocketOutputStream_write (JNIEnv *pEnv, jobject joThis, jint jiByte)
{
    jclass jcMOS = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMOS, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    StreamMocket *pMocket = (StreamMocket *) UtilWrapper::toPtr (jlMocket);
	if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException;"), "MocketOutputStream not initialized - Mocket is null");
        return;
	}
	char buf[1];
	buf[0] = (char)((int32)jiByte);
	
	int rc = pMocket->send (buf, 1);
	if (rc < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException;"), "error while calling write.");
	}
}

JNIEXPORT void JNICALL Java_us_ihmc_mockets_MocketOutputStream_nativeWrite (JNIEnv *pEnv, jobject joThis, jbyteArray jbaBuf, jint jiOff, jint jiLen)
{
    jclass jcMOS = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMOS, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    StreamMocket *pMocket = (StreamMocket *) UtilWrapper::toPtr (jlMocket);
	if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException;"), "MocketOutputStream not initialized - Mocket is null");
        return;
	}

    jbyte *pjbBuffer = pEnv->GetByteArrayElements (jbaBuf, NULL);

    if (pjbBuffer == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException;"), "buffer is null");
        pEnv->ReleaseByteArrayElements (jbaBuf, pjbBuffer, JNI_ABORT);
        return;
    }

    pjbBuffer = pjbBuffer + jiOff;

    int rc = pMocket->send (pjbBuffer, jiLen);

    if (rc < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException;"), "error in send.");
    }

    pEnv->ReleaseByteArrayElements (jbaBuf, pjbBuffer, JNI_ABORT);
    return;
}

JNIEXPORT void JNICALL Java_us_ihmc_mockets_MocketOutputStream_flush (JNIEnv *pEnv, jobject joThis)
{
    jclass jcMOS = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMOS, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    StreamMocket *pMocket = (StreamMocket *) UtilWrapper::toPtr (jlMocket);
	if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException;"), "MocketOutputStream not initialized - Mocket is null");
        return;
	}

	int rc = pMocket->flush();
	if (rc < 0) {
		pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException;"), "error calling flush()");
	}
}

JNIEXPORT void JNICALL Java_us_ihmc_mockets_MocketOutputStream_close (JNIEnv *pEnv, jobject joThis)
{
    jclass jcMOS = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMOS, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    StreamMocket *pMocket = (StreamMocket *) UtilWrapper::toPtr (jlMocket);
	if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException;"), "MocketOutputStream not initialized - Mocket is null");
        return;
	}

	int rc = pMocket->close();
	if (rc < 0) {
		pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException;"), "error calling close()");
	}
}
