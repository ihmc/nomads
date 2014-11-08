/*
 * StatisticsWrapper.cpp
 *
 * This file is part of the IHMC Mockets Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#include "StatisticsWrapper.h"

#include "Mocket.h"

#include "UtilWrapper.h"

JNIEXPORT jlong JNICALL Java_us_ihmc_mockets_Mocket_00024Statistics_getRetransmittedPacketCount (JNIEnv *pEnv, jobject joThis)
{
    jclass jcStatistics = pEnv->GetObjectClass (joThis);
    jfieldID jfStatistics = pEnv->GetFieldID (jcStatistics, "_statistics", "J");
    jlong jlStatistics = pEnv->GetLongField (joThis, jfStatistics);
    MocketStats *pStats = (MocketStats *) UtilWrapper::toPtr (jlStatistics);
    if (pStats == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "StatisticsWrapper not initialized - statistics is null");
        return -1;
    }
    return (uint32) pStats->getRetransmitCount();
}

JNIEXPORT jlong JNICALL Java_us_ihmc_mockets_Mocket_00024Statistics_getSentPacketCount (JNIEnv *pEnv, jobject joThis)
{
    jclass jcStatistics = pEnv->GetObjectClass (joThis);
    jfieldID jfStatistics = pEnv->GetFieldID (jcStatistics, "_statistics", "J");
    jlong jlStatistics = pEnv->GetLongField (joThis, jfStatistics);
    MocketStats *pStats = (MocketStats *) UtilWrapper::toPtr (jlStatistics);
    if (pStats == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "StatisticsWrapper not initialized - statistics is null");
        return -1;
    }
    return (uint32) pStats->getSentPacketCount();
}

JNIEXPORT jlong JNICALL Java_us_ihmc_mockets_Mocket_00024Statistics_getSentByteCount (JNIEnv *pEnv, jobject joThis)
{
    jclass jcStatistics = pEnv->GetObjectClass (joThis);
    jfieldID jfStatistics = pEnv->GetFieldID (jcStatistics, "_statistics", "J");
    jlong jlStatistics = pEnv->GetLongField (joThis, jfStatistics);
    MocketStats *pStats = (MocketStats *) UtilWrapper::toPtr (jlStatistics);
    if (pStats == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "StatisticsWrapper not initialized - statistics is null");
        return -1;
    }
    return (uint32) pStats->getSentByteCount();
}

JNIEXPORT jlong JNICALL Java_us_ihmc_mockets_Mocket_00024Statistics_getReceivedPacketCount (JNIEnv *pEnv, jobject joThis)
{
    jclass jcStatistics = pEnv->GetObjectClass (joThis);
    jfieldID jfStatistics = pEnv->GetFieldID (jcStatistics, "_statistics", "J");
    jlong jlStatistics = pEnv->GetLongField (joThis, jfStatistics);
    MocketStats *pStats = (MocketStats *) UtilWrapper::toPtr (jlStatistics);
    if (pStats == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "StatisticsWrapper not initialized - statistics is null");
        return -1;
    }
    return (uint32) pStats->getReceivedPacketCount();
}

JNIEXPORT jlong JNICALL Java_us_ihmc_mockets_Mocket_00024Statistics_getReceivedByteCount (JNIEnv *pEnv, jobject joThis)
{
    jclass jcStatistics = pEnv->GetObjectClass (joThis);
    jfieldID jfStatistics = pEnv->GetFieldID (jcStatistics, "_statistics", "J");
    jlong jlStatistics = pEnv->GetLongField (joThis, jfStatistics);
    MocketStats *pStats = (MocketStats *) UtilWrapper::toPtr (jlStatistics);
    if (pStats == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "StatisticsWrapper not initialized - statistics is null");
        return -1;
    }
    return (uint32) pStats->getReceivedByteCount();
}

/*JNIEXPORT jlong JNICALL Java_us_ihmc_mockets_Mocket_00024Statistics_getDiscardedPacketCount (JNIEnv *pEnv, jobject joThis)
{
    jclass jcStatistics = pEnv->GetObjectClass (joThis);
    jfieldID jfStatistics = pEnv->GetFieldID (jcStatistics, "_statistics", "J");
    jlong jlStatistics = pEnv->GetLongField (joThis, jfStatistics);
    MocketStats *pStats = (MocketStats *) UtilWrapper::toPtr (jlStatistics);
    if (pStats == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "StatisticsWrapper not initialized - statistics is null");
        return -1;
    }
    return (uint32) pStats->getNoRoomDiscardedPacketCount() + pStats->getDuplicatedDiscardedPacketCount();
}
*/

JNIEXPORT jlong JNICALL Java_us_ihmc_mockets_Mocket_00024Statistics_getDuplicatedDiscardedPacketCount (JNIEnv *pEnv, jobject joThis)
{
    jclass jcStatistics = pEnv->GetObjectClass (joThis);
    jfieldID jfStatistics = pEnv->GetFieldID (jcStatistics, "_statistics", "J");
    jlong jlStatistics = pEnv->GetLongField (joThis, jfStatistics);
    MocketStats *pStats = (MocketStats *) UtilWrapper::toPtr (jlStatistics);
    if (pStats == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "StatisticsWrapper not initialized - statistics is null");
        return -1;
    }
    return (uint32) pStats->getDuplicatedDiscardedPacketCount();
}

JNIEXPORT jlong JNICALL Java_us_ihmc_mockets_Mocket_00024Statistics_getNoRoomDiscardedPacketCount (JNIEnv *pEnv, jobject joThis)
{
    jclass jcStatistics = pEnv->GetObjectClass (joThis);
    jfieldID jfStatistics = pEnv->GetFieldID (jcStatistics, "_statistics", "J");
    jlong jlStatistics = pEnv->GetLongField (joThis, jfStatistics);
    MocketStats *pStats = (MocketStats *) UtilWrapper::toPtr (jlStatistics);
    if (pStats == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "StatisticsWrapper not initialized - statistics is null");
        return -1;
    }
    return (uint32) pStats->getNoRoomDiscardedPacketCount();
}
