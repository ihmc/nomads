/*
 * ParamsWrapper.cpp
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#include <stdio.h>
#include "ParamsWrapper.h"
#include "MessageSender.h"
#include "UtilWrapper.h"

JNIEXPORT void JNICALL Java_us_ihmc_mockets_Params_init (JNIEnv *pEnv, jobject joThis, jint jiTag, jshort jsPriority,
                                                         jlong jlEnqueueTimeout, jlong jlRetryTimeout)
{
    jclass jcParams = pEnv->GetObjectClass (joThis);
    jfieldID jfParams = pEnv->GetFieldID (jcParams, "_params", "J");
    MessageSender::Params *pParams = new MessageSender::Params ((uint16) jiTag, (uint8) jsPriority,
        (uint32) jlEnqueueTimeout, (uint32) jlRetryTimeout);
    if (pParams == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "ParamsWrapper not initialized - mocket is null");
        return;      
    }
    pEnv->SetLongField (joThis, jfParams, UtilWrapper::toJLong (pParams));
}

JNIEXPORT void JNICALL Java_us_ihmc_mockets_Params_dispose (JNIEnv *pEnv, jobject joThis)
{
    jclass jcParams = pEnv->GetObjectClass (joThis);
    jfieldID jfParams = pEnv->GetFieldID (jcParams, "_params", "J");
    jlong jlParams = pEnv->GetLongField (joThis, jfParams);
    MessageSender::Params *pParams = (MessageSender::Params *) UtilWrapper::toPtr (jlParams);
    if (pParams == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "ParamsWrapper not initialized - mocket is null");
        return;      
    }
    delete pParams;
}

JNIEXPORT jint JNICALL Java_us_ihmc_mockets_Params_getTag (JNIEnv *pEnv, jobject joThis)
{
    jclass jcParams = pEnv->GetObjectClass (joThis);
    jfieldID jfParams = pEnv->GetFieldID (jcParams, "_params", "J");
    jlong jlParams = pEnv->GetLongField (joThis, jfParams);
    MessageSender::Params *pParams = (MessageSender::Params *) UtilWrapper::toPtr (jlParams);
    if (pParams == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "ParamsWrapper not initialized - mocket is null");
        return -1;      
    }
    return pParams->getTag();
}

JNIEXPORT jshort JNICALL Java_us_ihmc_mockets_Params_getPriority (JNIEnv *pEnv, jobject joThis)
{
    jclass jcParams = pEnv->GetObjectClass (joThis);
    jfieldID jfParams = pEnv->GetFieldID (jcParams, "_params", "J");
    jlong jlParams = pEnv->GetLongField (joThis, jfParams);
    MessageSender::Params *pParams = (MessageSender::Params *) UtilWrapper::toPtr (jlParams);
    if (pParams == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "ParamsWrapper not initialized - mocket is null");
        return -1;      
    }
    return pParams->getPriority();
}

JNIEXPORT jlong JNICALL Java_us_ihmc_mockets_Params_getEnqueueTimeout (JNIEnv *pEnv, jobject joThis)
{
    jclass jcParams = pEnv->GetObjectClass (joThis);
    jfieldID jfParams = pEnv->GetFieldID (jcParams, "_params", "J");
    jlong jlParams = pEnv->GetLongField (joThis, jfParams);
    MessageSender::Params *pParams = (MessageSender::Params *) UtilWrapper::toPtr (jlParams);
    if (pParams == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "ParamsWrapper not initialized - mocket is null");
        return -1;      
    }
    return pParams->getEnqueueTimeout();
}

JNIEXPORT jlong JNICALL Java_us_ihmc_mockets_Params_getRetryTimeout (JNIEnv *pEnv, jobject joThis)
{
    jclass jcParams = pEnv->GetObjectClass (joThis);
    jfieldID jfParams = pEnv->GetFieldID (jcParams, "_params", "J");
    jlong jlParams = pEnv->GetLongField (joThis, jfParams);
    MessageSender::Params *pParams = (MessageSender::Params *) UtilWrapper::toPtr (jlParams);
    if (pParams == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "ParamsWrapper not initialized - mocket is null");
        return -1;      
    }
    return pParams->getRetryTimeout();
}
