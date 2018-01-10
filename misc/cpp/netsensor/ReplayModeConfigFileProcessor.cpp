/*
* ReplayModeConfigFileProcessor.cpp
* Author: amorelli@ihmc.us
* This file is part of the IHMC NetSensor Library/Component
* Copyright (c) 2010-2017 IHMC.
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
* NetSensor class for parsing REPLAY MODE configuration files in JSON format
*/

#include "ReplayModeConfigFileProcesser.h"
#include "Json.h"
#include "File.h"
#include "FileUtils.h"
#include "FileReader.h"

#include "NetSensor.h"


using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace IHMC_NETSENSOR
{
    LList<NetSensor::InterfaceInfoOpt> *
        ReplayModeConfigFileProcessor::getInterfaceInfoList(void)
    {
        if (!_bConfigFileProcessed) {
            if (processConfigFile() < 0) {
                return nullptr;
            }
        }

        return &_llIIO;
    }

    NOMADSUtil::LList<uint32> *
        ReplayModeConfigFileProcessor::getRemoteNpList(void)
    {
        if (!_bConfigFileProcessed) {
            if (processConfigFile() < 0) {
                return nullptr;
            }
        }

        return &_llRemoteNP;
    }

    NOMADSUtil::String ReplayModeConfigFileProcessor::getStatsRecipientIP(void)
    {
        if (!_bConfigFileProcessed) {
            if (processConfigFile() < 0) {
                return -1;
            }
        }

        return _sStatsRecipientIP;
    }

    NOMADSUtil::String ReplayModeConfigFileProcessor::getTDM(void)
    {
        if (!_bConfigFileProcessed) {
            if (processConfigFile() < 0) {
                return -1;
            }
        }

        return _sTDM;
    }

    NOMADSUtil::LList<NOMADSUtil::String> * ReplayModeConfigFileProcessor::getInterfaceNames (void)
    {
        if (!_bConfigFileProcessed) {
            if (processConfigFile() < 0) {
                return nullptr;
            }
        }

        return &_lInterfaceNames;
    }

    int ReplayModeConfigFileProcessor::processConfigFile(void)
    {
        if (_bError) {
            // Error encountered previously, avoid processing the config file again
            return -1;
        }

        if (!FileUtils::fileExists(_sConfigFile)) {
            return -2;
        }

        const uint16 ui16JsonSize = FileUtils::fileSize(_sConfigFile) + 1;
        String sJson(ui16JsonSize);
        int rc =  FileReader(_sConfigFile, "r").read(sJson, ui16JsonSize);
        if (rc < 0) {
            return -3;
        }
        sJson[rc] = 0;
        JsonObject jsonObj(sJson);

        if (jsonObj.hasObject("interfaces")) {
            const JsonArray * const jaInterfaces = jsonObj.getArray("interfaces");
            for (int i = 0; i < jaInterfaces->getSize(); ++i) {
                NetSensor::InterfaceInfoOpt iio =
                    NetSensor::InterfaceInfoOpt::fromJSON (jaInterfaces->getObject(i));
                if (iio == NetSensor::InterfaceInfoOpt()) {
                    _bError = true;
                    return -4;
                }
                if (_lInterfaceNames.search(iio.pcIname)) {
                    checkAndLogMsg("ReplayModeConfigFileProcessor::processConfigFile",
                                   Logger::L_Warning, "Another interface with name %s "
                                   "has already been acquired\n", iio.pcIname.c_str());
                    continue;
                }

                if (_sTDM.length() <= 0) {
                    _sTDM = iio.sTDM;
                }
                else if (!(_sTDM ^= iio.sTDM)) {
                    checkAndLogMsg("ReplayModeConfigFileProcessor::processConfigFile",
                                   Logger::L_MildError, "The values of TDM specified "
                                   "for different interfaces do not match\n");
                    _bError = true;
                    return -5;
                }

                if (consolidatePcapFile(iio.sPcapFilePath) < 0) {
                    checkAndLogMsg("ReplayModeConfigFileProcessor::processConfigFile",
                                   Logger::L_MildError, "Could not be found pcap file "
                                   "at path %s\n", iio.sPcapFilePath.c_str());
                    _bError = true;
                    return -6;
                }

                _lInterfaceNames.add(iio.pcIname);
                _llIIO.add(iio);
            }
        }

        if (jsonObj.hasObject("remote_netproxy_list")) {
            const JsonArray * const jaRemoteNP = jsonObj.getArray("remote_netproxy_list");
            for (int i = 0; i < jaRemoteNP->getSize(); ++i) {
                String sRemoteNP;
                if (jaRemoteNP->getString(i, sRemoteNP) < 0) {
                    _bError = true;
                    return -7;
                }
                if (!InetAddr::isIPv4Addr(sRemoteNP)) {
                    checkAndLogMsg("ReplayModeConfigFileProcessor::processConfigFile",
                                   Logger::L_MildError, "The string %s is not a valid "
                                   "IPv4 address\n", sRemoteNP.c_str());
                    _bError = true;
                    return -8;
                }

                _llRemoteNP.add(InetAddr(sRemoteNP).getIPAddress());
            }
        }

        if (jsonObj.hasObject("stats_recipient_ip")) {
            if (jsonObj.getString("stats_recipient_ip", _sStatsRecipientIP) < 0) {
                checkAndLogMsg("ReplayModeConfigFileProcessor::processConfigFile",
                               Logger::L_MildError, "Error processing the field "
                               "stats_recipient_ip from the config file\n");
                _bError = true;
                return -9;
            }
            if (!InetAddr::isIPv4Addr(_sStatsRecipientIP)) {
                checkAndLogMsg("ReplayModeConfigFileProcessor::processConfigFile",
                               Logger::L_MildError, "The string %s is not a valid "
                               "IPv4 address\n", _sStatsRecipientIP.c_str());
                _bError = true;
                return -10;
            }
        }

        _bConfigFileProcessed = true;
        return 0;
    }

    int ReplayModeConfigFileProcessor::consolidatePcapFile(NOMADSUtil::String & sPcapFilePath) const
    {
        if (sPcapFilePath.length() <= 0) {
            return -1;
        }

        // Case 1: path is absolute
        File fPcapFilePath (sPcapFilePath);
        if (fPcapFilePath.isPathAbsolute()) {
            if (fPcapFilePath.exists()) {
                return 0;
            }

            // No logical consolidation is possible when an absolute path is given
            return -2;
        }

        // Case 2: path is relative, use the directory of the config file as the home directory
        File fConfigFile(_sConfigFile);
        File fConsolidatedPcapFilePath(String(fConfigFile.getParent()) +
                                       getPathSepChar() + sPcapFilePath);
        if (fConsolidatedPcapFilePath.exists()) {
            sPcapFilePath = fConsolidatedPcapFilePath.getPath();
            return 0;
        }

        // Case 3: path is relative, use the working directory as the home directory
        if (fPcapFilePath.exists()) {
            return 0;
        }

        return -3;
    }

}