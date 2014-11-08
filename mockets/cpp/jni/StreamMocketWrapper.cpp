/*
 * StreamMocketWrapper.cpp
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#include "StreamMocketWrapper.h"
#include "StreamMocket.h"
#include "UtilWrapper.h"

#include "InetAddr.h"

using namespace NOMADSUtil;

extern bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec);
extern JNIEnv * JNU_GetEnv();
extern JavaVM *cached_jvm;
extern jclass jcMocketStatusListenerGlobalRef;
extern jmethodID jmPeerUnreachableWarningGlobal;

/*
JNIEXPORT jint JNICALL JNI_OnLoad (JavaVM *pJvm, void * pReserved)
{
    JNIEnv *pEnv;
    jclass jcCls;
    cached_jvm = pJvm; // cache the JavaVM pointer
    if (pJvm->GetEnv ((void **) &pEnv, JNI_VERSION_1_2)) {
        return JNI_ERR; // JNI version not supported
    }
    // Get access to the MocketStatusListener interface
    jcCls = pEnv->FindClass ("us/ihmc/mockets/MocketStatusListener");
    if (jcCls == NULL) {
        return JNI_ERR;
    }
    // Use weak global ref to allow the class to be unloaded
    jcMocketStatusListenerGlobalRef = (jclass) pEnv->NewWeakGlobalRef (jcCls);
    if (jcMocketStatusListenerGlobalRef == NULL) {
        return JNI_ERR;
    }
    // Get access to the peerUnreachableWarning method in the MocketStatusListener interface
    jmPeerUnreachableWarningGlobal = pEnv->GetMethodID (jcMocketStatusListenerGlobalRef, "peerUnreachableWarning", "(J)Z");
    if (jmPeerUnreachableWarningGlobal == NULL) {
        return JNI_ERR;
    }

    // Initialize the Logger
  
    if (pLogger == NULL) {
        ATime startTime;
        char szLogFileName[80];
        sprintf (szLogFileName, "mocket.jni.%04d%02d%02d.%02d%02d%02d.log",
                 startTime.year(), startTime.month(), startTime.dayOfMonth(),
                 startTime.h24(), startTime.min(), startTime.sec());
        pLogger = new Logger();
        pLogger->initLogFile (szLogFileName, false);
        pLogger->enableFileOutput();
        pLogger->disableScreenOutput();
        pLogger->setDebugLevel (Logger::L_LowDetailDebug);
    }

    return JNI_VERSION_1_2;
}
*/

JNIEXPORT void JNICALL Java_us_ihmc_mockets_StreamMocket_init (JNIEnv *pEnv, jobject joThis)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    StreamMocket *pMocket = new StreamMocket();
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return;
    }
    pEnv->SetLongField (joThis, jfMocket, UtilWrapper::toJLong (pMocket));
}

JNIEXPORT void JNICALL Java_us_ihmc_mockets_StreamMocket_setDataBufferingTime (JNIEnv *pEnv, jobject joThis, jint jiMillis)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    StreamMocket *pMocket = (StreamMocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "mocket is null");
        return;
    }

    pMocket->setDataBufferingTime ((uint32) jiMillis);
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_StreamMocket_getDataBufferingTime (JNIEnv *pEnv, jobject joThis)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    StreamMocket *pMocket = (StreamMocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "mocket is null");
        return -1;
    }

    jint jiTime = (jint) pMocket->getDataBufferingTime();
    return jiTime;
}


JNIEXPORT void JNICALL Java_us_ihmc_mockets_StreamMocket_close (JNIEnv *pEnv, jobject joThis)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    StreamMocket *pMocket = (StreamMocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "mocket is null");
        return;
    }

    pMocket->close();
}

JNIEXPORT void JNICALL Java_us_ihmc_mockets_StreamMocket_connect (JNIEnv *pEnv, jobject joThis, jstring jsHost, jint jiPort, jint jiTimeout)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    StreamMocket *pMocket = (StreamMocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "mocket is null");
        return;
    }

    const char *pszRemoteHost = pEnv->GetStringUTFChars (jsHost, NULL);
    int rc;

    if (jiTimeout == 0) {
        rc = pMocket->connect (pszRemoteHost, (uint16) jiPort);
    }
    else {
        rc = pMocket->connect (pszRemoteHost, (uint16) jiPort, (int64) jiTimeout);
    }

    pEnv->ReleaseStringUTFChars (jsHost, pszRemoteHost);

    if (rc < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), "could not connect to remote host.");
        return;
    }
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_StreamMocket_getLocalPort (JNIEnv *pEnv, jobject joThis)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    StreamMocket *pMocket = (StreamMocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "mocket is null");
        return -1;
    }

    return (jint) pMocket->getLocalPort();
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_StreamMocket_getRemotePort (JNIEnv *pEnv, jobject joThis)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    StreamMocket *pMocket = (StreamMocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "mocket is null");
        return -1;
    }

    return (jint) pMocket->getRemotePort();
}

JNIEXPORT void JNICALL Java_us_ihmc_mockets_StreamMocket_bind (JNIEnv *pEnv, jobject joThis, jstring jsAddress, jint jiPort)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    StreamMocket *pMocket = (StreamMocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "mocket is null");
        return;
    }

    const char *pszBindAddr = pEnv->GetStringUTFChars (jsAddress, NULL);
    int rc = pMocket->bind (pszBindAddr, (uint16) jiPort);
    pEnv->ReleaseStringUTFChars (jsAddress, pszBindAddr);

    if (rc < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), "could not connect to remote host.");
        return;
    }
}

JNIEXPORT jstring JNICALL Java_us_ihmc_mockets_StreamMocket_getLocalAddress (JNIEnv *pEnv, jobject joThis)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    StreamMocket *pMocket = (StreamMocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "mocket is null");
        return NULL;
    }

    InetAddr ina (pMocket->getLocalAddress());
    const char * pszAddr = ina.getIPAsString();
    jstring jsAddress = pEnv->NewStringUTF (pszAddr);

    return jsAddress;
}

JNIEXPORT jstring JNICALL Java_us_ihmc_mockets_StreamMocket_getRemoteAddress (JNIEnv *pEnv, jobject joThis)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    StreamMocket *pMocket = (StreamMocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "mocket is null");
        return NULL;
    }

    InetAddr ina (pMocket->getRemoteAddress());
    const char * pszAddr = ina.getIPAsString();
    jstring jsAddress = pEnv->NewStringUTF (pszAddr);

    return jsAddress;
}

JNIEXPORT void JNICALL Java_us_ihmc_mockets_StreamMocket_registerStatusListener (JNIEnv *pEnv, jobject joThis, jobject joListener)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    StreamMocket *pMocket = (StreamMocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return;
    }
    #if !defined (ANDROID) //No support for NewWeakGlobalRef on Dalvik JVM (ANDROID)
        // Use weak global ref to allow the class to be unloaded
        jobject joListenerGlobalRef = pEnv->NewWeakGlobalRef (joListener);
    #else
        jobject joListenerGlobalRef = pEnv->NewGlobalRef (joListener);
    #endif
    if (joListenerGlobalRef == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "could not create a weak global reference to the listener");
        return;
    }
    void *pCallbackArg = (void*) joListenerGlobalRef;
    pMocket->registerPeerUnreachableWarningCallback (unreachablePeerCallback, (void*) joListenerGlobalRef);
}


JNIEXPORT void JNICALL Java_us_ihmc_mockets_StreamMocket_dispose (JNIEnv *pEnv, jobject joThis)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    StreamMocket *pMocket = (StreamMocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "Mocket not initialized");
        return;
    }

    delete pMocket;
}

/*
bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec)
{
    JNIEnv *pEnv = JNU_GetEnv();
    jint jiRes;
    #ifdef JNI_VERSION_1_2
        jiRes = cached_jvm->AttachCurrentThread ((void**) &pEnv, NULL);
    #else
        jiRes = cached_jvm->AttachCurrentThread (&pEnv, NULL);
    #endif

    if (jiRes < 0) {
        printf ("MocketWrapper-ureachablePeerCallback: attach jvm failed\n");
        cached_jvm->DetachCurrentThread();
        return false;
    }
    jobject joListenerGlobalRef = (jobject) pCallbackArg;
    jobject joListener = pEnv->NewLocalRef (joListenerGlobalRef);
    jclass jcMocketStatusListener = (jclass) pEnv->NewLocalRef (jcMocketStatusListenerGlobalRef);
    if (jcMocketStatusListener == NULL) {
        printf ("MocketWrapper-ureachablePeerCallback: cannot invoke peer unreachable callback because the reference to the MocketStatusListener class is NULL\n");
        cached_jvm->DetachCurrentThread();
        return false;
    }
    if (joListener == NULL) {
        printf ("MocketWrapper-ureachablePeerCallback: cannot invoke peer unreachable callback because the reference to the listener is NULL\n");
        cached_jvm->DetachCurrentThread();
        return false;
    }
    jboolean jbResult = pEnv->CallBooleanMethod (joListener, jmPeerUnreachableWarningGlobal, (jlong)ulMilliSec);
    cached_jvm->DetachCurrentThread();
    return (jbResult ? true : false);
}
*/
