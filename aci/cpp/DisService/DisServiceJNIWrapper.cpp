/*
 * DisServiceJNIWrapper.cpp
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

#include "DisServiceJNIWrapper.h"

#include "DisseminationService.h"
#include "DisseminationServiceListener.h"
#include "PeerStatusListener.h"

#include "JNIUtils.h"

#include "ConfigManager.h"
#include "Logger.h"
#include "NLFLib.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

static JavaVM *pCachedJVM;
static jclass jcDisseminationServiceListenerGlobalRef;
static jobject joDisseminationServiceListenerGlobalRef;
//static jmethodID jmDataArrivedGlobal;
//static jmethodID jmChunkArrivedGlobal;
//static jmethodID jmMetadataArrivedGlobal;
//static jmethodID jmDataAvailableGlobal;
static jclass jcPeerStatusListenerGlobalRef;
static jobject joPeerStatusListenerGlobalRef;
static jmethodID jmNewPeerGlobal;
static jmethodID jmDeadPeerGlobal;

class DisServiceJNIWrapperListener : public DisseminationServiceListener
{
    public:
        bool dataArrived (uint16 ui16ClientId, const char *pszSender, const char *pszGroupName,
                          uint32 ui32SeqId, const char *pszObjectId, const char *pszInstanceId,
                          const char *pszAnnotatedObjId, const char *pszMimeType, const void *pData, uint32 ui32Length,
                          uint32 ui32MetadataLength, uint16 ui16Tag, uint8 ui8Priority,
                          const char *pszQueryId);

        bool chunkArrived (uint16 ui16ClientId, const char *pszSender, const char *pszGroupName,
                           uint32 ui32SeqId, const char *pszObjectId, const char *pszInstanceId,
                           const char *pszMimeType, const void *pChunk, uint32 ui32Length,
                           uint8 ui8NChunks, uint8 ui8TotNChunks, const char *pszChunkedMsgId,
                           uint16 ui16Tag, uint8 ui8Priority, const char *pszQueryId);

        bool metadataArrived (uint16 ui16ClientId, const char *pszSender, const char *pszGroupName,
                              uint32 ui32SeqId, const char *pszObjectId, const char *pszInstanceId,
                              const char *pszMimeType, const void *pMetadata, uint32 ui32MetadataLength,
                              bool bDataChunked, uint16 ui16Tag, uint8 ui8Priority, const char *pszQueryId);

        bool dataAvailable (uint16 ui16ClientId, const char *pszSender, const char *pszGroupName,
                            uint32 ui32SeqId, const char *pszObjectId, const char *pszInstanceId,
                            const char *pszMimeType, const char *pszRefObjId, const void *pMetadata,
                            uint32 ui32MetadataLength, uint16 ui16Tag, uint8 ui8Priority, const char *pszQueryId);
};

class PeerStatusJNIWrapperListener : public PeerStatusListener
{
    public:
        bool newPeer (const char *pszPeerNodeId);
        bool deadPeer (const char *pszPeerNodeId);
};

static DisServiceJNIWrapperListener dsListener;
static PeerStatusJNIWrapperListener psListener;

JNIEnv * JNU_GetEnv (void)
{
    JNIEnv *pEnv;
    pCachedJVM->GetEnv ((void **)&pEnv, JNI_VERSION_1_2);
    return pEnv;
}

JNIEXPORT jint JNICALL JNI_OnLoad (JavaVM *pJVM, void *pReserved)
{
    JNIEnv *pEnv;
    jclass jcCls;

    pCachedJVM = pJVM; // cache the JavaVM pointer

    if (pJVM->GetEnv ((void **) &pEnv, JNI_VERSION_1_2)) {
        return JNI_ERR; // JNI version not supported
    }

    // Get access to the DisseminationServiceListener interface
    jcCls = pEnv->FindClass ("us/ihmc/aci/disService/DisseminationServiceListener");
    if (jcCls == NULL) {
        checkAndLogMsg ("JNI_OnLoad", Logger::L_MildError,
                        "could not find class us/ihmc/aci/disService/DisseminationServiceListener\n");
        return JNI_ERR;
    }
    #if !defined (ANDROID) //No support for NewWeakGlobalRef on Dalvik JVM (ANDROID)
        // Use weak global ref to allow the class to be unloaded
        jcDisseminationServiceListenerGlobalRef = (jclass) pEnv->NewWeakGlobalRef (jcCls);
    #else
        jcDisseminationServiceListenerGlobalRef = (jclass) pEnv->NewGlobalRef (jcCls);
    #endif
    if (jcDisseminationServiceListenerGlobalRef == NULL) {
        checkAndLogMsg ("JNI_OnLoad", Logger::L_MildError,
                        "could not create global reference for class us/ihmc/aci/disService/DisseminationServiceListener\n");
        return JNI_ERR;
    }

    /*
    // Get access to the dataArrived method in the DisseminationStatusListener interface
    jmDataArrivedGlobal = pEnv->GetMethodID (jcDisseminationServiceListenerGlobalRef, "dataArrived", "(Ljava/lang/String;Ljava/lang/String;I[BISSB)Z");
    if (jmDataArrivedGlobal == NULL) {
        checkAndLogMsg ("JNI_OnLoad", Logger::L_MildError,
                        "could not find dataArrived method\n");
        return JNI_ERR;
    }

    // Get access to the chunkArrived method in the DisseminationStatusListener interface
    jmChunkArrivedGlobal = pEnv->GetMethodID (jcDisseminationServiceListenerGlobalRef, "chunkArrived", "(Ljava/lang/String;Ljava/lang/String;I[BSSLjava/lang/String;SSB)Z");
    if (jmChunkArrivedGlobal == NULL) {
        checkAndLogMsg ("JNI_OnLoad", Logger::L_MildError,
                        "could not find chunkArrived method\n");
        return JNI_ERR;
    }

    // Get access to the metadataArrived method in the DisseminationStatusListener interface
    jmMetadataArrivedGlobal = pEnv->GetMethodID (jcDisseminationServiceListenerGlobalRef, "metadataArrived", "(Ljava/lang/String;Ljava/lang/String;I[BZSSB)Z");
    if (jmMetadataArrivedGlobal == NULL) {
        checkAndLogMsg ("JNI_OnLoad", Logger::L_MildError,
                        "could not find metadataArrived method\n");
        return JNI_ERR;
    }

    // Get access to the dataArrived method in the DisseminationStatusListener interface
    jmDataAvailableGlobal = pEnv->GetMethodID (jcDisseminationServiceListenerGlobalRef, "dataAvailable", "(Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;[BSSB)Z");
    if (jmDataAvailableGlobal == NULL) {
        checkAndLogMsg ("JNI_OnLoad", Logger::L_MildError,
                        "could not find dataAvailable method\n");
        return JNI_ERR;
    }
    */

    // Get access to the PeerStatusListener interface
    jcCls = pEnv->FindClass ("us/ihmc/aci/disService/PeerStatusListener");
    if (jcCls == NULL) {
        checkAndLogMsg ("JNI_OnLoad", Logger::L_MildError,
                        "could not find class us/ihmc/aci/disService/PeerStatusListener\n");
        return JNI_ERR;
    }
    #if !defined (ANDROID) //No support for NewWeakGlobalRef on Dalvik JVM (ANDROID)
        // Use weak global ref to allow the class to be unloaded
        jcPeerStatusListenerGlobalRef = (jclass) pEnv->NewWeakGlobalRef (jcCls);
    #else
        jcPeerStatusListenerGlobalRef = (jclass) pEnv->NewGlobalRef (jcCls);
    #endif
    if (jcPeerStatusListenerGlobalRef == NULL) {
        checkAndLogMsg ("JNI_OnLoad", Logger::L_MildError,
                        "could not create global reference for class us/ihmc/aci/disService/PeerStatusListener\n");
        return JNI_ERR;
    }

    // Get access to the newPeer method in PeerStatusListener interface
    jmNewPeerGlobal = pEnv->GetMethodID (jcPeerStatusListenerGlobalRef, "newPeer", "(Ljava/lang/String;)Z");
    if (jmNewPeerGlobal == NULL) {
        checkAndLogMsg ("JNI_OnLoad", Logger::L_MildError,
                        "could not find newPeer method\n");
        return JNI_ERR;
    }

    // Get access to the deadPeer method in PeerStatusListener interface
    jmDeadPeerGlobal = pEnv->GetMethodID (jcPeerStatusListenerGlobalRef, "deadPeer", "(Ljava/lang/String;)Z");
    if (jmDeadPeerGlobal == NULL) {
        checkAndLogMsg ("JNI_OnLoad", Logger::L_MildError,
                        "could not find deadPeer method\n");
        return JNI_ERR;
    }

    return JNI_VERSION_1_2;
}

JNIEXPORT void JNICALL Java_us_ihmc_aci_disService_DisseminationService_init(JNIEnv *pEnv, jobject joThis, jstring jsConfigFile, jobjectArray joaOverrideAttrs, jobjectArray joaOverrideValues)
{
    int rc;
    jclass jcDisService = pEnv->GetObjectClass (joThis);
    jfieldID jfDisService = pEnv->GetFieldID (jcDisService, "_disService", "J");
    ConfigManager *pConfigManager = new ConfigManager();
    if (0 != (rc = pConfigManager->init())) {
        checkAndLogMsg ("DisServiceJNIWrapper::init", Logger::L_MildError,
                        "failed to initialize ConfigManager; rc = %d\n", rc);
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/RuntimeException"), "failed to initialize ConfigManager");
        return;
    }

    if (jsConfigFile != NULL) {
        const char *pszConfigFile = pEnv->GetStringUTFChars (jsConfigFile, NULL);
        if (pszConfigFile != NULL) {
            if (0 != (rc = pConfigManager->readConfigFile (pszConfigFile, true))) {
                checkAndLogMsg ("DisServiceJNIWrapper::init", Logger::L_MildError,
                                "failed to read config file <%s>; rc = %d\n", pszConfigFile, rc);
                pEnv->ReleaseStringUTFChars (jsConfigFile, pszConfigFile);
                pEnv->ThrowNew (pEnv->FindClass ("java/lang/RuntimeException"), "failed to read config file");
                return;
            }
        }
    }

    if ((joaOverrideAttrs != NULL) && (joaOverrideValues != NULL)) {
        jsize jsLength = pEnv->GetArrayLength (joaOverrideAttrs);
        if (jsLength != pEnv->GetArrayLength (joaOverrideValues)) {
            checkAndLogMsg ("DisServiceJNIWrapper::init", Logger::L_MildError,
                            "length of attributes and values not the same\n");
            pEnv->ThrowNew (pEnv->FindClass ("java/lang/RuntimeException"), "internal error - mismatch in number of attributes and values for config overrides");
            return;
        }
        for (int i = 0; i < jsLength; i++) {
            jstring jsAttr = (jstring) pEnv->GetObjectArrayElement (joaOverrideAttrs, i);
            jstring jsValue = (jstring) pEnv->GetObjectArrayElement (joaOverrideValues, i);
            if ((jsAttr != NULL) && (jsValue != NULL)) {
                const char *pszAttr = pEnv->GetStringUTFChars (jsAttr, NULL);
                const char *pszValue = pEnv->GetStringUTFChars (jsValue, NULL);
                if ((pszAttr != NULL) && (pszValue != NULL)) {
                    pConfigManager->setValue(pszAttr, pszValue);
                    checkAndLogMsg ("DisServiceJNIWrapper::init", Logger::L_Info,
                                    "set config parameter <%s=%s> from overrides\n", pszAttr, pszValue);
                }
                pEnv->ReleaseStringUTFChars (jsAttr, pszAttr);
                pEnv->ReleaseStringUTFChars (jsValue, pszValue);
            }
        }
    }

    // Initialize the Logger
    if (pConfigManager->getValueAsBool ("util.logger.enabled", true)) {
        if (!pLogger) {
            char szTimestamp[20];
            char szLogFileName[PATH_MAX];
            char szErrorLogFileName[PATH_MAX];
            if (NULL == generateTimestamp (szTimestamp, sizeof (szTimestamp))) {
                pEnv->ThrowNew (pEnv->FindClass ("java/lang/RuntimeException"), "failed to read generate timestamp");
                return;
            }
            const char *pszLogFilePath = pConfigManager->getValue ("util.logger.path", "");
            sprintf (szLogFileName, "%s%cds.jni.%s.log", pszLogFilePath, getPathSepChar(), szTimestamp);
            sprintf (szErrorLogFileName, "%s%cdserr.jni.%s.log", pszLogFilePath, getPathSepChar(), szTimestamp);

            pLogger = new Logger();
            if (pConfigManager->getValueAsBool ("util.logger.out.screen.enabled", false)) {
                pLogger->enableScreenOutput();
            }
            if (pConfigManager->getValueAsBool ("util.logger.out.file.enabled", true)) {
                pLogger->initLogFile (pConfigManager->getValue ("util.logger.out.file.path", szLogFileName), false);
                pLogger->enableFileOutput();
                pLogger->initErrorLogFile (pConfigManager->getValue ("util.logger.error.file.path", szErrorLogFileName), false);
                pLogger->enableErrorLogFileOutput();
            }
            uint8 ui8DbgDetLevel = (uint8) pConfigManager->getValueAsInt ("util.logger.detail", Logger::L_LowDetailDebug);
            switch (ui8DbgDetLevel) {
                case Logger::L_SevereError:
                case Logger::L_MildError:
                case Logger::L_Warning:
                case Logger::L_Info:
                case Logger::L_NetDetailDebug:
                case Logger::L_LowDetailDebug:
                case Logger::L_MediumDetailDebug:
                case Logger::L_HighDetailDebug:
                    pLogger->setDebugLevel (ui8DbgDetLevel);
                    pLogger->logMsg ("DisServiceJNIWrapper::init", Logger::L_Info,
                                     "Setting debug level to %d\n", ui8DbgDetLevel);
                    break;
                default:
                    pLogger->setDebugLevel(Logger::L_LowDetailDebug);
                    pLogger->logMsg ("DisServiceJNIWrapper::init",
                                     Logger::L_Info,
                                     "Invalid Logger detail debug level. Setting it to %d\n",
                                     Logger::L_LowDetailDebug);
            }
        }
    }

    pLogger->logMsg ("DisServiceJNIWrapper::init", Logger::L_Info,
                     "Version Timestamp %s %s\n", __DATE__, __TIME__);

    // Initializing and starting DisService
    DisseminationService *pDisService = (pConfigManager ? new DisseminationService (pConfigManager) : new DisseminationService());
    if (0 != (rc = pDisService->init())) {
        checkAndLogMsg ("DisServiceJNIWrapper::init", Logger::L_MildError,
                        "failed to initialize DisseminationService; rc = %d\n", rc);
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/RuntimeException"), "failed to initialize DisService");
        return;
    }
    pDisService->registerDisseminationServiceListener (0, &dsListener);
    pDisService->registerPeerStatusListener (0, &psListener);
    pDisService->start();
    pEnv->SetLongField (joThis, jfDisService, JNIUtils::toJLong (pDisService));
}

DisseminationService * checkAndRetrieveDisServicePtr (JNIEnv *pEnv, jobject joThis)
{
    jclass jcDisService = pEnv->GetObjectClass (joThis);
    jfieldID jfDisService = pEnv->GetFieldID (jcDisService, "_disService", "J");
    jlong jlDisService = pEnv->GetLongField (joThis, jfDisService);
    DisseminationService *pDisService = (DisseminationService *) JNIUtils::toPtr (jlDisService);
    if (pDisService == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "DisseminationServiceJNIWrapper not initialized - DisService is null");
        return NULL;
    }
    return pDisService;
}

JNIEXPORT jstring JNICALL Java_us_ihmc_aci_disService_DisseminationService_getNodeId (JNIEnv *pEnv, jobject joThis)
{
    DisseminationService *pDisService = checkAndRetrieveDisServicePtr (pEnv, joThis);
    if (pDisService == NULL) {
        return NULL;
    }
    return pEnv->NewStringUTF (pDisService->getNodeId());
}

JNIEXPORT jobject JNICALL Java_us_ihmc_aci_disService_DisseminationService_getPeerList (JNIEnv *pEnv, jobject joThis)
{
    DisseminationService *pDisService = checkAndRetrieveDisServicePtr (pEnv, joThis);
    if (pDisService == NULL) {
        return NULL;
    }
    char **ppszPeerList = pDisService->getPeerList();
    if (ppszPeerList == NULL) {
        return NULL;
    }
    jobject joList = JNIUtils::createListFromStringArray (pEnv, (const char **) ppszPeerList);
    free (ppszPeerList);
    return joList;
}

JNIEXPORT jstring JNICALL Java_us_ihmc_aci_disService_DisseminationService_push (JNIEnv *pEnv, jobject joThis, jstring jsGroupName, jstring jsObjectId, jstring jsInstanceId, jstring jsMIMEType,
                                                                                 jbyteArray jbaMetaData, jbyteArray jbaData, jlong jlExpiration, jshort jsHistoryWindow, jshort jsTag, jbyte jbPriority)
{
    DisseminationService *pDisService = checkAndRetrieveDisServicePtr (pEnv, joThis);
    if (pDisService == NULL) {
        return NULL;
    }

    // Get the Group Name
    String groupName;
    if (jsGroupName != NULL) {
        const char *pszGroupName = pEnv->GetStringUTFChars (jsGroupName, NULL);
        groupName = pszGroupName;
        pEnv->ReleaseStringUTFChars (jsGroupName, pszGroupName);
    }
    else {
        checkAndLogMsg ("DisServiceJNIWrapper::push", Logger::L_MildError,
                        "group name is null\n");
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "groupName is null");
        return NULL;
    }

    // Get the Object Id
    String objectId;
    if (jsObjectId != NULL) {
        const char *pszObjectId = pEnv->GetStringUTFChars (jsObjectId, NULL);
        objectId = pszObjectId;
        pEnv->ReleaseStringUTFChars (jsObjectId, pszObjectId);
    }

    // Get the Instance Id
    String instanceId;
    if (jsInstanceId != NULL) {
        const char *pszInstanceId = pEnv->GetStringUTFChars (jsInstanceId, NULL);
        instanceId = pszInstanceId;
        pEnv->ReleaseStringUTFChars (jsInstanceId, pszInstanceId);
    }

    // Get the MIME type
    String mimeType;
    if (jsMIMEType != NULL) {
        const char *pszMIMEType = pEnv->GetStringUTFChars (jsMIMEType, NULL);
        mimeType = pszMIMEType;
        pEnv->ReleaseStringUTFChars (jsMIMEType, pszMIMEType);
    }

    // Get the Metadata
    jsize jsMetaDataLen = 0;
    jbyte *pjbMetaData = NULL;
    if (jbaMetaData != NULL) {
        jsMetaDataLen = pEnv->GetArrayLength (jbaMetaData);
        pjbMetaData = pEnv->GetByteArrayElements (jbaMetaData, NULL);
        if ((jsMetaDataLen > 0) && (pjbMetaData == NULL)) {
            // Should not have happened
            checkAndLogMsg ("DisServiceJNIWrapper::push", Logger::L_MildError,
                            "meta data is null although length is > 0\n");
            pEnv->ThrowNew (pEnv->FindClass ("java/lang/RuntimeException"), "internal error obtaining metadata");
            return NULL;
        }
    }

    // Get the Data
    jsize jsDataLen = pEnv->GetArrayLength (jbaData);
    jbyte *pjbData = pEnv->GetByteArrayElements (jbaData, NULL);
    if ((jsDataLen > 0) && (pjbData == NULL)) {
        // Should not have happened
        checkAndLogMsg ("DisServiceJNIWrapper::push", Logger::L_MildError,
                        "data is null although length is > 0\n");
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/RuntimeException"), "internal error obtaining data");
        if (pjbMetaData != NULL) {
            pEnv->ReleaseByteArrayElements (jbaMetaData, pjbMetaData, 0);
            pjbMetaData = NULL;
        }
        return NULL;
    }

    // Do the push
    char szIdBuf [1024];
    int rc = pDisService->push (0, groupName, objectId, instanceId, mimeType, pjbMetaData, jsMetaDataLen, pjbData,
                                jsDataLen, jlExpiration, jsHistoryWindow, jsTag, jbPriority, szIdBuf, sizeof (szIdBuf));
    if (pjbMetaData != NULL) {
        pEnv->ReleaseByteArrayElements (jbaMetaData, pjbMetaData, 0);
        pjbMetaData = NULL;
    }
    if (pjbData != NULL) {
        pEnv->ReleaseByteArrayElements (jbaData, pjbData, 0);
        pjbData = NULL;
    }
    if (rc < 0) {
        checkAndLogMsg ("DisServiceJNIWrapper::push", Logger::L_MildError,
                        "push() failed with rc = %d\n");
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/RuntimeException"), "push failed");
        return NULL;
    }
    szIdBuf[1023] = '\0';    // Just in case - prevent a buffer overrun
    return pEnv->NewStringUTF (szIdBuf);
}

JNIEXPORT jstring JNICALL Java_us_ihmc_aci_disService_DisseminationService_makeAvailable (JNIEnv *pEnv, jobject joThis, jstring jsGroupName, jstring jsObjectId, jstring jsInstanceId, jbyteArray jbaMetaData, jbyteArray jbaData, jstring jsMIMEType, jlong jlExpiration, jshort jsHistoryWindow, jshort jsTag, jbyte jbPriority)
{
    DisseminationService *pDisService = checkAndRetrieveDisServicePtr (pEnv, joThis);
    if (pDisService == NULL) {
        return NULL;
    }

    // Get the Group Name
    const char *pszGroupName = pEnv->GetStringUTFChars (jsGroupName, NULL);
    if (pszGroupName == NULL) {
        checkAndLogMsg ("DisServiceJNIWrapper::makeAvailable", Logger::L_MildError,
                        "group name is null\n");
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "groupName is null");
        return NULL;
    }
    String groupName = pszGroupName;
    pEnv->ReleaseStringUTFChars (jsGroupName, pszGroupName);

    // Get the Object Id
    String objectId;
    if (jsObjectId != NULL) {
        const char *pszObjectId = pEnv->GetStringUTFChars (jsObjectId, NULL);
        objectId = pszObjectId;
        pEnv->ReleaseStringUTFChars (jsObjectId, pszObjectId);
    }

    // Get the Instance Id
    String instanceId;
    if (jsInstanceId != NULL) {
        const char *pszInstanceId = pEnv->GetStringUTFChars (jsInstanceId, NULL);
        instanceId = pszInstanceId;
        pEnv->ReleaseStringUTFChars (jsInstanceId, pszInstanceId);
    }

    // Get the MIME type
    String mimeType;
    if (jsMIMEType != NULL) {
        const char *pszMIMEType = pEnv->GetStringUTFChars (jsMIMEType, NULL);
        mimeType = pszMIMEType;
        pEnv->ReleaseStringUTFChars (jsMIMEType, pszMIMEType);
    }

    // Get the Metadata
    jsize jsMetaDataLen = 0;
    jbyte *pjbMetaData = NULL;
    if (jbaMetaData != NULL) {
        jsMetaDataLen = pEnv->GetArrayLength (jbaMetaData);
        pjbMetaData = pEnv->GetByteArrayElements (jbaMetaData, NULL);
        if ((jsMetaDataLen > 0) && (pjbMetaData == NULL)) {
            // Should not have happened
            checkAndLogMsg ("DisServiceJNIWrapper::makeAvailable", Logger::L_MildError,
                            "meta data is null although length is > 0\n");
            pEnv->ThrowNew (pEnv->FindClass ("java/lang/RuntimeException"), "internal error obtaining metadata");
            return NULL;
        }
    }

    // Get the Data
    jsize jsDataLen = pEnv->GetArrayLength (jbaData);
    jbyte *pjbData = pEnv->GetByteArrayElements (jbaData, NULL);
    if ((jsDataLen > 0) && (pjbData == NULL)) {
        // Should not have happened
        checkAndLogMsg ("DisServiceJNIWrapper::makeAvailable", Logger::L_MildError,
                        "data is null although length is > 0\n");
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/RuntimeException"), "internal error obtaining data");
        if (pjbMetaData != NULL) {
            pEnv->ReleaseByteArrayElements (jbaMetaData, pjbMetaData, 0);
            pjbMetaData = NULL;
        }
        return NULL;
    }

    // Do the makeAvailable
    char szIdBuf [1024];
    int rc = pDisService->makeAvailable (0, groupName, objectId, instanceId, pjbMetaData, jsMetaDataLen, pjbData, jsDataLen, mimeType, jlExpiration, jsHistoryWindow, jsTag, jbPriority, szIdBuf, sizeof (szIdBuf));
    if (pjbMetaData != NULL) {
        pEnv->ReleaseByteArrayElements (jbaMetaData, pjbMetaData, 0);
        pjbMetaData = NULL;
    }
    if (pjbData != NULL) {
        pEnv->ReleaseByteArrayElements (jbaData, pjbData, 0);
        pjbData = NULL;
    }
    if (rc < 0) {
        checkAndLogMsg ("DisServiceJNIWrapper::makeAvailable", Logger::L_MildError,
                        "makeAvailable() failed with rc = %d\n");
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/RuntimeException"), "makeAvailable failed");
        return NULL;
    }
    szIdBuf[1023] = '\0';    // Just in case - prevent a buffer overrun
    return pEnv->NewStringUTF (szIdBuf);
}

JNIEXPORT void JNICALL Java_us_ihmc_aci_disService_DisseminationService_cancel__Ljava_lang_String_2 (JNIEnv *pEnv, jobject joThis, jstring jsId)
{
    // Not yet implemented
    checkAndLogMsg ("DisServiceJNIWrapper::cancel1", Logger::L_MildError,
                    "not yet implemented\n");
}

JNIEXPORT void JNICALL Java_us_ihmc_aci_disService_DisseminationService_cancel__S (JNIEnv *pEnv, jobject joThis, jshort jsTag)
{
    // Not yet implemented
    checkAndLogMsg ("DisServiceJNIWrapper::cancel2", Logger::L_MildError,
                    "not yet implemented\n");
}

JNIEXPORT jboolean JNICALL Java_us_ihmc_aci_disService_DisseminationService_addFilter (JNIEnv *pEnv, jobject joThis, jstring jsGroupName, jshort jsTag)
{
    // Not yet implemented
    checkAndLogMsg ("DisServiceJNIWrapper::addFilter", Logger::L_MildError,
                    "not yet implemented\n");
    return (jboolean) false;
}

JNIEXPORT jboolean JNICALL Java_us_ihmc_aci_disService_DisseminationService_removeFilter (JNIEnv *pEnv, jobject joThis, jstring jsGroupName, jshort jsTag)
{
    // Not yet implemented
    checkAndLogMsg ("DisServiceJNIWrapper::removeFilter", Logger::L_MildError,
                    "not yet implemented\n");
    return (jboolean) false;
}

JNIEXPORT jboolean JNICALL Java_us_ihmc_aci_disService_DisseminationService_requestMoreChunks__Ljava_lang_String_2Ljava_lang_String_2I (JNIEnv *pEnv, jobject joThis, jstring jsGroupName, jstring jsSenderNodeId, jint jiSeqId)
{
    DisseminationService *pDisService = checkAndRetrieveDisServicePtr (pEnv, joThis);
    if (pDisService == NULL) {
        return false;
    }

    // Get the Group Name
    const char *pszGroupName = pEnv->GetStringUTFChars (jsGroupName, NULL);
    if (pszGroupName == NULL) {
        checkAndLogMsg ("DisServiceJNIWrapper::requestMoreChunks1", Logger::L_MildError,
                        "group name is null\n");
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "groupName is null");
        return false;
    }
    String groupName = pszGroupName;
    pEnv->ReleaseStringUTFChars (jsGroupName, pszGroupName);

    // Get the Sender Node Id
    const char *pszSenderNodeId = pEnv->GetStringUTFChars (jsSenderNodeId, NULL);
    if (pszSenderNodeId == NULL) {
        checkAndLogMsg ("DisServiceJNIWrapper::requestMoreChunks1", Logger::L_MildError,
                        "sender node id is null\n");
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "senderNodeId is null");
        return false;
    }
    String senderNodeId = pszSenderNodeId;
    pEnv->ReleaseStringUTFChars (jsSenderNodeId, pszSenderNodeId);

    int rc = pDisService->requestMoreChunks (0, groupName, senderNodeId, jiSeqId);

    return (jboolean) (rc == 0);
}

JNIEXPORT jboolean JNICALL Java_us_ihmc_aci_disService_DisseminationService_requestMoreChunks__Ljava_lang_String_2 (JNIEnv *pEnv, jobject joThis, jstring jsMessageId)
{
    DisseminationService *pDisService = checkAndRetrieveDisServicePtr (pEnv, joThis);
    if (pDisService == NULL) {
        return false;
    }

    // Get the MessageId
    const char *pszMessageId = pEnv->GetStringUTFChars (jsMessageId, NULL);
    if (pszMessageId == NULL) {
        checkAndLogMsg ("DisServiceJNIWrapper::requestMoreChunks2", Logger::L_MildError,
                        "message id is null\n");
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "messageId is null");
        return false;
    }
    String messageId = pszMessageId;
    pEnv->ReleaseStringUTFChars (jsMessageId, pszMessageId);

    int rc = pDisService->requestMoreChunks (0, messageId);

    return (jboolean) (rc == 0);
}

JNIEXPORT jbyteArray JNICALL Java_us_ihmc_aci_disService_DisseminationService_retrieve__Ljava_lang_String_2I (JNIEnv *pEnv, jobject joThis, jstring jsId, jint jiTimeout)
{
    DisseminationService *pDisService = checkAndRetrieveDisServicePtr (pEnv, joThis);
    if (pDisService == NULL) {
        return NULL;
    }

    // Get the Id
    const char *pszId = pEnv->GetStringUTFChars (jsId, NULL);
    if (pszId == NULL) {
        checkAndLogMsg ("DisServiceJNIWrapper::retrieve1", Logger::L_MildError,
                        "id is null\n");
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "id is null");
        return NULL;
    }
    String id = pszId;
    pEnv->ReleaseStringUTFChars (jsId, pszId);

    void *pBuf = NULL;
    uint32 ui32BufLen = 0;
    int rc = pDisService->retrieve (id, &pBuf, &ui32BufLen, (int64) jiTimeout);

    if (rc < 0) {
        checkAndLogMsg ("DisServiceJNIWrapper::retrieve1", Logger::L_MildError,
                        "retrieve failed for object <%s> with rc = %d\n", (const char *) id, rc);
        return NULL;
    }
    else if (rc == 0) {
        checkAndLogMsg ("DisServiceJNIWrapper::retrieve1", Logger::L_LowDetailDebug,
                        "did not find object <%s> locally cached\n", (const char *) id);
        return NULL;
    }
    if (pBuf == NULL) {
        checkAndLogMsg ("DisServiceJNIWrapper::retrieve1", Logger::L_MildError,
                        "retrieve failed for object <%s> - buffer is NULL\n", (const char *) id);
        return NULL;
    }

    jbyteArray jbaData = JNIUtils::createByteArrayFromBuffer (pEnv, pBuf, ui32BufLen);

    free (pBuf);

    return jbaData;
}

JNIEXPORT jint JNICALL Java_us_ihmc_aci_disService_DisseminationService_retrieve__Ljava_lang_String_2Ljava_lang_String_2 (JNIEnv *pEnv, jobject joThis, jstring jsId, jstring jsFilePath)
{
    // Not yet implemented
    checkAndLogMsg ("DisServiceJNIWrapper::retreve2", Logger::L_MildError,
                    "not yet implemented\n");
    return -1;
}

JNIEXPORT jboolean JNICALL Java_us_ihmc_aci_disService_DisseminationService_historyRequest (JNIEnv *pEnv, jobject joThis, jstring jsGroupName, jshort jsTag, jshort jsHistoryLength, jlong jlTimeout)
{
    DisseminationService *pDisService = checkAndRetrieveDisServicePtr (pEnv, joThis);
    if (pDisService == NULL) {
        return false;
    }

    // Get the Group Name
    const char *pszGroupName = pEnv->GetStringUTFChars (jsGroupName, NULL);
    if (pszGroupName == NULL) {
        checkAndLogMsg ("DisServiceJNIWrapper::historyRequest", Logger::L_MildError,
                        "group name is null\n");
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "groupName is null");
        return false;
    }
    String groupName = pszGroupName;
    pEnv->ReleaseStringUTFChars (jsGroupName, pszGroupName);

    int rc = pDisService->historyRequest (0, groupName, jsTag, jsHistoryLength, jlTimeout);

    return (jboolean) (rc == 0);
}

JNIEXPORT jboolean JNICALL Java_us_ihmc_aci_disService_DisseminationService_unsubscribe__Ljava_lang_String_2 (JNIEnv *pEnv, jobject joThis, jstring jsGroupName)
{
    DisseminationService *pDisService = checkAndRetrieveDisServicePtr (pEnv, joThis);
    if (pDisService == NULL) {
        return false;
    }
    const char *pszGroupName = pEnv->GetStringUTFChars (jsGroupName, NULL);
    if (pszGroupName == NULL) {
        checkAndLogMsg ("DisServiceJNIWrapper::unsubscribe", Logger::L_MildError,
                        "group name is null\n");
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "groupName is null");
        return false;
    }
    String groupName = pszGroupName;
    pEnv->ReleaseStringUTFChars (jsGroupName, pszGroupName);
    int rc;
    uint16 u16ClientId = 0;
    rc = pDisService->unsubscribe (u16ClientId, groupName);
    return (jboolean) (rc == 0);
}

JNIEXPORT jboolean JNICALL Java_us_ihmc_aci_disService_DisseminationService_unsubscribe__Ljava_lang_String_2S (JNIEnv *pEnv, jobject joThis, jstring jsGroupName, jshort jsTag)
{
    DisseminationService *pDisService = checkAndRetrieveDisServicePtr (pEnv, joThis);
    if (pDisService == NULL) {
        return false;
    }
    const char *pszGroupName = pEnv->GetStringUTFChars (jsGroupName, NULL);
    if (pszGroupName == NULL) {
        checkAndLogMsg ("DisServiceJNIWrapper::unsubscribe2", Logger::L_MildError,
                        "group name is null\n");
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "groupName is null");
        return false;
    }
    String groupName = pszGroupName;
    pEnv->ReleaseStringUTFChars (jsGroupName, pszGroupName);
    int rc;
    uint16 u16ClientId = 0;
    rc = pDisService->unsubscribe (u16ClientId, groupName, (uint16)jsTag);
    return (jboolean) (rc == 0);
}

JNIEXPORT jboolean JNICALL Java_us_ihmc_aci_disService_DisseminationService_subscribe__Ljava_lang_String_2BZZZ (JNIEnv *pEnv, jobject joThis, jstring jsGroupName, jbyte jbPriority, jboolean jbGroupReliable, jboolean jbMsgReliable, jboolean jbSequenced)
{
    DisseminationService *pDisService = checkAndRetrieveDisServicePtr (pEnv, joThis);
    if (pDisService == NULL) {
        return false;
    }

    // Get the Group Name
    const char *pszGroupName = pEnv->GetStringUTFChars (jsGroupName, NULL);
    if (pszGroupName == NULL) {
        checkAndLogMsg ("DisServiceJNIWrapper::subscribe1", Logger::L_MildError,
                        "group name is null\n");
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "groupName is null");
        return false;
    }
    String groupName = pszGroupName;
    pEnv->ReleaseStringUTFChars (jsGroupName, pszGroupName);

    int rc = pDisService->subscribe (0, groupName, (uint8) jbPriority, jbGroupReliable != 0, jbMsgReliable != 0, jbSequenced != 0);

    return (jboolean) (rc == 0);
}

JNIEXPORT jboolean JNICALL Java_us_ihmc_aci_disService_DisseminationService_subscribe__Ljava_lang_String_2SBZZZ (JNIEnv *pEnv, jobject joThis, jstring jsGroupName, jshort jsTag, jbyte jbPriority, jboolean jbGroupReliable, jboolean jbMsgReliable, jboolean jbSequenced)
{
    DisseminationService *pDisService = checkAndRetrieveDisServicePtr (pEnv, joThis);
    if (pDisService == NULL) {
        return false;
    }

    // Get the Group Name
    const char *pszGroupName = pEnv->GetStringUTFChars (jsGroupName, NULL);
    if (pszGroupName == NULL) {
        checkAndLogMsg ("DisServiceJNIWrapper::subscribe2", Logger::L_MildError,
                        "group name is null\n");
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "groupName is null");
        return false;
    }
    String groupName = pszGroupName;
    pEnv->ReleaseStringUTFChars (jsGroupName, pszGroupName);

    int rc = pDisService->subscribe (0, groupName, (uint16) jsTag, (uint8) jbPriority, jbGroupReliable != 0, jbMsgReliable != 0, jbSequenced != 0);

    return (jboolean) (rc == 0);
}

JNIEXPORT jboolean JNICALL Java_us_ihmc_aci_disService_DisseminationService_subscribe__Ljava_lang_String_2BLjava_lang_String_2BZZZ (JNIEnv *pEnv, jobject joThis, jstring jsGroupName, jbyte jbPredicateType, jstring jsPredicate, jbyte jbPriority, jboolean jbGroupReliable, jboolean jbMsgReliable, jboolean jbSequenced)
{
    // Not yet implemented
    checkAndLogMsg ("DisServiceJNIWrapper::subscribe3", Logger::L_MildError,
                    "not yet implemented\n");
    return (jboolean) false;
}

JNIEXPORT void JNICALL Java_us_ihmc_aci_disService_DisseminationService_registerDisseminationServiceListener (JNIEnv *pEnv, jobject joThis, jobject joListener)
{
    #if !defined (ANDROID) //No support for NewWeakGlobalRef on Dalvik JVM (ANDROID)
        // Use weak global ref to allow the class to be unloaded
        if (joDisseminationServiceListenerGlobalRef != NULL) {
            pEnv->DeleteWeakGlobalRef (joDisseminationServiceListenerGlobalRef);
        }
        joDisseminationServiceListenerGlobalRef = pEnv->NewWeakGlobalRef (joListener);
    #else
        if (joDisseminationServiceListenerGlobalRef != NULL) {
            pEnv->DeleteGlobalRef (joDisseminationServiceListenerGlobalRef);
        }
        joDisseminationServiceListenerGlobalRef = pEnv->NewGlobalRef (joListener);
    #endif
    if (joDisseminationServiceListenerGlobalRef == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "could not create a weak global reference to the listener");
    }
}

JNIEXPORT void JNICALL Java_us_ihmc_aci_disService_DisseminationService_registerPeerStatusListener (JNIEnv *pEnv, jobject joThis, jobject joListener)
{
    #if !defined (ANDROID) //No support for NewWeakGlobalRef on Dalvik JVM (ANDROID)
        // Use weak global ref to allow the class to be unloaded
        if (joPeerStatusListenerGlobalRef != NULL) {
            pEnv->DeleteWeakGlobalRef (joPeerStatusListenerGlobalRef);
        }
        joPeerStatusListenerGlobalRef = pEnv->NewWeakGlobalRef (joListener);
    #else
        if (joPeerStatusListenerGlobalRef != NULL) {
            pEnv->DeleteGlobalRef (joPeerStatusListenerGlobalRef);
        }
        joPeerStatusListenerGlobalRef = pEnv->NewGlobalRef (joListener);
    #endif
    if (joPeerStatusListenerGlobalRef == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "could not create a weak global reference to the listener");
    }
}

JNIEXPORT void JNICALL Java_us_ihmc_aci_disService_DisseminationService_resetTransmissionHistory (JNIEnv *pEnv, jobject joThis)
{
    DisseminationService *pDisService = checkAndRetrieveDisServicePtr (pEnv, joThis);
    if (pDisService == NULL) {
        return;
    }

    pDisService->resetTransmissionHistory();    
}

JNIEXPORT jstring JNICALL Java_us_ihmc_aci_disService_DisseminationService_getNextPushId (JNIEnv *pEnv, jobject joThis, jstring jsGroupName)
{
    DisseminationService *pDisService = checkAndRetrieveDisServicePtr (pEnv, joThis);
    if (pDisService == NULL) {
        return NULL;
    }
    const char *pszGroupName = pEnv->GetStringUTFChars (jsGroupName, NULL);
    if (pszGroupName == NULL) {
        checkAndLogMsg ("DisServiceJNIWrapper::getNextPushId", Logger::L_MildError,
                        "group name is null\n");
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "groupName is null");
        return NULL;
    }
    String groupName = pszGroupName;
    pEnv->ReleaseStringUTFChars (jsGroupName, pszGroupName);
    int rc;
    char szIdBuf[1024];
    if (0 != (rc = pDisService->getNextPushId (groupName, szIdBuf, sizeof (szIdBuf)))) {
        checkAndLogMsg ("DisService::getNextPushId", Logger::L_MildError,
                        "failed to get the next push id; rc = %d\n", rc);
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/RuntimeException"), "could not get next push id");
        return NULL;
    }
    return pEnv->NewStringUTF (szIdBuf);
}

bool DisServiceJNIWrapperListener::dataArrived (uint16 ui16ClientId, const char *pszSender, const char *pszGroupName,
                                                uint32 ui32SeqId, const char *pszObjectId, const char *pszInstanceId,
                                                const char *pszAnnotatedObjId, const char *pszMimeType, const void *pData,
                                                uint32 ui32Length, uint32 ui32MetadataLength, uint16 ui16Tag, uint8 ui8Priority,
                                                const char *pszQueryId)
{
    checkAndLogMsg ("DisServiceJNIWrapperListener::dataArrived", Logger::L_Info,
                    "entered dataArrived\n");

    if ((pszSender == NULL) || (pszGroupName == NULL)) {
        checkAndLogMsg ("DisServiceJNIWrapperListener::dataArrived", Logger::L_MildError,
                        "sender and/or groupName is/are NULL\n");
        return false;
    }

    JNIEnv *pEnv = JNU_GetEnv();
    jint jiRes;
    #ifdef JNI_VERSION_1_2
        #if !defined (ANDROID)
            jiRes = pCachedJVM->AttachCurrentThread ((void**) &pEnv, NULL);
        #else
            jiRes = pCachedJVM->AttachCurrentThread (&pEnv, NULL);
        #endif
    #else
        jiRes = pCachedJVM->AttachCurrentThread (&pEnv, NULL);
    #endif

    if (jiRes < 0) {
        checkAndLogMsg ("DisServiceJNIWrapperListener::dataArrived", Logger::L_MildError,
                        "JVM attach current thread failed\n");
        pCachedJVM->DetachCurrentThread();
        return false;
    }
    jobject joListener = pEnv->NewLocalRef (joDisseminationServiceListenerGlobalRef);
    if (joListener == NULL) {
        checkAndLogMsg ("DisServiceJNIWrapperListener::dataArrived", Logger::L_MediumDetailDebug,
                        "cannot invoke dataArrived callback because no listener is registered\n");
        pCachedJVM->DetachCurrentThread();
        return false;
    }
    jclass jcDisseminationServiceListener = (jclass) pEnv->NewLocalRef (jcDisseminationServiceListenerGlobalRef);
    if (jcDisseminationServiceListener == NULL) {
        checkAndLogMsg ("DisServiceJNIWrapperListener::dataArrived", Logger::L_MildError,
                        "cannot invoke dataArrived callback because the reference to the DisseminationServiceListener class is NULL\n");
        pEnv->DeleteLocalRef (joListener);
        pCachedJVM->DetachCurrentThread();
        return false;
    }
    // Get access to the dataArrived method in the DisseminationStatusListener interface
    jmethodID jmDataArrived = pEnv->GetMethodID (jcDisseminationServiceListener, "dataArrived",
                                                 "(Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;[BISBLjava/lang/String;)Z");
    if (jmDataArrived == NULL) {
        checkAndLogMsg ("DisServiceJNIWrapperListener::dataArrived", Logger::L_MildError,
                        "could not find dataArrived method\n");
        pEnv->DeleteLocalRef (joListener);
        pEnv->DeleteLocalRef (jcDisseminationServiceListener);
        pCachedJVM->DetachCurrentThread();
        return false;
    }
    jstring jsSender = pEnv->NewStringUTF (pszSender);
    jstring jsGroupName = pEnv->NewStringUTF (pszGroupName);
    jbyteArray jbaData = JNIUtils::createByteArrayFromBuffer (pEnv, pData, ui32Length);
    if (jbaData == NULL) {
        checkAndLogMsg ("DisServiceJNIWrapperListener::dataArrived", Logger::L_MildError,
                        "could not create byte array for data\n");
        pEnv->DeleteLocalRef (joListener);
        pEnv->DeleteLocalRef (jcDisseminationServiceListener);
        pCachedJVM->DetachCurrentThread();
        return false;
    }
    jstring jsObjectId = pEnv->NewStringUTF (pszObjectId);
    jstring jsInstanceId = pEnv->NewStringUTF (pszInstanceId);
    jstring jsMimeType = pEnv->NewStringUTF (pszMimeType);
    jstring jsQueryId = pEnv->NewStringUTF (pszQueryId);

    jboolean jbResult = pEnv->CallBooleanMethod (joListener, jmDataArrived, jsSender, jsGroupName, (jint) ui32SeqId, jsObjectId,
                                                 jsInstanceId, jsMimeType, jbaData, (jint) ui32MetadataLength,
                                                 (jshort) ui16Tag, (jbyte) ui8Priority, jsQueryId);

    checkAndLogMsg ("DisServiceLNIWrapperListener::dataArrived", Logger::L_Info,
                    "invoked dataArrived\n");

    pEnv->DeleteLocalRef (joListener);
    pEnv->DeleteLocalRef (jcDisseminationServiceListener);
    pCachedJVM->DetachCurrentThread();

    return (jbResult ? true : false);
}

bool DisServiceJNIWrapperListener::chunkArrived (uint16 ui16ClientId, const char *pszSender, const char *pszGroupName,
                                                 uint32 ui32SeqId, const char *pszObjectId, const char *pszInstanceId,
                                                 const char *pszMimeType, const void *pChunk, uint32 ui32Length,
                                                 uint8 ui8NChunks, uint8 ui8TotNChunks, const char *pszChunkedMsgId,
                                                 uint16 ui16Tag, uint8 ui8Priority, const char *pszQueryId)
{
    checkAndLogMsg ("DisServiceJNIWrapperListener::chunkArrived", Logger::L_Info,
                    "entered chunkArrived\n");

    if ((pszSender == NULL) || (pszGroupName == NULL) || (pszChunkedMsgId == NULL)) {
        checkAndLogMsg ("DisServiceJNIWrapperListener::chunkArrived", Logger::L_MildError,
                        "sender, groupName or chunkedMsgId is/are NULL\n");
        return false;
    }

    JNIEnv *pEnv = JNU_GetEnv();
    jint jiRes;
    #ifdef JNI_VERSION_1_2
        #if !defined (ANDROID)
            jiRes = pCachedJVM->AttachCurrentThread ((void**) &pEnv, NULL);
        #else
            jiRes = pCachedJVM->AttachCurrentThread (&pEnv, NULL);
        #endif
    #else
        jiRes = pCachedJVM->AttachCurrentThread (&pEnv, NULL);
    #endif

    if (jiRes < 0) {
        checkAndLogMsg ("DisServiceJNIWrapperListener::chunkArrived", Logger::L_MildError,
                        "JVM attach current thread failed\n");
        pCachedJVM->DetachCurrentThread();
        return false;
    }
    jobject joListener = pEnv->NewLocalRef (joDisseminationServiceListenerGlobalRef);
    if (joListener == NULL) {
        checkAndLogMsg ("DisServiceJNIWrapperListener::chunkArrived", Logger::L_MediumDetailDebug,
                        "cannot invoke chunkArrived callback because no listener is registered\n");
        pCachedJVM->DetachCurrentThread();
        return false;
    }
    jclass jcDisseminationServiceListener = (jclass) pEnv->NewLocalRef (jcDisseminationServiceListenerGlobalRef);
    if (jcDisseminationServiceListener == NULL) {
        checkAndLogMsg ("DisServiceJNIWrapperListener::chunkArrived", Logger::L_MildError,
                        "cannot invoke chunkArrived callback because the reference to the DisseminationServiceListener class is NULL\n");
        pEnv->DeleteLocalRef (joListener);
        pCachedJVM->DetachCurrentThread();
        return false;
    }
    // Get access to the chunkArrived method in the DisseminationStatusListener interface
    jmethodID jmChunkArrived = pEnv->GetMethodID (jcDisseminationServiceListener, "chunkArrived",
                                                  "(Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;[BSSLjava/lang/String;SBLjava/lang/String;)Z");
    if (jmChunkArrived == NULL) {
        checkAndLogMsg ("DisServiceJNIWrapperListener::chunkArrived", Logger::L_MildError,
                        "could not find chunkArrived method\n");
        pEnv->DeleteLocalRef (joListener);
        pEnv->DeleteLocalRef (jcDisseminationServiceListener);
        pCachedJVM->DetachCurrentThread();
        return false;
    }
    jstring jsSender = pEnv->NewStringUTF (pszSender);
    jstring jsGroupName = pEnv->NewStringUTF (pszGroupName);
    jbyteArray jbaChunk = JNIUtils::createByteArrayFromBuffer (pEnv, pChunk, ui32Length);
    if (jbaChunk == NULL) {
        checkAndLogMsg ("DisServiceJNIWrapperListener::chunkArrived", Logger::L_MildError,
                        "could not create byte array for data\n");
        pEnv->DeleteLocalRef (joListener);
        pEnv->DeleteLocalRef (jcDisseminationServiceListener);
        pCachedJVM->DetachCurrentThread();
        return false;
    }
    jstring jsChunkedMsgId = pEnv->NewStringUTF (pszChunkedMsgId);
    jstring jsObjectId = pEnv->NewStringUTF (pszObjectId);
    jstring jsInstanceId = pEnv->NewStringUTF (pszInstanceId);
    jstring jsMimeType = pEnv->NewStringUTF (pszMimeType);
    jstring jsQueryId = pEnv->NewStringUTF (pszQueryId);

    jboolean jbResult = pEnv->CallBooleanMethod (joListener, jmChunkArrived, jsSender, jsGroupName, (jint) ui32SeqId, jsObjectId,
                                                 jsInstanceId, jsMimeType, jbaChunk, (jshort) ui8NChunks, (jshort) ui8TotNChunks,
                                                 jsChunkedMsgId, (jshort) ui16Tag, (jbyte) ui8Priority, jsQueryId);

    checkAndLogMsg ("DisServiceLNIWrapperListener::chunkArrived", Logger::L_Info,
                    "invoked chunkArrived\n");

    pEnv->DeleteLocalRef (joListener);
    pEnv->DeleteLocalRef (jcDisseminationServiceListener);
    pCachedJVM->DetachCurrentThread();

    return (jbResult ? true : false);
}

bool DisServiceJNIWrapperListener::metadataArrived (uint16 ui16ClientId, const char *pszSender, const char *pszGroupName,
                                                    uint32 ui32SeqId, const char *pszObjectId, const char *pszInstanceId,
                                                    const char *pszMimeType, const void *pMetadata, uint32 ui32MetadataLength,
                                                    bool bDataChunked, uint16 ui16Tag, uint8 ui8Priority, const char *pszQueryId)
{
    checkAndLogMsg ("DisServiceJNIWrapperListener::metadataArrived", Logger::L_Info,
                    "entered metadataArrived\n");

    if ((pszSender == NULL) || (pszGroupName == NULL)) {
        checkAndLogMsg ("DisServiceJNIWrapperListener::metadataArrived", Logger::L_MildError,
                        "sender and/or groupName is/are NULL\n");
        return false;
    }

    JNIEnv *pEnv = JNU_GetEnv();
    jint jiRes;
    #ifdef JNI_VERSION_1_2
        #if !defined (ANDROID)
            jiRes = pCachedJVM->AttachCurrentThread ((void**) &pEnv, NULL);
        #else
            jiRes = pCachedJVM->AttachCurrentThread (&pEnv, NULL);
        #endif
    #else
        jiRes = pCachedJVM->AttachCurrentThread (&pEnv, NULL);
    #endif

    if (jiRes < 0) {
        checkAndLogMsg ("DisServiceJNIWrapperListener::metadataArrived", Logger::L_MildError,
                        "JVM attach current thread failed\n");
        pCachedJVM->DetachCurrentThread();
        return false;
    }
    jobject joListener = pEnv->NewLocalRef (joDisseminationServiceListenerGlobalRef);
    if (joListener == NULL) {
        checkAndLogMsg ("DisServiceJNIWrapperListener::metadataArrived", Logger::L_MediumDetailDebug,
                        "cannot invoke metadataArrived callback because no listener is registered\n");
        pCachedJVM->DetachCurrentThread();
        return false;
    }
    jclass jcDisseminationServiceListener = (jclass) pEnv->NewLocalRef (jcDisseminationServiceListenerGlobalRef);
    if (jcDisseminationServiceListener == NULL) {
        checkAndLogMsg ("DisServiceJNIWrapperListener::metadataArrived", Logger::L_MildError,
                        "cannot invoke metadataArrived callback because the reference to the DisseminationServiceListener class is NULL\n");
        pEnv->DeleteLocalRef (joListener);
        pCachedJVM->DetachCurrentThread();
        return false;
    }
    // Get access to the metadataArrived method in the DisseminationStatusListener interface
    jmethodID jmMetadataArrived = pEnv->GetMethodID (jcDisseminationServiceListener, "metadataArrived",
                                                     "(Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;[BZSBLjava/lang/String;)Z");
    if (jmMetadataArrived == NULL) {
        checkAndLogMsg ("DisServiceJNIWrapperListener::metadataArrived", Logger::L_MildError,
                        "could not find metadataArrived method\n");
        pEnv->DeleteLocalRef (joListener);
        pEnv->DeleteLocalRef (jcDisseminationServiceListener);
        pCachedJVM->DetachCurrentThread();
        return false;
    }
    jstring jsSender = pEnv->NewStringUTF (pszSender);
    jstring jsGroupName = pEnv->NewStringUTF (pszGroupName);
    jbyteArray jbaMetadata = JNIUtils::createByteArrayFromBuffer (pEnv, pMetadata, ui32MetadataLength);
    if (jbaMetadata == NULL) {
        checkAndLogMsg ("DisServiceJNIWrapperListener::metadataArrived", Logger::L_MildError,
                        "could not create byte array for metadata\n");
        pEnv->DeleteLocalRef (joListener);
        pEnv->DeleteLocalRef (jcDisseminationServiceListener);
        pCachedJVM->DetachCurrentThread();
        return false;
    }
    jstring jsObjectId = pEnv->NewStringUTF (pszObjectId);
    jstring jsInstanceId = pEnv->NewStringUTF (pszInstanceId);
    jstring jsMimeType = pEnv->NewStringUTF (pszMimeType);
    jstring jsQueryId = pEnv->NewStringUTF (pszQueryId);

    jboolean jbResult = pEnv->CallBooleanMethod (joListener, jmMetadataArrived, jsSender, jsGroupName, (jint) ui32SeqId, jsObjectId,
                                                 jsInstanceId, jsMimeType, jbaMetadata, (jboolean) bDataChunked,
                                                 (jshort) ui16Tag, (jbyte) ui8Priority, jsQueryId);

    checkAndLogMsg ("DisServiceLNIWrapperListener::metadataArrived", Logger::L_Info,
                    "invoked metadataArrived\n");

    pEnv->DeleteLocalRef (joListener);
    pEnv->DeleteLocalRef (jcDisseminationServiceListener);
    pCachedJVM->DetachCurrentThread();
    return (jbResult ? true : false);
}

bool DisServiceJNIWrapperListener::dataAvailable (uint16 ui16ClientId, const char *pszSender, const char *pszGroupName,
                                                  uint32 ui32SeqId, const char *pszObjectId, const char *pszInstanceId,
                                                  const char *pszMimeType, const char *pszRefObjId, const void *pMetadata,
                                                  uint32 ui32MetadataLength, uint16 ui16Tag, uint8 ui8Priority,
                                                  const char *pszQueryId)
{
    checkAndLogMsg ("DisServiceJNIWrapperListener::dataAvailable", Logger::L_Info,
                    "entered dataAvailable\n");

    if ((pszSender == NULL) || (pszGroupName == NULL) || (pszRefObjId == NULL)) {
        checkAndLogMsg ("DisServiceJNIWrapperListener::dataAvailable", Logger::L_MildError,
                        "sender, groupName, and/or refObjId is/are NULL\n");
        return false;
    }

    JNIEnv *pEnv = JNU_GetEnv();
    jint jiRes;
    #ifdef JNI_VERSION_1_2
        #if !defined (ANDROID)
            jiRes = pCachedJVM->AttachCurrentThread ((void**) &pEnv, NULL);
        #else
            jiRes = pCachedJVM->AttachCurrentThread (&pEnv, NULL);
        #endif
    #else
        jiRes = pCachedJVM->AttachCurrentThread (&pEnv, NULL);
    #endif

    if (jiRes < 0) {
        checkAndLogMsg ("DisServiceJNIWrapperListener::dataAvailable", Logger::L_MildError,
                        "JVM attach current thread failed\n");
        pCachedJVM->DetachCurrentThread();
        return false;
    }
    jobject joListener = pEnv->NewLocalRef (joDisseminationServiceListenerGlobalRef);
    if (joListener == NULL) {
        checkAndLogMsg ("DisServiceJNIWrapperListener::dataAvailable", Logger::L_MediumDetailDebug,
                        "cannot invoke dataAvailable callback because no listener is registered\n");
        pCachedJVM->DetachCurrentThread();
        return false;
    }
    jclass jcDisseminationServiceListener = (jclass) pEnv->NewLocalRef (jcDisseminationServiceListenerGlobalRef);
    if (jcDisseminationServiceListener == NULL) {
        checkAndLogMsg ("DisServiceJNIWrapperListener::dataAvailable", Logger::L_MildError,
                        "cannot invoke dataAvailable callback because the reference to the DisseminationServiceListener class is NULL\n");
        pEnv->DeleteLocalRef (joListener);
        pCachedJVM->DetachCurrentThread();
        return false;
    }
    // Get access to the dataArrived method in the DisseminationStatusListener interface
    jmethodID jmDataAvailable = pEnv->GetMethodID (jcDisseminationServiceListener, "dataAvailable",
                                                   "(Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;[BSBLjava/lang/String;)Z");
    if (jmDataAvailable == NULL) {
        checkAndLogMsg ("DisServiceJNIWrapperListener::dataAvailable", Logger::L_MildError,
                        "could not find dataAvailable method\n");
        pEnv->DeleteLocalRef (joListener);
        pEnv->DeleteLocalRef (jcDisseminationServiceListener);
        pCachedJVM->DetachCurrentThread();
        return false;
    }
    jstring jsSender = pEnv->NewStringUTF (pszSender);
    jstring jsGroupName = pEnv->NewStringUTF (pszGroupName);
    jbyteArray jbaMetadata = JNIUtils::createByteArrayFromBuffer (pEnv, pMetadata, ui32MetadataLength);
    if (jbaMetadata == NULL) {
        checkAndLogMsg ("DisServiceJNIWrapperListener::dataAvailable", Logger::L_MildError,
                        "could not create byte array for metadata\n");
        pEnv->DeleteLocalRef (joListener);
        pEnv->DeleteLocalRef (jcDisseminationServiceListener);
        pCachedJVM->DetachCurrentThread();
        return false;
    }
    jstring jsRefObjId = pEnv->NewStringUTF (pszRefObjId);
    jstring jsObjectId = pEnv->NewStringUTF (pszObjectId);
    jstring jsInstanceId = pEnv->NewStringUTF (pszInstanceId);
    jstring jsMimeType = pEnv->NewStringUTF (pszMimeType);
    jstring jsQueryId = pEnv->NewStringUTF (pszQueryId);

    jboolean jbResult = pEnv->CallBooleanMethod (joListener, jmDataAvailable, jsSender, jsGroupName, (jint) ui32SeqId, jsObjectId,
                                                 jsInstanceId, jsMimeType, jsRefObjId, jbaMetadata,
                                                 (jshort) ui16Tag, (jbyte) ui8Priority, jsQueryId);

    checkAndLogMsg ("DisServiceLNIWrapperListener::dataAvailable", Logger::L_Info,
                    "invoked dataAvailable\n");

    pEnv->DeleteLocalRef (joListener);
    pEnv->DeleteLocalRef (jcDisseminationServiceListener);
    pCachedJVM->DetachCurrentThread();

    return (jbResult ? true : false);
}

bool PeerStatusJNIWrapperListener::newPeer (const char *pszPeerNodeId)
{
    if (pszPeerNodeId == NULL) {
        checkAndLogMsg ("PeerStatusJNIWrapperListener::newPeer", Logger::L_MildError,
                        "peerNodeId is NULL\n");
        return false;
    }

    JNIEnv *pEnv = JNU_GetEnv();
    jint jiRes;
    #ifdef JNI_VERSION_1_2
        #if !defined (ANDROID)
            jiRes = pCachedJVM->AttachCurrentThread ((void**) &pEnv, NULL);
        #else
            jiRes = pCachedJVM->AttachCurrentThread (&pEnv, NULL);
        #endif
    #else
        jiRes = pCachedJVM->AttachCurrentThread (&pEnv, NULL);
    #endif

    if (jiRes < 0) {
        checkAndLogMsg ("PeerStatusJNIWrapperListener::newPeer", Logger::L_MildError,
                        "JVM attach current thread failed\n");
        pCachedJVM->DetachCurrentThread();
        return false;
    }

    jobject joListener = pEnv->NewLocalRef (joPeerStatusListenerGlobalRef);
    if (joListener == NULL) {
        checkAndLogMsg ("PeerStatusJNIWrapperListener::newPeer", Logger::L_MediumDetailDebug,
                        "cannot invoke newPeer callback because no listener is registered\n");
        pCachedJVM->DetachCurrentThread();
        return false;
    }
    jstring jsPeerNodeId = pEnv->NewStringUTF (pszPeerNodeId);

    jboolean jbResult = pEnv->CallBooleanMethod (joListener, jmNewPeerGlobal, jsPeerNodeId);

    pEnv->DeleteLocalRef (joListener);
    pCachedJVM->DetachCurrentThread();

    return (jbResult ? true : false);
}

bool PeerStatusJNIWrapperListener::deadPeer (const char *pszPeerNodeId)
{
    if (pszPeerNodeId == NULL) {
        checkAndLogMsg ("PeerStatusJNIWrapperListener::deadPeer", Logger::L_MildError,
                        "peerNodeId is NULL\n");
        return false;
    }

    JNIEnv *pEnv = JNU_GetEnv();
    jint jiRes;
    #ifdef JNI_VERSION_1_2
        #if !defined (ANDROID)
            jiRes = pCachedJVM->AttachCurrentThread ((void**) &pEnv, NULL);
        #else
            jiRes = pCachedJVM->AttachCurrentThread (&pEnv, NULL);
        #endif
    #else
        jiRes = pCachedJVM->AttachCurrentThread (&pEnv, NULL);
    #endif

    if (jiRes < 0) {
        checkAndLogMsg ("PeerStatusJNIWrapperListener::deadPeer", Logger::L_MildError,
                        "JVM attach current thread failed\n");
        pCachedJVM->DetachCurrentThread();
        return false;
    }

    jobject joListener = pEnv->NewLocalRef (joPeerStatusListenerGlobalRef);
    if (joListener == NULL) {
        checkAndLogMsg ("PeerStatusJNIWrapperListener::deadPeer", Logger::L_MediumDetailDebug,
                        "cannot invoke deadPeer callback because no listener is registered\n");
        pCachedJVM->DetachCurrentThread();
        return false;
    }
    jstring jsPeerNodeId = pEnv->NewStringUTF (pszPeerNodeId);

    jboolean jbResult = pEnv->CallBooleanMethod (joListener, jmDeadPeerGlobal, jsPeerNodeId);

    pEnv->DeleteLocalRef (joListener);
    pCachedJVM->DetachCurrentThread();

    return (jbResult ? true : false);
}
