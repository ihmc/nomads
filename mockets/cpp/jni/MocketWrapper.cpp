/*
 * MocketWrapper.cpp
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#include "MocketWrapper.h"

#include "MessageSender.h"
#include "Mocket.h"

#include "Logger.h"
#include "InetAddr.h"
#include "TClass.h"

#include "UtilWrapper.h"
//#include "WriterWrapper.h"

using namespace NOMADSUtil;

bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec);
bool reachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec);
bool suspendReceivedCallback (void *pCallbackArg, unsigned long ulMilliSec);

JavaVM *cached_jvm;
jclass jcMocketStatusListenerGlobalRef;
jmethodID jmPeerUnreachableWarningGlobal;
jmethodID jmPeerReachableGlobal;
jmethodID jmSuspendReceivedGlobal;

JNIEnv * JNU_GetEnv()
{
    JNIEnv *pEnv;
    cached_jvm->GetEnv ((void **)&pEnv, JNI_VERSION_1_2);
    return pEnv;
}

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
    #if !defined (ANDROID) //No support for NewWeakGlobalRef on Dalvik JVM (ANDROID)
        // Use weak global ref to allow the class to be unloaded
        jcMocketStatusListenerGlobalRef = (jclass) pEnv->NewWeakGlobalRef (jcCls);
    #else
        jcMocketStatusListenerGlobalRef = (jclass) pEnv->NewGlobalRef (jcCls);
    #endif
    if (jcMocketStatusListenerGlobalRef == NULL) {
        return JNI_ERR;
    }
    // Get access to the peerUnreachableWarning method in the MocketStatusListener interface
    jmPeerUnreachableWarningGlobal = pEnv->GetMethodID (jcMocketStatusListenerGlobalRef, "peerUnreachableWarning", "(J)Z");
    if (jmPeerUnreachableWarningGlobal == NULL) {
        return JNI_ERR;
    }
    // Get access to the peerReachable method in the MocketStatusListener interface
    jmPeerReachableGlobal = pEnv->GetMethodID (jcMocketStatusListenerGlobalRef, "peerReachable", "(J)Z");
    if (jmPeerReachableGlobal == NULL) {
        return JNI_ERR;
    }

    // Get access to the suspendReceived method in the MocketStatusListener interface
    jmSuspendReceivedGlobal = pEnv->GetMethodID (jcMocketStatusListenerGlobalRef, "suspendReceived", "(J)Z");
    if (jmSuspendReceivedGlobal == NULL) {
        return JNI_ERR;
    }

/*    // Initialize the Logger
    if (pLogger == NULL) {
        ATime startTime;
        char szLogFileName[80];
        sprintf (szLogFileName, "mocket.jni.%04d%02d%02d.%02d%02d%02d.log",
                 startTime.year(), startTime.month(), startTime.dayOfMonth(),
                 startTime.h24(), startTime.min(), startTime.sec());
        pLogger = new Logger();
        pLogger->initLogFile (szLogFileName, false);
        pLogger->disableFileOutput();
        pLogger->disableScreenOutput();
        pLogger->setDebugLevel (Logger::L_LowDetailDebug);
    }
*/
    return JNI_VERSION_1_2;
}

JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_init (JNIEnv *pEnv, jobject joThis, jstring jsConfigFile)
{
    // bAbortConnection = false;
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    const char *pszConfigFile = NULL;
    if (jsConfigFile != NULL) {
        pszConfigFile = pEnv->GetStringUTFChars (jsConfigFile, NULL);
    }
    Mocket *pMocket = new Mocket (pszConfigFile);
    if (pszConfigFile != NULL) {
        pEnv->ReleaseStringUTFChars (jsConfigFile, pszConfigFile);
    }
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return;
    }
    pEnv->SetLongField (joThis, jfMocket, UtilWrapper::toJLong (pMocket));
}

JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_dispose (JNIEnv *pEnv, jobject joThis)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return;
    }

    delete pMocket;
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_connectNative (JNIEnv *pEnv, jobject joThis,
                                                                         jstring jsRemoteHost, jint jiRemotePort)
{
    if (jiRemotePort < 0) {
        pEnv->ThrowNew(pEnv->FindClass ("java/lang/IllegalArgumentException"), "illegal argument: RemotePort cannot be negative");
        return -20;
    }
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return -22;
    }
    const char *pcRemoteHost;
    pcRemoteHost = pEnv->GetStringUTFChars(jsRemoteHost, NULL);
    if(pcRemoteHost == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "fail while copying string RemoteHost");
        pEnv->ReleaseStringUTFChars (jsRemoteHost, pcRemoteHost);
        return -21;
    }
    int rc = pMocket->connect (pcRemoteHost, (uint16) jiRemotePort);
    if (rc < 0) {
    	char buf[100];
    	sprintf (buf, "failed to connect to %s:%d\n", pcRemoteHost, jiRemotePort);
        pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), buf);
        pEnv->ReleaseStringUTFChars (jsRemoteHost, pcRemoteHost);
        return rc;
    }
    else if (!rc) {
        pEnv->ReleaseStringUTFChars (jsRemoteHost, pcRemoteHost);
        return rc;
    }
    pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), "returned value from send unknown");
    pEnv->ReleaseStringUTFChars (jsRemoteHost, pcRemoteHost);
    return rc;
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_connectAsyncNative (JNIEnv *pEnv, jobject joThis,
                                                                         jstring jsRemoteHost, jint jiRemotePort)
{
    if (jiRemotePort < 0) {
        pEnv->ThrowNew(pEnv->FindClass ("java/lang/IllegalArgumentException"), "illegal argument: RemotePort cannot be negative");
        return -20;
    }
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return -22;
    }
    const char *pcRemoteHost;
    pcRemoteHost = pEnv->GetStringUTFChars(jsRemoteHost, NULL);
    if(pcRemoteHost == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "fail while copying string RemoteHost");
        pEnv->ReleaseStringUTFChars (jsRemoteHost, pcRemoteHost);
        return -21;
    }

    int rc = pMocket->connectAsync (pcRemoteHost, (uint16) jiRemotePort);
    if (rc < 0) {
    	char buf[100];
    	sprintf (buf, "failed to connect to %s:%d\n", pcRemoteHost, jiRemotePort);
        pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), buf);
        pEnv->ReleaseStringUTFChars (jsRemoteHost, pcRemoteHost);
        return rc;
    }
    else if (!rc) {
        pEnv->ReleaseStringUTFChars (jsRemoteHost, pcRemoteHost);
        return rc;
    }
    pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), "returned value from send unknown");
    pEnv->ReleaseStringUTFChars (jsRemoteHost, pcRemoteHost);
    return rc;
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_finishConnectNative (JNIEnv *pEnv, jobject joThis)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return -1;
    }
    return pMocket->finishConnect();
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_connectTimeout (JNIEnv *pEnv, jobject joThis,
                                                                          jstring jsRemoteHost, jint jiRemotePort,
                                                                          jlong jlTimeout)
{
    if (jiRemotePort < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/IllegalArgumentException"), "illegal argument: RemotePort");
        return -20;
    }
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return -22;
    }

    const char *pcRemoteHost;
    pcRemoteHost = pEnv->GetStringUTFChars (jsRemoteHost,NULL);
    if(pcRemoteHost == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "fail while copying string RemoteHost");
        pEnv->ReleaseStringUTFChars (jsRemoteHost, pcRemoteHost);
        return -23;
    }
    int rc = pMocket->connect (pcRemoteHost, (uint16) jiRemotePort, (int64)jlTimeout);
    if (rc < 0) {
    	char buf[100];
    	sprintf (buf, "failed to connect to %s:%d; rc = %d\n", pcRemoteHost, jiRemotePort, rc);
        pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), buf);
        pEnv->ReleaseStringUTFChars (jsRemoteHost, pcRemoteHost);
        return rc;
    }
    else if (!rc) {
        pEnv->ReleaseStringUTFChars (jsRemoteHost,pcRemoteHost);
        return rc;
    }
    pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), "returned value from send unknown");
    pEnv->ReleaseStringUTFChars (jsRemoteHost,pcRemoteHost);
    return rc;

}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_connectNative (JNIEnv *pEnv, jobject joThis,
                                                                    jstring jsRemoteHost, jint jiRemotePort,
                                                                    jboolean jbPreExchangeKeys)
{
    if (jiRemotePort < 0) {
        pEnv->ThrowNew(pEnv->FindClass ("java/lang/IllegalArgumentException"), "illegal argument: RemotePort cannot be negative");
        return -20;
    }
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return -22;
    }
    const char *pcRemoteHost;
    pcRemoteHost = pEnv->GetStringUTFChars(jsRemoteHost, NULL);
    if(pcRemoteHost == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "fail while copying string RemoteHost");
        pEnv->ReleaseStringUTFChars (jsRemoteHost, pcRemoteHost);
        return -21;
    }
    bool bPreExchangeKeys = jbPreExchangeKeys ? true : false;
    int rc = pMocket->connect (pcRemoteHost, (uint16) jiRemotePort, bPreExchangeKeys);
    if (rc < 0) {
    	char buf[100];
    	sprintf (buf, "failed to connect to %s:%d\n", pcRemoteHost, jiRemotePort);
        pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), buf);
        pEnv->ReleaseStringUTFChars (jsRemoteHost, pcRemoteHost);
        return rc;
    }
    else if (!rc) {
        pEnv->ReleaseStringUTFChars (jsRemoteHost, pcRemoteHost);
        return rc;
    }
    pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), "returned value from send unknown");
    pEnv->ReleaseStringUTFChars (jsRemoteHost, pcRemoteHost);
    return rc;
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_connectExchangeKeysTimeout (JNIEnv *pEnv, jobject joThis,
                                                                          jstring jsRemoteHost, jint jiRemotePort,
                                                                          jboolean jbPreExchangeKeys, jlong jlTimeout)
{
    if (jiRemotePort < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/IllegalArgumentException"), "illegal argument: RemotePort");
        return -20;
    }
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return -22;
    }

    const char *pcRemoteHost;
    pcRemoteHost = pEnv->GetStringUTFChars (jsRemoteHost,NULL);
    if(pcRemoteHost == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "fail while copying string RemoteHost");
        pEnv->ReleaseStringUTFChars (jsRemoteHost, pcRemoteHost);
        return -23;
    }
    bool bPreExchangeKeys = jbPreExchangeKeys ? true : false;
    int rc = pMocket->connect (pcRemoteHost, (uint16) jiRemotePort, bPreExchangeKeys, (int64)jlTimeout);
    if (rc < 0) {
    	char buf[100];
    	sprintf (buf, "failed to connect to %s:%d; rc = %d\n", pcRemoteHost, jiRemotePort, rc);
        pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), buf);
        pEnv->ReleaseStringUTFChars (jsRemoteHost, pcRemoteHost);
        return rc;
    }
    else if (!rc) {
        pEnv->ReleaseStringUTFChars (jsRemoteHost,pcRemoteHost);
        return rc;
    }
    pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), "returned value from send unknown");
    pEnv->ReleaseStringUTFChars (jsRemoteHost,pcRemoteHost);
    return rc;

}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_bindNative (JNIEnv *pEnv, jobject joThis, jstring jsRemoteHost, jint jiRemotePort)
{
    if (jiRemotePort < 0) {
        pEnv->ThrowNew(pEnv->FindClass ("java/lang/IllegalArgumentException"), "illegal argument: RemotePort cannot be negative");
        return -1;
    }
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return -2;
    }
    const char *pcRemoteHost;
    pcRemoteHost = pEnv->GetStringUTFChars(jsRemoteHost, NULL);
    if(pcRemoteHost == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "fail while copying string RemoteHost");
        pEnv->ReleaseStringUTFChars (jsRemoteHost, pcRemoteHost);
        return -3;
    }
    int rc = pMocket->bind (pcRemoteHost, (uint16) jiRemotePort);
    if (rc < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), "connect failed");
        pEnv->ReleaseStringUTFChars (jsRemoteHost, pcRemoteHost);
        return rc;
    }
    else if (!rc) {
        pEnv->ReleaseStringUTFChars (jsRemoteHost, pcRemoteHost);
        return rc;
    }
    pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), "returned value from send unknown");
    pEnv->ReleaseStringUTFChars (jsRemoteHost, pcRemoteHost);
    return rc;
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_reEstablishConn (JNIEnv *pEnv, jobject joThis)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return -10;
    }
    int iReEstablishReturn = pMocket->reEstablishConn();
    if (iReEstablishReturn < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), "reEstablishCon returned with error");
        return iReEstablishReturn;
    }
    return iReEstablishReturn;
}

JNIEXPORT jlong JNICALL Java_us_ihmc_mockets_Mocket_getRemoteAddress (JNIEnv *pEnv, jobject joThis)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return -1;
    }
    return pMocket->getRemoteAddress();
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_getRemotePort (JNIEnv *pEnv, jobject joThis)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return -1;
    }
    return pMocket->getRemotePort();
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_close (JNIEnv *pEnv, jobject joThis)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return -10;
    }
    int iCloseReturn = pMocket->close();
    if (iCloseReturn < 0) {
		char szErrMsg[1024];
		sprintf (szErrMsg, "close returned with error: %d", iCloseReturn);
		pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), szErrMsg);
        return iCloseReturn;
    }
    return iCloseReturn;
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_suspend (JNIEnv *pEnv, jobject joThis)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return -10;
    }
    int iSuspendReturn = pMocket->suspend();
    if (iSuspendReturn < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), "suspend returned with error");
        return iSuspendReturn;
    }
    return iSuspendReturn;
}
/*
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_getStateNative (JNIEnv *pEnv, jobject joThis, jobject joOutputStream)
{
    // Get an instance of Mocket
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return -10;
    }
    int igetStateReturn;
    #if !defined (ANDROID) //unsolved problem with android and WriteWrapper
        // Create a C++ Writer object to pass to the getState method in Mockets
        WriterWrapper* wr = new WriterWrapper (pEnv, joOutputStream);
        // Call getState on the Mocket instance
        igetStateReturn = pMocket->getState (wr);
        if (igetStateReturn < 0) {
            pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), "getState returned with error");
            return igetStateReturn;
        }
    #else
        igetStateReturn = 0;
    #endif
    return igetStateReturn;
}
*/
JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_enableCrossSequecing (JNIEnv *pEnv, jobject joThis,
                                                                               jboolean jbEnable)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return -1;
    }
    bool bEnable = jbEnable ? true : false;

    return pMocket->enableCrossSequencing (bEnable);
}

JNIEXPORT jboolean JNICALL Java_us_ihmc_mockets_Mocket_isCrossSequencingEnabled (JNIEnv *pEnv, jobject joThis)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return -1;
    }
    return pMocket->isCrossSequencingEnabled();
}

JNIEXPORT jobject JNICALL Java_us_ihmc_mockets_Mocket_getSenderNative (JNIEnv *pEnv, jobject joThis,
                                                                        jboolean jbReliable, jboolean jbSequenced,
                                                                        jobject joSender)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return NULL;
    }
    bool bReliable = jbReliable ? true : false;
    bool bSequenced = jbSequenced ? true : false;

    MessageSender Sender = pMocket->getSender (bReliable, bSequenced);
    MessageSender *pSender = new MessageSender(Sender);
    if (pSender == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - sender is null");
        return NULL;
    }
    jclass jcSender = pEnv->GetObjectClass (joSender);
    jfieldID jfSender = pEnv->GetFieldID (jcSender, "_messageSender", "J");
    pEnv->SetLongField (joSender, jfSender, UtilWrapper::toJLong (pSender));

    return joSender;

}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_sendNative (JNIEnv *pEnv, jobject joThis,jboolean jbReliable,
                                                                      jboolean jbSequenced, jbyteArray jbABuffer, jint jiOffset,
                                                                      jint jiLength, jint jiTag, jshort jsPriority,
                                                                      jlong jlEnqueueTimeout, jlong jlRetryTimeout)
{
    if (jiTag < 0) {
        pEnv->ThrowNew(pEnv->FindClass ("java/lang/IllegalArgumentException"), "illegal argument: Tag cannot be negative");
        return -10;
    }
    if (jsPriority < 0) {
        pEnv->ThrowNew(pEnv->FindClass ("java/lang/IllegalArgumentException"), "illegal argument: Priority cannot be negative");
        return -10;
    }
    if (jlEnqueueTimeout < 0) {
        pEnv->ThrowNew(pEnv->FindClass ("java/lang/IllegalArgumentException"), "illegal argument: EnqueueTimeout cannot be negative");
        return -10;
    }
    if (jlRetryTimeout < 0) {
        pEnv->ThrowNew(pEnv->FindClass ("java/lang/IllegalArgumentException"), "illegal argument: RetryTimeout cannot be negative");
        return -10;
    }
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return -11;
    }
    jbyte *pjbBuffer = pEnv->GetByteArrayElements (jbABuffer, NULL);
    if (pjbBuffer == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - buffer is null");
        pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
        return -10;
    }
    pjbBuffer = pjbBuffer + jiOffset;
    bool bReliable = jbReliable ? true : false;
    bool bSequenced = jbSequenced ? true : false;

    int rc = pMocket->send (bReliable, bSequenced, pjbBuffer, jiLength, (uint16) jiTag, (uint8) jsPriority,
                            (uint32) jlEnqueueTimeout, (uint32) jlRetryTimeout);
    if (rc < 0) {
        char szErrMsg[1024];
        sprintf (szErrMsg, "send failed for reliable(=%s), sequenced(=%s) message with rc = %d; length = %d, tag = %d, priority = %d, enqueue timeout = %u, retry timeout = %u",
                 bReliable?"true":"false", bSequenced?"true":"false",
                 rc, jiLength, jiTag, (int) jsPriority,
                 (uint32) jlEnqueueTimeout, (uint32) jlRetryTimeout);
        pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), szErrMsg);
        pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
        return rc;
    }
    else if (!rc) {
        pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
        return rc;
    }
    pEnv->ThrowNew(pEnv->FindClass ("java/io/IOException"), "returned value from send unknown");
    pEnv->ReleaseByteArrayElements (jbABuffer,pjbBuffer, JNI_ABORT);
    return rc;
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_gsendNative (JNIEnv *pEnv, jobject joThis,jboolean jbReliable,
                                                                       jboolean jbSequenced, jbyteArray jbABuffer, jint jiOffset,
                                                                       jint jiLength, jint jiTag, jshort jsPriority,
                                                                       jlong jlEnqueueTimeout, jlong jlRetryTimeout,
                                                                       jstring jsValist1, jstring jsValist2)
{
    if (jiTag < 0) {
        pEnv->ThrowNew(pEnv->FindClass ("java/lang/IllegalArgumentException"), "illegal argument: Tag cannot be negative");
    }
    if (jsPriority < 0) {
        pEnv->ThrowNew(pEnv->FindClass ("java/lang/IllegalArgumentException"), "illegal argument: Priority cannot be negative");
    }
    if (jlEnqueueTimeout < 0) {
        pEnv->ThrowNew(pEnv->FindClass ("java/lang/IllegalArgumentException"), "illegal argument: EnqueueTimeout cannot be negative");
    }
    if (jlRetryTimeout < 0) {
        pEnv->ThrowNew(pEnv->FindClass ("java/lang/IllegalArgumentException"), "illegal argument: RetryTimeout cannot be negative");
    }
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return -1;
    }
    jbyte *pjbBuffer = pEnv->GetByteArrayElements (jbABuffer, NULL);
    if (pjbBuffer == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - buffer is null");
        pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
        return -1;
    }
    pjbBuffer = pjbBuffer + jiOffset;
    bool bReliable = jbReliable ? true : false;
    bool bSequenced = jbSequenced ? true: false;

    char *pcValist1 = (char *) pEnv->GetStringUTFChars (jsValist1, NULL);
    if(pcValist1 == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "fail while copying string valist1");
        pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
        pEnv->ReleaseStringUTFChars (jsValist1, pcValist1);
        return -1;
    }
    char *pcValist2 = (char *) pEnv->GetStringUTFChars (jsValist2, NULL);
    if(pcValist2 == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "fail while copying string RemoteHost");
        pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
        pEnv->ReleaseStringUTFChars (jsValist1, pcValist1);
        pEnv->ReleaseStringUTFChars (jsValist2, pcValist2);
        return -1;
    }
    int rc = pMocket->gsend (bReliable, bSequenced, (uint16) jiTag, (uint8) jsPriority, (uint32) jlEnqueueTimeout,
        (uint32) jlRetryTimeout, pjbBuffer, (uint32)jiLength, pcValist1, pcValist2);
    if (rc < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), "gsend failed");
        pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
        pEnv->ReleaseStringUTFChars (jsValist1, pcValist1);
        pEnv->ReleaseStringUTFChars (jsValist2, pcValist2);
        return rc;
    }
    else if (!rc) {
        pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
        pEnv->ReleaseStringUTFChars (jsValist1, pcValist1);
        pEnv->ReleaseStringUTFChars (jsValist2, pcValist2);
        return rc;
    }
    pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), "returned value from send unknown");
    pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
    pEnv->ReleaseStringUTFChars (jsValist1, pcValist1);
    pEnv->ReleaseStringUTFChars (jsValist2, pcValist2);
    return rc;
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_getNextMessageSize (JNIEnv *pEnv, jobject joThis, jlong jlTimeout)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return -1;
    }
    int rc = pMocket->getNextMessageSize ((int64) jlTimeout);
    if (rc<0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), "Timeout is expired");
        return rc;
    }
    else {
        return rc;
    }
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_receiveNative (JNIEnv *pEnv, jobject joThis, jbyteArray jbABuffer,
                                                                         jint jiOffset, jint jiLength, jlong jlTimeout)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return -10;
    }
    jbyte *pjbBuffer = pEnv->GetByteArrayElements (jbABuffer, NULL);
    if (pjbBuffer == NULL) {
        //pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "fail while copying string RemoteHost");
        return -11;
    }
   
    int rc = pMocket->receive (pjbBuffer + jiOffset, jiLength, jlTimeout);
    if (rc < -1) {
        pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), "receive failed");
        pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
        return rc;
    }
    else {
        pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, 0);
        return rc;
    }
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_sreceive2Args (JNIEnv *pEnv, jobject joThis, jbyteArray jbABuffer1,
                                                                      jint jiOffset1, jint jiLength1, jbyteArray jbABuffer2,
                                                                      jint jiOffset2, jint jiLength2, jlong jlTimeout)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return -1;
    }
    jbyte *pjbBuffer1 = pEnv->GetByteArrayElements (jbABuffer1, NULL);
    if (pjbBuffer1 == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - buffer is null");
        pEnv->ReleaseByteArrayElements (jbABuffer1, pjbBuffer1, JNI_ABORT);
        return -1;
    }
    pjbBuffer1 = pjbBuffer1 + jiOffset1;
    jbyte *pjbBuffer2 = pEnv->GetByteArrayElements (jbABuffer2, NULL);
    if (pjbBuffer2 == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - buffer is null");
        pEnv->ReleaseByteArrayElements (jbABuffer1, pjbBuffer1, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer2, pjbBuffer2, JNI_ABORT);
        return -1;
    }
    pjbBuffer2 = pjbBuffer2 + jiOffset2;
    int rc = pMocket->sreceive ((int64) jlTimeout, pjbBuffer1, jiLength1, pjbBuffer2, jiLength2);
    if (rc < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), "sreceive failed");
        pEnv->ReleaseByteArrayElements (jbABuffer1, pjbBuffer1, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer2, pjbBuffer2, JNI_ABORT);
        return rc;
    }
    else if (!rc) {
        pEnv->ReleaseByteArrayElements (jbABuffer1, pjbBuffer1, 0);
        pEnv->ReleaseByteArrayElements (jbABuffer2, pjbBuffer2, 0);
        return rc;
    }
    pEnv->ThrowNew(pEnv->FindClass ("java/io/IOException"), "returned value from sreceive unknown");
    pEnv->ReleaseByteArrayElements (jbABuffer1, pjbBuffer1, JNI_ABORT);
    pEnv->ReleaseByteArrayElements (jbABuffer2, pjbBuffer2, JNI_ABORT);
    return rc;
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_sreceive3Args (JNIEnv *pEnv, jobject joThis, jbyteArray jbABuffer1,
                                                                      jint jiOffset1, jint jiLength1, jbyteArray jbABuffer2,
                                                                      jint jiOffset2, jint jiLength2, jbyteArray jbABuffer3,
                                                                      jint jiOffset3, jint jiLength3, jlong jlTimeout)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return -1;
    }
    jbyte *pjbBuffer1 = pEnv->GetByteArrayElements (jbABuffer1, NULL);
    if (pjbBuffer1 == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - buffer is null");
        pEnv->ReleaseByteArrayElements (jbABuffer1, pjbBuffer1, JNI_ABORT);
        return -1;
    }
    pjbBuffer1 = pjbBuffer1 + jiOffset1;
    jbyte *pjbBuffer2 = pEnv->GetByteArrayElements (jbABuffer2, NULL);
    if (pjbBuffer2 == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - buffer is null");
        pEnv->ReleaseByteArrayElements (jbABuffer1, pjbBuffer1, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer2, pjbBuffer2, JNI_ABORT);
        return -1;
    }
    pjbBuffer2 = pjbBuffer2 + jiOffset2;
    jbyte *pjbBuffer3 = pEnv->GetByteArrayElements (jbABuffer3, NULL);
    if (pjbBuffer3 == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - buffer is null");
        pEnv->ReleaseByteArrayElements (jbABuffer1, pjbBuffer1, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer2, pjbBuffer2, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer3, pjbBuffer3, JNI_ABORT);
        return -1;
    }
    pjbBuffer3 = pjbBuffer3 + jiOffset3;
    int rc = pMocket->sreceive ((int64) jlTimeout, pjbBuffer1, jiLength1, pjbBuffer2, jiLength2, pjbBuffer3, jiLength3);
    if (rc < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), "sreceive failed");
        pEnv->ReleaseByteArrayElements (jbABuffer1, pjbBuffer1, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer2, pjbBuffer2, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer3, pjbBuffer3, JNI_ABORT);
        return rc;
    }
    else if (!rc) {
        pEnv->ReleaseByteArrayElements (jbABuffer1, pjbBuffer1, 0);
        pEnv->ReleaseByteArrayElements (jbABuffer2, pjbBuffer2, 0);
        pEnv->ReleaseByteArrayElements (jbABuffer3, pjbBuffer3, 0);
        return rc;
    }
    pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), "returned value from sreceive unknown");
    pEnv->ReleaseByteArrayElements (jbABuffer1, pjbBuffer1, JNI_ABORT);
    pEnv->ReleaseByteArrayElements (jbABuffer2, pjbBuffer2, JNI_ABORT);
    pEnv->ReleaseByteArrayElements (jbABuffer3, pjbBuffer3, JNI_ABORT);
    return rc;
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_sreceive4Args (JNIEnv *pEnv, jobject joThis, jbyteArray jbABuffer1,
                                                                      jint jiOffset1, jint jiLength1, jbyteArray jbABuffer2,
                                                                      jint jiOffset2, jint jiLength2, jbyteArray jbABuffer3,
                                                                      jint jiOffset3, jint jiLength3, jbyteArray jbABuffer4,
                                                                      jint jiOffset4, jint jiLength4, jlong jlTimeout)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return -1;
    }
    jbyte *pjbBuffer1 = pEnv->GetByteArrayElements (jbABuffer1, NULL);
    if (pjbBuffer1 == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - buffer is null");
        pEnv->ReleaseByteArrayElements (jbABuffer1, pjbBuffer1, JNI_ABORT);
        return -1;
    }
    pjbBuffer1 = pjbBuffer1 + jiOffset1;
    jbyte *pjbBuffer2 = pEnv->GetByteArrayElements (jbABuffer2, NULL);
    if (pjbBuffer2 == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - buffer is null");
        pEnv->ReleaseByteArrayElements (jbABuffer1, pjbBuffer1, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer2, pjbBuffer2, JNI_ABORT);
        return -1;
    }
    pjbBuffer2 = pjbBuffer2 + jiOffset2;
    jbyte *pjbBuffer3 = pEnv->GetByteArrayElements (jbABuffer3, NULL);
    if (pjbBuffer3 == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - buffer is null");
        pEnv->ReleaseByteArrayElements (jbABuffer1, pjbBuffer1, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer2, pjbBuffer2, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer3, pjbBuffer3, JNI_ABORT);
        return -1;
    }
    pjbBuffer3 = pjbBuffer3 + jiOffset3;
    jbyte *pjbBuffer4 = pEnv->GetByteArrayElements (jbABuffer4, NULL);
    if (pjbBuffer4 == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - buffer is null");
        pEnv->ReleaseByteArrayElements (jbABuffer1, pjbBuffer1, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer2, pjbBuffer2, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer3, pjbBuffer3, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer4, pjbBuffer4, JNI_ABORT);
        return -1;
    }
    pjbBuffer4 = pjbBuffer4 + jiOffset4;
    int rc = pMocket->sreceive ((int64) jlTimeout, pjbBuffer1, jiLength1, pjbBuffer2, jiLength2, pjbBuffer3, jiLength3, pjbBuffer4, jiLength4);
    if (rc < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), "sreceive failed");
        pEnv->ReleaseByteArrayElements (jbABuffer1, pjbBuffer1, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer2, pjbBuffer2, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer3, pjbBuffer3, JNI_ABORT);
        pEnv->ReleaseByteArrayElements (jbABuffer4, pjbBuffer4, JNI_ABORT);
        return rc;
    }
    else if (!rc) {
        pEnv->ReleaseByteArrayElements (jbABuffer1, pjbBuffer1, 0);
        pEnv->ReleaseByteArrayElements (jbABuffer2, pjbBuffer2, 0);
        pEnv->ReleaseByteArrayElements (jbABuffer3, pjbBuffer3, 0);
        pEnv->ReleaseByteArrayElements (jbABuffer4, pjbBuffer4, 0);
        return rc;
    }
    pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), "returned value from sreceive unknown");
    pEnv->ReleaseByteArrayElements (jbABuffer1, pjbBuffer1, JNI_ABORT);
    pEnv->ReleaseByteArrayElements (jbABuffer2, pjbBuffer2, JNI_ABORT);
    pEnv->ReleaseByteArrayElements (jbABuffer3, pjbBuffer3, JNI_ABORT);
    pEnv->ReleaseByteArrayElements (jbABuffer4, pjbBuffer4, JNI_ABORT);
    return rc;
}

JNIEXPORT jbyteArray JNICALL Java_us_ihmc_mockets_Mocket_receiveBuffer (JNIEnv *pEnv, jobject joThis, jlong jlTimeout)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return NULL;
    }
    char *pcBuffer = NULL;
    int rc = pMocket->receive ((void**)&pcBuffer, (int64)jlTimeout);
    if (rc <= 0) {
        return NULL;
    }
    else {
        jbyteArray jbaBuffer = pEnv->NewByteArray (rc);
        pEnv->SetByteArrayRegion (jbaBuffer, 0, rc, (jbyte *)pcBuffer);
        return jbaBuffer;
    }
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_replaceNative (JNIEnv *pEnv, jobject joThis, jboolean jbReliable,
                                                                         jboolean jbSequenced, jbyteArray jbABuffer,
                                                                         jint jiOldTag, jint jiOffset, jint jiLength,
                                                                         jint jiNewTag, jshort jsPriority,
                                                                         jlong jlEnqueueTimeout, jlong jlRetryTimeout)
{
    if (jiOldTag < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/IllegalArgumentException"), "illegal argument: OldTag cannot be negative");
        return -1;
    }
    if (jiNewTag < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/IllegalArgumentException"), "illegal argument: NewTag cannot be negative");
        return -1;
    }
    if (jsPriority < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/IllegalArgumentException"), "illegal argument: Priority cannot be negative");
        return -1;
    }
    if (jlEnqueueTimeout < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/IllegalArgumentException"), "illegal argument: Timeout cannot be negative");
        return -1;
    }
    if (jlRetryTimeout < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/IllegalArgumentException"), "illegal argument: RetryTimeout cannot be negative");
        return -1;
    }
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return -1;
    }
    jbyte *pjbBuffer = pEnv->GetByteArrayElements(jbABuffer,NULL);
    if(pjbBuffer == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "fail while copying string RemoteHost");
        pEnv->ReleaseByteArrayElements (jbABuffer, pjbBuffer, JNI_ABORT);
        return -1;
    }
    pjbBuffer = pjbBuffer + jiOffset;
    bool bReliable = jbReliable ? true : false;
    bool bSequenced = jbSequenced ? true : false;

    int rc = pMocket->replace (bReliable, bSequenced, pjbBuffer, jiLength, (uint16) jiOldTag, (uint16) jiNewTag,
        (uint8) jsPriority, (uint32) jlEnqueueTimeout, (uint32) jlRetryTimeout);
    if (rc < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), "replace failed");
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

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_cancel (JNIEnv *pEnv, jobject joThis, jboolean jbReliable,
                                                                  jboolean jbSequenced, jint jiTagId)
{
    if (jiTagId<0) {
        pEnv->ThrowNew(pEnv->FindClass ("java/lang/IllegalArgumentException"), "illegal argument: TagId cannot be negative");
        return -1;
    }
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return -1;
    }
    bool bReliable = jbReliable ? true : false;
    bool bSequenced = jbSequenced ? true : false;

    return pMocket->cancel (bReliable, bSequenced, (uint16) jiTagId);
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_setConnectionLingerTime (JNIEnv *pEnv, jobject joThis, jlong jlLingerTime)
{
    if (jlLingerTime<0) {
        pEnv->ThrowNew(pEnv->FindClass ("java/lang/IllegalArgumentException"), "illegal argument: TagId cannot be negative");
        return -1;
    }
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return -1;
    }
    return pMocket->setConnectionLingerTime((uint32)jlLingerTime);
}

JNIEXPORT jlong JNICALL Java_us_ihmc_mockets_Mocket_getConnectionLingetTime (JNIEnv *pEnv, jobject joThis)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return -1;
    }
    return pMocket->getConnectionLingerTime();
}

JNIEXPORT jobject JNICALL Java_us_ihmc_mockets_Mocket_getStatisticsNative (JNIEnv *pEnv, jobject joThis, jobject joStatistics)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return NULL;
    }
    MocketStats *pStats = pMocket->getStatistics();
    if (pStats == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - statistics is null");
        return NULL;
    }

    jclass jcStatistics = pEnv->GetObjectClass (joStatistics);
    jfieldID jfStatistics = pEnv->GetFieldID (jcStatistics, "_statistics", "J");
    pEnv->SetLongField (joStatistics, jfStatistics, UtilWrapper::toJLong (pStats));

    return joStatistics;
}

JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_registerStatusListener (JNIEnv *pEnv, jobject joThis, jobject joListener)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
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
    pMocket->registerPeerReachableCallback (reachablePeerCallback, (void*) joListenerGlobalRef);
    pMocket->registerSuspendReceivedWarningCallback (suspendReceivedCallback, (void*) joListenerGlobalRef);
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_getMaximumMTU (JNIEnv *pEnv, jobject joThis)
{
    return Mocket::getMaximumMTU();
}

JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_getPeerNameNative (JNIEnv *pEnv, jobject joThis)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return;
    }
    jfieldID jfRemoteAddr = pEnv->GetFieldID (jcMocket, "_remoteAddress", "Ljava/lang/String;");
    
    if (jfRemoteAddr == NULL) {
        printf("could not find field _remoteAddress!!!\n");
        return;
    }
   
    InetAddr *pAddr = new InetAddr();
    pAddr->setIPAddress (pMocket->getRemoteAddress());
    const char *pcAddr = pAddr->getIPAsString();
    jstring jsAddr = pEnv->NewStringUTF (pcAddr);
    pEnv->SetObjectField (joThis, jfRemoteAddr, jsAddr);

    jfieldID jfRemotePort = pEnv->GetFieldID (jcMocket, "_remotePort", "I");
    jint jiRemotePort = pEnv->GetIntField (joThis, jfRemotePort);
    pEnv->SetIntField (joThis, jfRemotePort, (jint) pMocket->getRemotePort());
}


JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Mocket_activateCongestionControl (JNIEnv *pEnv, jobject joThis)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return -1;
    }
    return pMocket->activateCongestionControl();
}

JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_debugStateCapture (JNIEnv *pEnv, jobject joThis)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return;
    }
    pMocket->debugStateCapture();
}

bool unreachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec)
{
    JNIEnv *pEnv = JNU_GetEnv();
    jint jiRes;
    #ifdef JNI_VERSION_1_2
        #if !defined (ANDROID)
            jiRes = cached_jvm->AttachCurrentThread ((void**) &pEnv, NULL);
        #else
             jiRes = cached_jvm->AttachCurrentThread (&pEnv, NULL);
        #endif
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

bool reachablePeerCallback (void *pCallbackArg, unsigned long ulMilliSec)
{
    JNIEnv *pEnv = JNU_GetEnv();
    jint jiRes;
    #ifdef JNI_VERSION_1_2
        #if !defined (ANDROID)
            jiRes = cached_jvm->AttachCurrentThread ((void**) &pEnv, NULL);
        #else
            jiRes = cached_jvm->AttachCurrentThread (&pEnv, NULL);
        #endif
    #else
        jiRes = cached_jvm->AttachCurrentThread (&pEnv, NULL);
    #endif

    if (jiRes < 0) {
        printf ("MocketWrapper-reachablePeerCallback: attach jvm failed\n");
        cached_jvm->DetachCurrentThread();
        return false;
    }
    jobject joListenerGlobalRef = (jobject) pCallbackArg;
    jobject joListener = pEnv->NewLocalRef (joListenerGlobalRef);
    jclass jcMocketStatusListener = (jclass) pEnv->NewLocalRef (jcMocketStatusListenerGlobalRef);
    if (jcMocketStatusListener == NULL) {
        printf ("MocketWrapper-reachablePeerCallback: cannot invoke peer reachable callback because the reference to the MocketStatusListener class is NULL\n");
        cached_jvm->DetachCurrentThread();
        return false;
    }
    if (joListener == NULL) {
        printf ("MocketWrapper-reachablePeerCallback: cannot invoke peer reachable callback because the reference to the listener is NULL\n");
        cached_jvm->DetachCurrentThread();
        return false;
    }
    jboolean jbResult = pEnv->CallBooleanMethod (joListener, jmPeerReachableGlobal, (jlong)ulMilliSec);
    cached_jvm->DetachCurrentThread();
    return (jbResult ? true : false);
}

bool suspendReceivedCallback (void *pCallbackArg, unsigned long ulMilliSec)
{
    JNIEnv *pEnv = JNU_GetEnv();
    jint jiRes;
    #ifdef JNI_VERSION_1_2
        #if !defined (ANDROID)
            jiRes = cached_jvm->AttachCurrentThread ((void**) &pEnv, NULL);
        #else
            jiRes = cached_jvm->AttachCurrentThread (&pEnv, NULL);
        #endif
    #else
        jiRes = cached_jvm->AttachCurrentThread (&pEnv, NULL);
    #endif

    if (jiRes < 0) {
        printf ("MocketWrapper-suspendReceivedCallback: attach jvm failed\n");
        cached_jvm->DetachCurrentThread();
        return false;
    }
    jobject joListenerGlobalRef = (jobject) pCallbackArg;
    jobject joListener = pEnv->NewLocalRef (joListenerGlobalRef);
    jclass jcMocketStatusListener = (jclass) pEnv->NewLocalRef (jcMocketStatusListenerGlobalRef);
    if (jcMocketStatusListener == NULL) {
        printf ("MocketWrapper-suspendReceivedCallback: cannot invoke suspend received callback because the reference to the MocketStatusListener class is NULL\n");
        cached_jvm->DetachCurrentThread();
        return false;
    }
    if (joListener == NULL) {
        printf ("MocketWrapper-suspendReceivedCallback: cannot invoke suspend received callback because the reference to the listener is NULL\n");
        cached_jvm->DetachCurrentThread();
        return false;
    }
    jboolean jbResult = pEnv->CallBooleanMethod (joListener, jmSuspendReceivedGlobal, (jlong)ulMilliSec);
    cached_jvm->DetachCurrentThread();
    return (jbResult ? true : false);
}

JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_useTwoWayHandshake (JNIEnv *pEnv, jobject joThis)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return;
    }

    pMocket->useTwoWayHandshake();
}

/*JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_useTransmissionRateModulation (JNIEnv *pEnv, jobject joThis)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return;
    }

    pMocket->useTransmissionRateModulation();
}*/

JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_setKeepAliveTimeout (JNIEnv *pEnv, jobject joThis, jint jiTimeout)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return;
    }

    if (jiTimeout < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/IllegalArgumentException"), "illegal argument: Input value cannot be negative");
        return;
    }

    pMocket->setKeepAliveTimeout ((uint32) jiTimeout);
}

JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_disableKeepAlive (JNIEnv *pEnv, jobject joThis)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return;
    }

    pMocket->disableKeepAlive();
}

JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_setInitialAssumedRTT (JNIEnv *pEnv, jobject joThis, jlong jlRTT)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return;
    }

    if (jlRTT < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/IllegalArgumentException"), "illegal argument: Input value cannot be negative");
        return;
    }

    pMocket->setInitialAssumedRTT ((uint32) jlRTT);
}

JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_setMaximumRTO (JNIEnv *pEnv, jobject joThis, jlong jlRTO)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return;
    }

    if (jlRTO < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/IllegalArgumentException"), "illegal argument: Input value cannot be negative");
        return;
    }

    pMocket->setMaximumRTO ((uint32) jlRTO);
}

JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_setMinimumRTO (JNIEnv *pEnv, jobject joThis, jlong jlRTO)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return;
    }

    if (jlRTO < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/IllegalArgumentException"), "illegal argument: Input value cannot be negative");
        return;
    }

    pMocket->setMinimumRTO ((uint32) jlRTO);
}

JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_setRTOFactor (JNIEnv *pEnv, jobject joThis, jdouble jdRTOFactor)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return;
    }

    pMocket->setRTOFactor ((float) jdRTOFactor);
}

JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_setRTOConstant (JNIEnv *pEnv, jobject joThis, jint jiRTOConst)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return;
    }

    if (jiRTOConst < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/IllegalArgumentException"), "illegal argument: Input value cannot be negative");
        return;
    }

    pMocket->setRTOConstant ((uint16) jiRTOConst);
}

JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_disableRetransmitCountFactorInRTO (JNIEnv *pEnv, jobject joThis)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return;
    }

    pMocket->disableRetransmitCountFactorInRTO();
}

JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_setMaximumWindowSize (JNIEnv *pEnv, jobject joThis, jlong jlWindSize)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return;
    }

    if (jlWindSize < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/IllegalArgumentException"), "illegal argument: Input value cannot be negative");
        return;
    }

    pMocket->setMaximumWindowSize ((uint32) jlWindSize);
}

JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_setSAckTransmitTimeout (JNIEnv *pEnv, jobject joThis, jint jiSackTransTO)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return;
    }

    if (jiSackTransTO < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/IllegalArgumentException"), "illegal argument: Input value cannot be negative");
        return;
    }

    pMocket->setSAckTransmitTimeout ((uint16) jiSackTransTO);
}

JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_setConnectTimeout (JNIEnv *pEnv, jobject joThis, jlong jlConnectTO)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return;
    }

    if (jlConnectTO < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/IllegalArgumentException"), "illegal argument: Input value cannot be negative");
        return;
    }

    pMocket->setConnectTimeout ((uint32) jlConnectTO);
}

JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_setUDPReceiveConnectionTimeout (JNIEnv *pEnv, jobject joThis, jint jiUDPConnectTO)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return;
    }

    if (jiUDPConnectTO < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/IllegalArgumentException"), "illegal argument: Input value cannot be negative");
        return;
    }

    pMocket->setUDPReceiveConnectionTimeout ((uint16) jiUDPConnectTO);
}

JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_setUDPReceiveTimeout (JNIEnv *pEnv, jobject joThis, jint jiUDPRecTO)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return;
    }

    if (jiUDPRecTO < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/IllegalArgumentException"), "illegal argument: Input value cannot be negative");
        return;
    }

    pMocket->setUDPReceiveTimeout ((uint16) jiUDPRecTO);
}

JNIEXPORT void JNICALL Java_us_ihmc_mockets_Mocket_setIdentifier (JNIEnv *pEnv, jobject joThis, jstring jsIdentifier)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return;
    }

    if (jsIdentifier == NULL) {
        pMocket->setIdentifier (NULL);
    }
    else {
        const char *pszIdentifier = pEnv->GetStringUTFChars (jsIdentifier, NULL);
        pMocket->setIdentifier (pszIdentifier);
        pEnv->ReleaseStringUTFChars (jsIdentifier, pszIdentifier);
    }
}

JNIEXPORT jstring JNICALL Java_us_ihmc_mockets_Mocket_getIdentifier (JNIEnv *pEnv, jobject joThis)
{
    jclass jcMocket = pEnv->GetObjectClass (joThis);
    jfieldID jfMocket = pEnv->GetFieldID (jcMocket, "_mocket", "J");
    jlong jlMocket = pEnv->GetLongField (joThis, jfMocket);
    Mocket *pMocket = (Mocket *) UtilWrapper::toPtr (jlMocket);
    if (pMocket == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "MocketsJavaWrapper not initialized - mocket is null");
        return NULL;
    }

    const char *pszIdentifier = pMocket->getIdentifier();
    if (pszIdentifier == NULL) {
        return NULL;
    }
    else {
        jstring jsIdentifier = pEnv->NewStringUTF (pszIdentifier);
        return jsIdentifier;
    }
}
