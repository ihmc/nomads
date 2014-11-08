/*
 * StreamServerMocketWrapper.cpp
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#include "StreamServerMocketWrapper.h"

#include "Mocket.h"
#include "UtilWrapper.h"

JNIEXPORT void JNICALL Java_us_ihmc_mockets_StreamServerMocket_init (JNIEnv *pEnv, jobject joThis)
{
    jclass jcServerMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfServerMocket = pEnv->GetFieldID (jcServerMocket, "_serverMocket", "J");
    StreamServerMocket *pServerMocket = new StreamServerMocket();
    if (pServerMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "StreamServerMocketWrapper not initialized - StreamServerMocket is null");
        return;
    }
    pEnv->SetLongField (joThis, jfServerMocket, UtilWrapper::toJLong (pServerMocket));
}

JNIEXPORT void JNICALL Java_us_ihmc_mockets_StreamServerMocket_close (JNIEnv *pEnv, jobject joThis)
{
    jclass jcServerMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfServerMocket = pEnv->GetFieldID (jcServerMocket, "_serverMocket", "J");
    jlong jlServerMocketRef = pEnv->GetLongField (joThis, jfServerMocket);
    StreamServerMocket *pServerMocket = (StreamServerMocket*) UtilWrapper::toPtr (jlServerMocketRef);

    int rc = pServerMocket->close();
    if (rc != 0) {
        pEnv->ThrowNew (pEnv->FindClass("java/lang/Exception"), "StreamServerMocket::close() error.");
    }
}

JNIEXPORT void JNICALL Java_us_ihmc_mockets_StreamServerMocket_listen (JNIEnv *pEnv, jobject joThis)
{
    jclass jcServerMocket = pEnv->GetObjectClass(joThis);
    jfieldID jfPort = pEnv->GetFieldID (jcServerMocket, "_listenPort", "I");
    jint jiPort = pEnv->GetIntField (joThis, jfPort);
    if (jiPort < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/IllegalArgumentException"), "illegal argument: Port cannot be negative");
        return;
    }

    jfieldID jfAddr = pEnv->GetFieldID (jcServerMocket, "_listenAddress", "Ljava/lang/String;");
    jstring jsAddr = (jstring) pEnv->GetObjectField (joThis, jfAddr);

    jfieldID jfServerMocket = pEnv->GetFieldID (jcServerMocket, "_serverMocket", "J");
    jlong jlServerMocket = pEnv->GetLongField (joThis, jfServerMocket);

    StreamServerMocket *pServerMocket = (StreamServerMocket *) UtilWrapper::toPtr (jlServerMocket);
    if (pServerMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "StreamServerMocket not initialized - ServerMocket is null");
        return;
    }
    int rc;
    if (jsAddr != NULL) {
        const char* pszListenAddr = pEnv->GetStringUTFChars (jsAddr, NULL);
        rc = pServerMocket->listen ((uint16) jiPort, pszListenAddr);
        pEnv->ReleaseStringUTFChars (jsAddr, pszListenAddr);
    }
    else {
        rc = pServerMocket->listen ((uint16) jiPort);
    }

    if (rc < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), "listen failed");
        return;
    }
}

JNIEXPORT jobject JNICALL Java_us_ihmc_mockets_StreamServerMocket_accept (JNIEnv *pEnv, jobject joThis)
{
    jclass jcServerMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfServerMocket = pEnv->GetFieldID (jcServerMocket, "_serverMocket", "J");
    jlong jlServerMocketRef = pEnv->GetLongField (joThis, jfServerMocket);
    StreamServerMocket *pServerMocket = (StreamServerMocket*) UtilWrapper::toPtr (jlServerMocketRef);

    if (pServerMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "StreamServerMocket not initialized - ServerMocket is null");
        return NULL;
    }

    StreamMocket *pMocket = pServerMocket->accept();
    if (pMocket == NULL) {

        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "StreamServerMocket not initialized - Mocket is null");
        return NULL;
    }

    jclass jcMocket = pEnv->FindClass ("us/ihmc/mockets/StreamMocket");
    jmethodID jmConstructor = pEnv->GetMethodID (jcMocket, "<init>", "()V");
    jobject joMocket = pEnv->NewObject (jcMocket, jmConstructor);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    pEnv->SetLongField (joMocket, jfMocket, UtilWrapper::toJLong (pMocket));

    return joMocket;
}

JNIEXPORT void JNICALL Java_us_ihmc_mockets_StreamServerMocket_dispose (JNIEnv *pEnv, jobject joThis)
{
    jclass jcServerMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfServerMocket = pEnv->GetFieldID (jcServerMocket, "_serverMocket", "J");
    jlong jlServerMocketRef = pEnv->GetLongField (joThis, jfServerMocket);
    StreamServerMocket *pServerMocket = (StreamServerMocket*) UtilWrapper::toPtr (jlServerMocketRef);
    if (pServerMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "ServerMocket not initialized - ServerMocket is null");
        return;
    }

    delete pServerMocket;
}

