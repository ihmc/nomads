/*
* NmsJNIWrapper.cpp
*
* This file is part of the IHMC NMS Library
* Copyright (c) IHMC. All Rights Reserved.
*
* Usage restricted to not-for-profit use only.
* Contact IHMC for other types of licenses.
*
* @author Rita Lenzi (rlenzi@ihmc.us)
*/

#include "NmsJNIWrapper.h"

#include "NetworkMessageService.h"
#include "StrClass.h"
#include "ConfigManager.h"

#include "Logger.h"

using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg
#ifdef WIN32
#include <io.h>
    #define strdup _strdup
    #define access _access
#endif

static JavaVM *pCachedJVM;

JNIEnv * JNU_GetEnv()
{
    JNIEnv *pEnv;
    pCachedJVM->GetEnv ((void **) &pEnv, JNI_VERSION_1_2);
    return pEnv;
}

JNIEXPORT jint JNICALL JNI_OnLoad (JavaVM *pJvm, void * pReserved)
{
    JNIEnv *pEnv;
    pCachedJVM = pJvm; // cache the JavaVM pointer
    if (pJvm->GetEnv ((void **)&pEnv, JNI_VERSION_1_2)) {
        return JNI_ERR; // JNI version not supported
    }

    return JNI_VERSION_1_2;
}

JNIEXPORT void JNICALL Java_us_ihmc_nms_NetworkMessageService_constructor (JNIEnv *pEnv, jobject joThis)
{
    jclass jcNetworkMessageService = pEnv->GetObjectClass (joThis);
    jfieldID jfNetworkMessageService = pEnv->GetFieldID (jcNetworkMessageService, "_networkMessageService", "J");
    NetworkMessageService *pNetworkMessageService = new NetworkMessageService();
    if (pNetworkMessageService == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "NmsJNIWrapper not initialized - NetworkMessageService is null");
        return;
    }
    union {
        jlong jlValue;
        void *p;
    } u;
    u.p = pNetworkMessageService;

    pEnv->SetLongField (joThis, jfNetworkMessageService, u.jlValue);
}

NetworkMessageService * checkAndRetrieveNmsPtr (JNIEnv *pEnv, jobject joThis)
{
    jclass jcNetworkMessageService = pEnv->GetObjectClass (joThis);
    jfieldID jfNetworkMessageService = pEnv->GetFieldID (jcNetworkMessageService, "_networkMessageService", "J");
    jlong jlNetworkMessageService = pEnv->GetLongField (joThis, jfNetworkMessageService);

    union {
        jlong jlValue;
        void *p;
    } u;
    u.jlValue = jlNetworkMessageService;
    NetworkMessageService *pNetworkMessageService = (NetworkMessageService *) u.p;
    if (pNetworkMessageService == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "NmsJNIWrapper not initialized - NetworkMessageService is null");
        return NULL;
    }

    return pNetworkMessageService;
}

JNIEXPORT jint JNICALL Java_us_ihmc_nms_NetworkMessageService_initNativeConfig (JNIEnv *pEnv, jobject joThis, jstring jsConfigFile)
{
    NetworkMessageService *pNetworkMessageService = checkAndRetrieveNmsPtr (pEnv, joThis);
    if (pNetworkMessageService == NULL) {
        return -11;
    }

    const char *pcConfigFile;
    pcConfigFile = pEnv->GetStringUTFChars(jsConfigFile, NULL);
    if (pcConfigFile == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "FAIL while copying string configFile");
        pEnv->ReleaseStringUTFChars (jsConfigFile, pcConfigFile);
        return -12;
    }

    ConfigManager *pCfgMgr = new ConfigManager();
    if (pCfgMgr == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "NmsJNIWrapper not initialized - ConfigManager is null");
        return -13;
    }

    pCfgMgr->init();
    String cfgFilePath = pcConfigFile;
    if (0 == access (cfgFilePath, 0)) {
        // There is a config file to be read
        int rc;
        if (0 != (rc = (pCfgMgr->readConfigFile (cfgFilePath, true)))) {

            if (pLogger != NULL) {
                pLogger->logMsg ("main", Logger::L_SevereError,
                    "failed to read the config file; rc=%d\n", rc);
            }
            delete pCfgMgr;
            pCfgMgr = NULL;
            return -23;
        }

        if (pLogger != NULL) {
            pLogger->logMsg ("main", Logger::L_LowDetailDebug,
                "reading config file: %s\n", (const char*)cfgFilePath);
        }

    }
    else {
        if (pLogger != NULL) {
            pLogger->logMsg ("main", Logger::L_SevereError,
                "could not find config file %s\n", (const char *)cfgFilePath);
        }
        delete pCfgMgr;
        pCfgMgr = NULL;
        return -24;
    }

    int rc = pNetworkMessageService->init (pCfgMgr);
    if (rc < 0) {
        pEnv->ThrowNew (pEnv->FindClass ("java/io/IOException"), "NmsJNIWrapper not initialized - failed to initialize the run init() - cannot continue");
    }

    pEnv->ReleaseStringUTFChars (jsConfigFile, pcConfigFile);

    return rc;
}

JNIEXPORT void JNICALL Java_us_ihmc_nms_NetworkMessageService_start (JNIEnv *pEnv, jobject joThis)
{
    NetworkMessageService *pNetworkMessageService = checkAndRetrieveNmsPtr (pEnv, joThis);
    if (pNetworkMessageService == NULL) {
        return;
    }

    pNetworkMessageService->start();
}
