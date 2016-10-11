/*
 * DSProJNIWrapper.cpp
 *
 * This file is part of the IHMC DSPro Library/Component
 * Copyright (c) 2008-2016 IHMC.
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
 *
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */

#include "DSProJNIWrapper.h"
#include "Instrumentator.h"
#include "Topology.h"
#include "DSProCmdProcessor.h"

#include "DSPro.h"
#include "DSLib.h"
#include "DSProProxyServer.h"
#include "StrClass.h"
#include "ConfigManager.h"
#include "FileUtils.h"
#include "NetUtils.h"
#include "UUID.h"

#include "Logger.h"

#include <time.h>

using namespace IHMC_ACI;
using namespace NOMADSUtil;

//#define checkAndLogMsg if (pLogger) pLogger->logMsg
#ifdef WIN32
#include <io.h>
    #define strdup _strdup
    #define access _access
#endif

static JavaVM *pCachedJVM;

JNIEnv * JNU_GetEnv()
{
    JNIEnv *pEnv;
    pCachedJVM->GetEnv ((void **)&pEnv, JNI_VERSION_1_2);
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


ConfigManager * checkAndRetrieveCfgMgrPtr (JNIEnv *pEnv, jobject joThis)
{
    jclass jcCfgMgr = pEnv->GetObjectClass (joThis);
    jfieldID jfCfgMgr = pEnv->GetFieldID (jcCfgMgr, "_configManager", "J");
    jlong jlCfgMgr = pEnv->GetLongField (joThis, jfCfgMgr);

    union {
        jlong jlValue;
        void *p;
    } u;
    u.jlValue = jlCfgMgr;
    ConfigManager *pCfgMgr = (ConfigManager *) u.p;
    if (pCfgMgr == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("us/ihmc/aci/dspro2/DSProLauncher/DSProInitException"), 
            "ConfigManager not initialized - ConfigManager is null");
        return NULL;
    }

    return pCfgMgr;
}

DSPro * checkAndRetrieveDSProPtr (JNIEnv *pEnv, jobject joThis)
{
    jclass jcDSPro = pEnv->GetObjectClass (joThis);
    jfieldID jfDSPro = pEnv->GetFieldID (jcDSPro, "_dspro", "J");
    jlong jlDSPro = pEnv->GetLongField (joThis, jfDSPro);

    union {
        jlong jlValue;
        void *p;
    } u;
    u.jlValue = jlDSPro;
    DSPro *pDSPro = (DSPro *) u.p;
    if (pDSPro == NULL) {
        //pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "DSProJNIWrapper not initialized - DSPro is null");
        pEnv->ThrowNew (pEnv->FindClass ("us/ihmc/aci/dspro2/DSProLauncher/DSProInitException"), 
            "DSProJNIWrapper not initialized - DSPro is null");
        return NULL;
    }

    return pDSPro;
}

void initializeLogger (Logger **pLogger, const char *pszLogDir, const char *pszLogFileName,
	const char *pszTimestamp, bool bEnableScreenOutput)
{
	if ((pLogger == NULL) || (pszLogFileName == NULL) || (pszTimestamp == NULL)) {
		return;
	}
	if ((*pLogger) == NULL) {
		(*pLogger) = new Logger();
		(*pLogger)->setDebugLevel(Logger::L_LowDetailDebug);

		// Screen output
		if (bEnableScreenOutput) {
			(*pLogger)->enableScreenOutput();
		}

		// File output
		String filename((pszLogDir == NULL) ? "" : pszLogDir);
		if (filename.length() > 0) {
			filename += getPathSepCharAsString();
		}
		filename += pszTimestamp;
		filename += "-";
		filename += pszLogFileName;
		(*pLogger)->initLogFile(filename.c_str(), false);
		(*pLogger)->enableFileOutput();
		printf("Enabled logging on file %s\n", filename.c_str());

		// Network output
		(*pLogger)->initNetworkLogging("127.0.0.1");
		(*pLogger)->enableNetworkOutput(Logger::L_LowDetailDebug);
	}
}

JNIEXPORT jint JNICALL Java_us_ihmc_aci_dspro2_DSProLauncher_initLoggers (JNIEnv *pEnv, jobject joThis,
	jstring jsLogDir)
{
	const char *pszLogDir;
	pszLogDir = pEnv->GetStringUTFChars (jsLogDir, NULL);
	if (pszLogDir == NULL) {
		pEnv->ThrowNew (pEnv->FindClass("us/ihmc/aci/dspro2/DSProLauncher/DSProInitException"),
			"FAIL while copying string logDir");
		pEnv->ReleaseStringUTFChars (jsLogDir, pszLogDir);
		return -11;
	}

	if (!FileUtils::directoryExists (pszLogDir)) {
		if (!FileUtils::createDirectory (pszLogDir)) {
			return -12;
		}
	}
	time_t now = time(NULL);
	struct tm *ptm = localtime(&now);
	static char timestamp[17];
	sprintf (timestamp, "%d%02d%02d-%02d_%02d_%02d",
		(ptm->tm_year + 1900), (ptm->tm_mon + 1), ptm->tm_mday,
		ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

	initializeLogger (&pLogger, pszLogDir, "dspro.log", timestamp, false);
	initializeLogger (&pNetLog, pszLogDir, "dspro-matchmaking.log", timestamp, false);
	initializeLogger (&pTopoLog, pszLogDir, "dspro-topology.log", timestamp, false);
	initializeLogger (&pCmdProcLog, pszLogDir, "dspro-notifications.log", timestamp, false);

	return 0;
}

JNIEXPORT jint JNICALL Java_us_ihmc_aci_dspro2_DSProLauncher_initConfigNative (JNIEnv *pEnv, jobject joThis, 
	jstring jsConfigFile)
{
	// Loading config file
	const char *pcConfigFile;
	pcConfigFile = pEnv->GetStringUTFChars (jsConfigFile, NULL);
	if (pcConfigFile == NULL) {
		pEnv->ThrowNew (pEnv->FindClass ("us/ihmc/aci/dspro2/DSProLauncher/DSProInitException"),
			"FAIL while copying string configFile");
		pEnv->ReleaseStringUTFChars (jsConfigFile, pcConfigFile);
		return -12;
	}

	ConfigManager *pCfgMgr = new ConfigManager();
	if (pCfgMgr == NULL) {
		pEnv->ThrowNew (pEnv->FindClass ("us/ihmc/aci/dspro2/DSProLauncher/DSProInitException"),
			"DSProJNIWrapper not initialized - ConfigManager is null");
		return -13;
	}

	jclass jcCfgMgr = pEnv->GetObjectClass (joThis);
	jfieldID jfCfgMgr = pEnv->GetFieldID (jcCfgMgr, "_configManager", "J");
	union {
		jlong jlValue;
		void *p;
	} u;
	u.p = pCfgMgr;
	pEnv->SetLongField (joThis, jfCfgMgr, u.jlValue);

	pCfgMgr->init();

	String cfgFilePath = pcConfigFile;
	if (0 == access (cfgFilePath, 0)) {
		// There is a config file to be read
		int rc;
		if (0 != (rc = (pCfgMgr->readConfigFile(cfgFilePath, true)))) {

			if (pLogger != NULL) {
				pLogger->logMsg("main", Logger::L_SevereError,
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
			pLogger->logMsg("main", Logger::L_SevereError,
				"could not find config file %s\n", (const char *)cfgFilePath);
		}
		delete pCfgMgr;
		pCfgMgr = NULL;
		return -24;
	}

	return 0;
}

JNIEXPORT jstring JNICALL Java_us_ihmc_aci_dspro2_DSProLauncher_getNodeId (JNIEnv *pEnv, jobject joThis)
{
	ConfigManager *pCfgMgr = checkAndRetrieveCfgMgrPtr (pEnv, joThis);
	if (pCfgMgr == NULL) {
		return NULL;
	}

	String nodeId;
	if (pCfgMgr->hasValue ("aci.dspro.node.id")) {
		nodeId = pCfgMgr->getValue ("aci.dspro.node.id");
	}
	else if (pCfgMgr->hasValue ("aci.disService.nodeUUID")) {
		// for backward compatibility
		nodeId = pCfgMgr->getValue ("aci.disService.nodeUUID");
	}
	else if (pCfgMgr->hasValue ("aci.dspro.node.id.auto") &&
		(strcmp ("hostname", pCfgMgr->getValue ("aci.dspro.node.id.auto")) == 0)) {
		nodeId = NetUtils::getLocalHostName();
		pCfgMgr->setValue ("aci.dspro.node.id", nodeId);
	}
	else if (pCfgMgr->hasValue ("aci.disService.nodeUUID.auto.mode") &&
		(strcmp ("hostname", pCfgMgr->getValue ("aci.disService.nodeUUID.auto.mode")) == 0)) {
		// for backward compatibility
		nodeId = NetUtils::getLocalHostName();
		pCfgMgr->setValue ("aci.dspro.node.id", nodeId);
	}
	else {
		NOMADSUtil::UUID uuid;
		uuid.generate();
		const char *pszUUID = uuid.getAsString();
		if (pszUUID != NULL) {
			nodeId = pszUUID;
		}
	}

	if (nodeId == NULL) {
		return NULL;
	}

	return pEnv->NewStringUTF (nodeId.c_str());
}

JNIEXPORT void JNICALL Java_us_ihmc_aci_dspro2_DSProLauncher_constructor (JNIEnv *pEnv, jobject joThis,
	jstring jsNodeId, jstring jsVersion)
{
	jclass jcDSPro = pEnv->GetObjectClass (joThis);
	jfieldID jfDSPro = pEnv->GetFieldID (jcDSPro, "_dspro", "J");

	const char *pcNodeId;
	pcNodeId = pEnv->GetStringUTFChars (jsNodeId, NULL);
	if (pcNodeId == NULL) {
		//pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "FAIL while copying string node id");
		pEnv->ThrowNew (pEnv->FindClass ("us/ihmc/aci/dspro2/DSProLauncher/DSProInitException"),
			"FAIL while copying string node id");
		pEnv->ReleaseStringUTFChars (jsNodeId, pcNodeId);
		return;
	}

	const char *pcVersion;
	if (jsVersion != NULL) {
		pcVersion = pEnv->GetStringUTFChars (jsVersion, NULL);
		if (pcVersion == NULL) {
			//pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "FAIL while copying string version");
			pEnv->ThrowNew (pEnv->FindClass ("us/ihmc/aci/dspro2/DSProLauncher/DSProInitException"),
				"FAIL while copying string version");
			pEnv->ReleaseStringUTFChars (jsVersion, pcVersion);
			return;
		}
	}
	else {
		pcVersion = NULL;
	}

	DSPro *pDSPro = new DSPro (pcNodeId, pcVersion);
	if (pDSPro == NULL) {
		//pEnv->ThrowNew (pEnv->FindClass ("java/lang/NullPointerException"), "DSProJNIWrapper not initialized - DSPro is null");
		pEnv->ThrowNew (pEnv->FindClass ("us/ihmc/aci/dspro2/DSProLauncher/DSProInitException"),
			"DSProJNIWrapper not initialized - DSPro is null");
		return;
	}
	union {
		jlong jlValue;
		void *p;
	} u;
	u.p = pDSPro;

	pEnv->SetLongField (joThis, jfDSPro, u.jlValue);
}

JNIEXPORT jint JNICALL Java_us_ihmc_aci_dspro2_DSProLauncher_initDSProNative (JNIEnv *pEnv, jobject joThis,
    jstring jsMetadataExtraAttrFile, jstring jsMetadataValuesFile)
{
    DSPro *pDSPro = checkAndRetrieveDSProPtr (pEnv, joThis);
    if (pDSPro == NULL) {
        return -11;
    }

	ConfigManager *pCfgMgr = checkAndRetrieveCfgMgrPtr(pEnv, joThis);
	if (pCfgMgr == NULL) {
		return -21;
	}

    // Loading jsMetadataExtraAttrFile
    const char *pcMetadataExtraAttrFile;
    pcMetadataExtraAttrFile = pEnv->GetStringUTFChars (jsMetadataExtraAttrFile, NULL);
    if (pcMetadataExtraAttrFile == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("us/ihmc/aci/dspro2/DSProLauncher/DSProInitException"), 
            "FAIL while copying string metadata extra attr file");
        pEnv->ReleaseStringUTFChars (jsMetadataExtraAttrFile, pcMetadataExtraAttrFile);
        return -31;
    }

    String metadataExtraAttrFile = pcMetadataExtraAttrFile;
    char *pszMetadataExtraAttributes = NULL;
    if (0 == access (metadataExtraAttrFile, 0)) {
        // There is a metadata extra attr file to be read
        DSLib::readFileIntoString (metadataExtraAttrFile, &pszMetadataExtraAttributes);

        if (pLogger != NULL) {
            pLogger->logMsg ("main", Logger::L_LowDetailDebug,
                "reading metadata extra attr file: %s\n", (const char*)metadataExtraAttrFile);
        }
    }
    else {
        if (pLogger != NULL) {
            pLogger->logMsg ("main", Logger::L_SevereError,
                "could not find metadata extra attr file %s\n", (const char *)metadataExtraAttrFile);
        }
        delete pCfgMgr;
        pCfgMgr = NULL;
        return -32;
    }

    // TODO: load jsMetadataValuesFile
    const char *pcMetadataValuesFile;
    pcMetadataValuesFile = pEnv->GetStringUTFChars (jsMetadataValuesFile, NULL);
    if (pcMetadataValuesFile == NULL) {
        pEnv->ThrowNew (pEnv->FindClass ("us/ihmc/aci/dspro2/DSProLauncher/DSProInitException"),
            "FAIL while copying string metadata values file");
        pEnv->ReleaseStringUTFChars (jsMetadataValuesFile, pcMetadataValuesFile);
        return -41;
    }

    String metadataValuesFile = pcMetadataValuesFile;
    char *pszMetadataValues = NULL;
    if (0 == access (metadataValuesFile, 0)) {
        // There is a metadata values file to be read
        DSLib::readFileIntoString (metadataValuesFile, &pszMetadataValues);

        if (pLogger != NULL) {
            pLogger->logMsg ("main", Logger::L_LowDetailDebug,
                "reading metadata values file: %s\n", (const char*)metadataValuesFile);
        }
    }
    else {
        if (pLogger != NULL) {
            pLogger->logMsg ("main", Logger::L_SevereError,
                "could not find metadata values file %s\n", (const char *)metadataValuesFile);
        }
        delete pCfgMgr;
        pCfgMgr = NULL;
        return -42;
    }

    // Calling init
    return pDSPro->init (pCfgMgr, pszMetadataExtraAttributes, pszMetadataValues);
}

JNIEXPORT jint JNICALL Java_us_ihmc_aci_dspro2_DSProLauncher_startNative (JNIEnv *pEnv, jobject joThis)
{
    DSPro *pDSPro = checkAndRetrieveDSProPtr (pEnv, joThis);
    if (pDSPro == NULL) {
        return -1;
    }

    ConfigManager *pCfgMgr = checkAndRetrieveCfgMgrPtr (pEnv, joThis);
    if (pCfgMgr == NULL) {
        return -2;
    }

    DSProProxyServer proxySrv;
    const char *pszProxyServerInterface = NULL;
    uint16 ui16ProxyServerPort = 56487; // DISPRO_SVC_PROXY_SERVER_PORT_NUMBER;
    if (pCfgMgr->hasValue ("aci.disservice.proxy.interface")) {
        pszProxyServerInterface = pCfgMgr->getValue ("aci.disservice.proxy.interface");
    }
    if (pCfgMgr->hasValue ("aci.disservice.proxy.port")) {
        ui16ProxyServerPort = (uint16) pCfgMgr->getValueAsInt ("aci.disservice.proxy.port", 56487);
    }

    if (proxySrv.init (pDSPro, pszProxyServerInterface, ui16ProxyServerPort) != 0) {
        if (pLogger != NULL) {
            pLogger->logMsg ("main", Logger::L_SevereError,
                "DSProProxyServer has failed to initialize\n");
        }
        return -3;
    }
    proxySrv.start();

    return 0;
}
