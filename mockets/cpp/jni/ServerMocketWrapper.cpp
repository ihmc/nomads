/*
 * ServerMocketWrapper.cpp
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#include "ServerMocketWrapper.h"

#include "ServerMocket.h"

#include "UtilWrapper.h"

JNIEXPORT void JNICALL Java_us_ihmc_mockets_ServerMocket_init (JNIEnv *pEnv, jobject joThis, jstring jsConfigFile)
{
    jclass jcServerMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfServerMocket = pEnv->GetFieldID (jcServerMocket, "_serverMocket", "J");
    const char *pszConfigFile = NULL;
    if (jsConfigFile != NULL) {
        pszConfigFile = pEnv->GetStringUTFChars (jsConfigFile, NULL);
    }
    ServerMocket *pServerMocket = new ServerMocket (pszConfigFile);
    if (pszConfigFile != NULL) {
        pEnv->ReleaseStringUTFChars (jsConfigFile, pszConfigFile);
    }
    if (pServerMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/Exception"), "MocketsJavaWrapper not initialized - ServerMocket is null");
        return;      
    }
    pEnv->SetLongField (joThis, jfServerMocket, UtilWrapper::toJLong (pServerMocket));
}

JNIEXPORT void JNICALL Java_us_ihmc_mockets_ServerMocket_dispose (JNIEnv *pEnv, jobject joThis)
{
    jclass jcServerMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfServerMocket = pEnv->GetFieldID (jcServerMocket, "_serverMocket", "J");
    jlong jlServerMocket = pEnv->GetLongField (joThis, jfServerMocket);
    ServerMocket *pServerMocket = (ServerMocket *) UtilWrapper::toPtr (jlServerMocket);
    if (pServerMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - ServerMocket is null");
        return;      
    }
    delete pServerMocket;
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_ServerMocket_listen (JNIEnv *pEnv, jobject joThis, jint jiPort)
{
    if (jiPort < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/IllegalArgumentException"), "illegal argument: Port cannot be negative");
        return -1;
    }
    jclass jcServerMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfServerMocket = pEnv->GetFieldID (jcServerMocket, "_serverMocket", "J");
    jlong jlServerMocket = pEnv->GetLongField (joThis, jfServerMocket);
    ServerMocket *pServerMocket = (ServerMocket *) UtilWrapper::toPtr (jlServerMocket);
    if (pServerMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - ServerMocket is null");
        return -1;      
    }
    int rc = pServerMocket->listen ((uint16) jiPort);
    if (rc < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), "listen failed");
        return rc;
    }
    else {
        return rc;
    }
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_ServerMocket_listenAddr (JNIEnv *pEnv, jobject joThis, jint jiPort,
                                                                     jstring jsListenAddr)
{
    if (jiPort <0) {
        pEnv->ThrowNew(pEnv->FindClass ("java/lang/IllegalArgumentException"), "illegal argument: Port cannot be negative");
        return -1;
    }
    jclass jcServerMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfServerMocket = pEnv->GetFieldID (jcServerMocket, "_serverMocket", "J");
    jlong jlServerMocket = pEnv->GetLongField (joThis, jfServerMocket);
    ServerMocket *pServerMocket = (ServerMocket *) UtilWrapper::toPtr (jlServerMocket);
    if (pServerMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - ServerMocket is null");
        return -1;      
    }
    const char *pcListenAddr = pEnv->GetStringUTFChars (jsListenAddr, NULL);
    if (pcListenAddr == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - listenAddr is null");
        return -1;
    }
    int rc = pServerMocket->listen ((uint16) jiPort, pcListenAddr);
    if (rc < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), "listen failed");
        pEnv->ReleaseStringUTFChars (jsListenAddr, pcListenAddr);
        return rc;
    }
    else {
        pEnv->ReleaseStringUTFChars (jsListenAddr, pcListenAddr);
        return rc;
    }
}

JNIEXPORT jobject JNICALL Java_us_ihmc_mockets_ServerMocket_acceptNoPort (JNIEnv *pEnv, jobject joThis)
{
    jclass jcServerMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfServerMocket = pEnv->GetFieldID (jcServerMocket, "_serverMocket", "J");
    jlong jlServerMocket = pEnv->GetLongField (joThis, jfServerMocket);
    ServerMocket *pServerMocket = (ServerMocket *) UtilWrapper::toPtr (jlServerMocket);
    if (pServerMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - ServerMocket is null");
        return NULL;      
    }
    Mocket *pMocket = pServerMocket->accept();
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - Mocket is null");
        return NULL;      
    }
    jclass jcMocket = pEnv->FindClass ("us/ihmc/mockets/Mocket");
    jmethodID jmConstructor = pEnv->GetMethodID (jcMocket, "<init>", "()V");
    jobject joMocket = pEnv->NewObject (jcMocket, jmConstructor);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    pEnv->SetLongField (joMocket, jfMocket, UtilWrapper::toJLong (pMocket));
    return joMocket;
}

JNIEXPORT jobject JNICALL Java_us_ihmc_mockets_ServerMocket_acceptWPort (JNIEnv *pEnv, jobject joThis, jint jiPort)
{
    jclass jcServerMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfServerMocket = pEnv->GetFieldID (jcServerMocket, "_serverMocket", "J");
    jlong jlServerMocket = pEnv->GetLongField (joThis, jfServerMocket);
    ServerMocket *pServerMocket = (ServerMocket *) UtilWrapper::toPtr (jlServerMocket);
    if (pServerMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - ServerMocket is null");
        return NULL;
    }
    Mocket *pMocket = pServerMocket->accept ((uint16) jiPort);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - Mocket is null");
        return NULL;
    }
    jclass jcMocket = pEnv->FindClass ("us/ihmc/mockets/Mocket");
    jmethodID jmConstructor = pEnv->GetMethodID (jcMocket, "<init>", "()V");
    jobject joMocket = pEnv->NewObject (jcMocket, jmConstructor);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    pEnv->SetLongField (joMocket, jfMocket, UtilWrapper::toJLong (pMocket));
    return joMocket;
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_ServerMocket_close (JNIEnv *pEnv, jobject joThis)
{
    jclass jcServerMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfServerMocket = pEnv->GetFieldID (jcServerMocket, "_serverMocket", "J");
    jlong jlServerMocket = pEnv->GetLongField (joThis, jfServerMocket);
    ServerMocket *pServerMocket = (ServerMocket *) UtilWrapper::toPtr (jlServerMocket);
    return pServerMocket->close();
}

JNIEXPORT void JNICALL Java_us_ihmc_mockets_ServerMocket_setIdentifier (JNIEnv *pEnv, jobject joThis, jstring jsIdentifier)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfServerMocket = pEnv->GetFieldID (jcMocket, "_serverMocket", "J");
    jlong jlServerMocket = pEnv->GetLongField (joThis, jfServerMocket);
    ServerMocket *pServerMocket = (ServerMocket *) UtilWrapper::toPtr (jlServerMocket);
    if (pServerMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return;      
    }

    if (jsIdentifier == NULL) {
        pServerMocket->setIdentifier (NULL);
    }
    else {
        const char *pszIdentifier = pEnv->GetStringUTFChars (jsIdentifier, NULL);
        pServerMocket->setIdentifier (pszIdentifier);
        pEnv->ReleaseStringUTFChars (jsIdentifier, pszIdentifier);
    }
}

JNIEXPORT jstring JNICALL Java_us_ihmc_mockets_ServerMocket_getIdentifier (JNIEnv *pEnv, jobject joThis)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfServerMocket = pEnv->GetFieldID (jcMocket, "_serverMocket", "J");
    jlong jlServerMocket = pEnv->GetLongField (joThis, jfServerMocket);
    ServerMocket *pServerMocket = (ServerMocket *) UtilWrapper::toPtr (jlServerMocket);
    if (pServerMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return NULL;      
    }

    const char *pszIdentifier = pServerMocket->getIdentifier();
    if (pszIdentifier == NULL) {
        return NULL;
    }
    else {
        jstring jsIdentifier = pEnv->NewStringUTF (pszIdentifier);
        return jsIdentifier;
    }
}

