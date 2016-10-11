/*
 * DisServiceJNIWrapper.h
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2016 IHMC.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 (GPLv3) as published by the Free Software Foundation.
 *
 * U.S. Government agencies and organizations may redistribute
 * and/or modify this program under terms equivalent to
 * "Government Purpose Rights" as defined by DFARS 
 * 252.227-7014(a)(12) (February 2014).
 *
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 */

#ifndef INCL_DIS_SERVICE_JNI_WRAPPER_H
#define INCL_DIS_SERVICE_JNI_WRAPPER_H

#include "jni.h"

#ifdef __cplusplus
    extern "C" {
#endif

JNIEXPORT jint JNICALL JNI_OnLoad (JavaVM *pJVM, void *pReserved);
JNIEXPORT void JNICALL Java_us_ihmc_aci_disService_DisseminationService_init(JNIEnv *pEnv, jobject joThis, jstring jsConfigFile, jobjectArray joaOverrideAttrs, jobjectArray joaOverrideValues);
JNIEXPORT jstring JNICALL Java_us_ihmc_aci_disService_DisseminationService_getNodeId (JNIEnv *pEnv, jobject joThis);
JNIEXPORT jobject JNICALL Java_us_ihmc_aci_disService_DisseminationService_getPeerList (JNIEnv *pEnv, jobject joThis);
JNIEXPORT jstring JNICALL Java_us_ihmc_aci_disService_DisseminationService_push (JNIEnv *pEnv, jobject joThis, jstring jsGroupName, jstring jsObjectId, jstring jsInstanceId, jstring jsMIMEType, jbyteArray jbaMetaData, jbyteArray jbaData, jlong jlExpiration, jshort jsHistoryWindow, jshort jsTag, jbyte jbPriority);
JNIEXPORT jstring JNICALL Java_us_ihmc_aci_disService_DisseminationService_makeAvailable (JNIEnv *pEnv, jobject joThis, jstring jsGroupName, jstring jsObjectId, jstring jsInstanceId, jbyteArray jbaMetaData, jbyteArray jbaData, jstring jsMIMEType, jlong jlExpiration, jshort jsHistoryWindow, jshort jsTag, jbyte jbPriority);
JNIEXPORT void JNICALL Java_us_ihmc_aci_disService_DisseminationService_cancel__Ljava_lang_String_2 (JNIEnv *pEnv, jobject joThis, jstring jsId);
JNIEXPORT void JNICALL Java_us_ihmc_aci_disService_DisseminationService_cancel__S (JNIEnv *pEnv, jobject joThis, jshort jsTag);
JNIEXPORT jboolean JNICALL Java_us_ihmc_aci_disService_DisseminationService_addFilter (JNIEnv *pEnv, jobject joThis, jstring jsGroupName, jshort jsTag);
JNIEXPORT jboolean JNICALL Java_us_ihmc_aci_disService_DisseminationService_removeFilter (JNIEnv *pEnv, jobject joThis, jstring jsGroupName, jshort jsTag);
JNIEXPORT jboolean JNICALL Java_us_ihmc_aci_disService_DisseminationService_requestMoreChunks__Ljava_lang_String_2Ljava_lang_String_2I (JNIEnv *pEnv, jobject joThis, jstring jsGroupName, jstring jsSenderNodeId, jint jiSeqId);
JNIEXPORT jboolean JNICALL Java_us_ihmc_aci_disService_DisseminationService_requestMoreChunks__Ljava_lang_String_2 (JNIEnv *pEnv, jobject joThis, jstring jsMessageId);
JNIEXPORT jbyteArray JNICALL Java_us_ihmc_aci_disService_DisseminationService_retrieve__Ljava_lang_String_2I (JNIEnv *pEnv, jobject joThis, jstring jsId, jint jiTimeout);
JNIEXPORT jint JNICALL Java_us_ihmc_aci_disService_DisseminationService_retrieve__Ljava_lang_String_2Ljava_lang_String_2 (JNIEnv *pEnv, jobject joThis, jstring jsId, jstring jsFilePath);
JNIEXPORT jboolean JNICALL Java_us_ihmc_aci_disService_DisseminationService_historyRequest (JNIEnv *pEnv, jobject joThis, jstring jsGroupName, jshort jsTag, jshort jsHistoryLength, jlong jlTimeout);
JNIEXPORT jboolean JNICALL Java_us_ihmc_aci_disService_DisseminationService_subscribe__Ljava_lang_String_2BZZZ (JNIEnv *pEnv, jobject joThis, jstring jsGroupName, jbyte jbPriority, jboolean jbGroupReliable, jboolean jbMsgReliable, jboolean jbSequenced);
JNIEXPORT jboolean JNICALL Java_us_ihmc_aci_disService_DisseminationService_subscribe__Ljava_lang_String_2SBZZZ (JNIEnv *pEnv, jobject joThis, jstring jsGroupName, jshort jsTag, jbyte jbPriority, jboolean jbGroupReliable, jboolean jbMsgReliable, jboolean jbSequenced);
JNIEXPORT jboolean JNICALL Java_us_ihmc_aci_disService_DisseminationService_subscribe__Ljava_lang_String_2BLjava_lang_String_2BZZZ (JNIEnv *pEnv, jobject joThis, jstring jsGroupName, jbyte jbPredicateType, jstring jsPredicate, jbyte jbPriority, jboolean jbGroupReliable, jboolean jbMsgReliable, jboolean jbSequenced);
JNIEXPORT jboolean JNICALL Java_us_ihmc_aci_disService_DisseminationService_unsubscribe__Ljava_lang_String_2 (JNIEnv *pEnv, jobject joThis, jstring jsGroupName);
JNIEXPORT jboolean JNICALL Java_us_ihmc_aci_disService_DisseminationService_unsubscribe__Ljava_lang_String_2S (JNIEnv *pEnv, jobject joThis, jstring jsGroupName, jshort jsTag);
JNIEXPORT void JNICALL Java_us_ihmc_aci_disService_DisseminationService_registerDisseminationServiceListener (JNIEnv *pEnv, jobject joThis, jobject joListener);
JNIEXPORT void JNICALL Java_us_ihmc_aci_disService_DisseminationService_registerPeerStatusListener (JNIEnv *pEnv, jobject joThis, jobject joListener);
JNIEXPORT void JNICALL Java_us_ihmc_aci_disService_DisseminationService_resetTransmissionHistory (JNIEnv *pEnv, jobject joThis);
JNIEXPORT jstring JNICALL Java_us_ihmc_aci_disService_DisseminationService_getNextPushId (JNIEnv *pEnv, jobject joThis, jstring jsGroupName);

#ifdef __cplusplus
    }
#endif

#endif   // #ifndef INCL_DIS_SERVICE_JNI_WRAPPER_H
