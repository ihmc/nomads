/*
 * Comparator.cpp
 *
 * This file is part of the IHMC Voi Library/Component
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
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on Febraury 15, 2017, 2:20 PM
 */

#include "Comparator.h"

#include "BoundingBox.h"
#include "Voi.h"
#include "MetadataInterface.h"

#include "SymbolCode.h"
#include "MetadataRankerLocalConfiguration.h"

#include "Statistics.h"
#include "StrClass.h"

#include "tinyxml.h"

#include <map>

#define voiLog if (pNetLog != NULL) pNetLog->logMsg

using namespace IHMC_VOI;
using namespace NOMADSUtil;

namespace COMPARATOR
{
    Score::Novelty compareAffiliation (String &oldNodeType, String &newNodeType)
    {
        IHMC_MISC_MIL_STD_2525::SymbolCode oldCode (oldNodeType);
        if (!oldCode.isValid()) {
            return Score::INSIGNIFICANT;
        }
        IHMC_MISC_MIL_STD_2525::SymbolCode newCode (newNodeType);
        if (!newCode.isValid()) {
            return Score::INSIGNIFICANT;
        }
        if (oldCode.getAffiliation() != newCode.getAffiliation()) {
            return Score::CRITICAL;
        }
        return Score::INSIGNIFICANT;
    }

    // Computed novelty depends on the amount of change in position as well as the distance between the IO and the user
    Score::Novelty comparePosition (const MetadataInterface *pOldMetadata, const MetadataInterface *pNewMetadata,
                                    Point *pPeerPosition, uint32 ui32RangeOfInfluence,
                                    MetadataRankerLocalConfiguration *pMetadataRankerLocalCfg)
    {
        const char *pszMethodName = "Comparator::comparePosition";
        const float fRange = (float) ui32RangeOfInfluence;
        Point oldTrackPos (pOldMetadata->getLocation (fRange).getBaricenter());
        Point newTrackPos (pNewMetadata->getLocation (fRange).getBaricenter());
        if (pPeerPosition == NULL) {
            // Can't do much...
            return Score::SIGNIFICANT;
        }
        const float fDistanceFromOldTrack = greatCircleDistance (oldTrackPos._fLatitude, oldTrackPos._fLongitude,
                                                                 pPeerPosition->_fLatitude, pPeerPosition->_fLongitude);
        const float fDistanceFromNewTrack = greatCircleDistance (newTrackPos._fLatitude, newTrackPos._fLongitude,
                                                                 pPeerPosition->_fLatitude, pPeerPosition->_fLongitude);
        const float fDistanceBetweenTracks = greatCircleDistance (oldTrackPos._fLatitude, oldTrackPos._fLongitude,
                                                                  newTrackPos._fLatitude, newTrackPos._fLongitude);
        const float fInsignPerc = pMetadataRankerLocalCfg == NULL ?
                                  MetadataRankerLocalConfiguration::TrackVoIConf::DEFAULT_INSIGNIFICANT_MOVEMENT_PERC :
                                  pMetadataRankerLocalCfg->_track.getInsignificantTrackMovementFactor();
        if ((fDistanceBetweenTracks < (fDistanceFromNewTrack * fInsignPerc)) &&
            (fDistanceBetweenTracks < (fDistanceFromOldTrack * fInsignPerc))) {
            voiLog (pszMethodName, Logger::L_LowDetailDebug, "Distance between tracks %f < distances from peer %f, %f\n",
                    fDistanceBetweenTracks, fDistanceFromNewTrack, fDistanceFromOldTrack);
            return Score::INSIGNIFICANT;
        }
        if (fDistanceFromNewTrack > fDistanceFromOldTrack) {
            return Score::SIGNIFICANT;
        }
        // It's getting closer
        return Score::CRITICAL;
    }

    Point getCurrentPosition (NodeContext *pNodeCtxt, bool &bErr)
    {
        bErr = false;
        float latitude = MAX_LATITUDE + 1;
        float longitude = MAX_LONGITUDE + 1;
        if (pNodeCtxt->getCurrentLatitude (latitude) < 0 || pNodeCtxt->getCurrentLongitude (longitude) < 0) {
            bErr = true;
        }
        if ((latitude > MAX_LATITUDE) || (latitude < MIN_LATITUDE)) {
            bErr = true;
        }
        if ((longitude > MAX_LONGITUDE) || (longitude < MIN_LONGITUDE)) {
            bErr = true;
        }
        return Point (latitude, longitude);
    }

    //-------------------------------------------------------------------------

#if !defined(ANDROID)

    struct Item
    {
        int iObjective;
        int iAvailable;
    };

    struct Supplies
    {
        String status;
        std::map<std::string, Item> itemsByNsn;
    };

    struct LogstatInfo
    {
        std::string date;
        String unit;
        String status;
        std::map<std::string, int> personnel;
        std::map<std::string, Supplies> suppliesByClass;

        int entryCount (void) const
        {
            return personnel.size() + suppliesByClass.size();
        }

        float compare (const LogstatInfo &newLogStat) const
        {
            const char *pszMethodName = "LogstatInfo";
            if (newLogStat.unit != unit) {
                return 1.0;
            }
            if (newLogStat.date < date) {
                // Obsolete
                return 0.0f;
            }
            if (newLogStat.status != status) {
                return 1.0f;
            }
            if ((personnel.size() < newLogStat.personnel.size()) ||
                (suppliesByClass.size() < newLogStat.suppliesByClass.size())) {
                // Update has new information
                return 1.0f;
            }
            if (newLogStat.entryCount() == 0) {
                // Neither LogStat contains any entry
                return 0.0f;
            }

            Statistics stats;
            for (const auto& kv : personnel) {
                const std::map<std::string, int>::const_iterator it = newLogStat.personnel.find (kv.first);
                const int iCount = (it == newLogStat.personnel.end()) ? 0 : it->second;
                double percVariation = (iCount > 0.0f) ? 1.0 : 0.0;
                if (kv.second > 0.0f) {
                    percVariation = abs ((kv.second - iCount)) / (float) kv.second;
                }
                voiLog (pszMethodName, Logger::L_LowDetailDebug, "Comparing %s: %d -> %d\n",
                        kv.first.c_str(), kv.second, iCount);
                stats.update (percVariation);
            }
            for (const auto& classKV : suppliesByClass) {
                const std::map<std::string, Supplies>::const_iterator newClassIt = newLogStat.suppliesByClass.find (classKV.first);
                if (newClassIt == newLogStat.suppliesByClass.end()) {
                    return 1.0f;
                }
                else {
                    for (const auto& itemKV : classKV.second.itemsByNsn) {
                        const std::map<std::string, Item>::const_iterator newItemIt = newClassIt->second.itemsByNsn.find (itemKV.first);
                        const int iCount = (newItemIt == newClassIt->second.itemsByNsn.end()) ? 0 : newItemIt->second.iAvailable;
                        double percVariation = (iCount > 0.0f) ? 1.0 : 0.0;
                        if (itemKV.second.iAvailable > 0.0f) {
                            percVariation = abs ((itemKV.second.iAvailable - iCount)) / (float) itemKV.second.iAvailable;
                        }
                        voiLog (pszMethodName, Logger::L_LowDetailDebug, "Comparing %s: %d -> %d\n",
                                classKV.first.c_str(), itemKV.second.iAvailable, iCount);
                        stats.update (percVariation);
                    }
                }
            }

            return (float) stats.getMax();
        }
    };

    void parseLogstat (const TiXmlElement *pLogstat, LogstatInfo &info)
    {
        if (pLogstat == NULL) {
            return;
        }
        static const char * SUPPLIES_TEMPLATE = "class*Supplies";
        static const char * ITEMS_TEMPLATE = "*Items";
        for (const TiXmlElement *pChild = pLogstat->FirstChildElement(); pChild != NULL; pChild = pChild->NextSiblingElement()) {
            const String element (pChild->Value());
            if (element == "created") {
                String s (pChild->GetText());
                s.trim();
                info.date = s.c_str();
            }
            else if (element == "unitName") {
                info.unit = pChild->GetText();
                info.unit.trim();
            }
            else if (element == "overallStatus") {
                const TiXmlElement *pStatus = pChild->FirstChildElement ("status");
                if (pStatus != NULL) {
                    info.status = pStatus->GetText();
                    info.status.trim();
                    info.status.convertToLowerCase();
                }
            }
            else if (element == "personnelList") {
                const TiXmlElement *pPersonnel = pChild->FirstChildElement();
                for (; pPersonnel != NULL; pPersonnel = pPersonnel->NextSiblingElement()) {
                    std::string categoy (pPersonnel->Value());
                    const char *pszValue = pPersonnel->GetText();
                    if (pszValue != NULL) {
                        int iValue = atoi (pszValue);
                        info.personnel[categoy] = iValue;
                    }
                }
            }
            else if (wildcardStringCompare (element, SUPPLIES_TEMPLATE)) {
                std::string supplyClass (pChild->Value());
                Supplies supplies;
                info.suppliesByClass[supplyClass] = supplies;
                const TiXmlElement *pSupply = pChild->FirstChildElement();
                for (; pSupply != NULL; pSupply = pSupply->NextSiblingElement()) {
                    String supplyElement (pSupply->Value());
                    if (supplyElement == "status") info.suppliesByClass[supplyClass].status = pSupply->GetText();
                    else if (wildcardStringCompare (supplyElement, ITEMS_TEMPLATE)) {
                        const TiXmlElement *pItem = pSupply->FirstChildElement();
                        for (; pItem != NULL; pItem = pItem->NextSiblingElement()) {
                            // For each item
                            std::string nsn;
                            const char *pszObjective, *pszAvailable, *pszReady;
                            pszObjective = pszAvailable = pszReady = NULL;
                            const TiXmlElement *pItemInfo = pItem->FirstChildElement();
                            for (; pItemInfo != NULL; pItemInfo = pItemInfo->NextSiblingElement()) {
                                String itemInfo (pItemInfo->Value());
                                if (itemInfo == "nsn") nsn = pItemInfo->GetText();
                                else if (itemInfo == "stockingObjective") pszObjective = pItemInfo->GetText();
                                else if (itemInfo == "onHand") pszAvailable = pItemInfo->GetText();
                                else if (itemInfo == "readiness") pszReady = pItemInfo->GetText();
                            }
                            if (pszObjective == NULL) pszObjective = "0";
                            if (pszAvailable == NULL) pszAvailable = "0";
                            if (pszReady == NULL) pszReady = "0";
                            Item item;
                            item.iAvailable = atoi (pszAvailable) + atoi (pszReady);
                            item.iObjective = atoi (pszObjective);
                            info.suppliesByClass[supplyClass].itemsByNsn[nsn] = item;
                        }
                    }
                }
            }
        }
    }
#endif
}

using namespace COMPARATOR;

Score::Novelty Comparator::compare (const MetadataInterface *pOldMetadata, const MetadataInterface *pNewMetadata,
                                    NodeContext *pNodeCtxt, MetadataRankerLocalConfiguration *pMetadataRankerLocalCfg)
{
    if (pOldMetadata == NULL) {
        return Score::SIGNIFICANT;
    }
    if (pNewMetadata == NULL) {
        return Score::INSIGNIFICANT;
    }
    String mimeType;
    if (pOldMetadata == NULL) {
        return Score::SIGNIFICANT;
    }
    if (pNewMetadata == NULL) {
        return Score::INSIGNIFICANT;
    }
    if (pNewMetadata->getFieldValue (MetadataInterface::DATA_FORMAT, mimeType) < 0) {
        return Score::SIGNIFICANT;
    }
    static const char * TRACK_TEMPLATE_SOI = "x-dspro/x-soi-track*";
    static const char * TRACK_TEMPLATE_PHOENIX = "x-dspro/x-phoenix-track*";
    static const char * LOGSTAT_TEMPLATE = "x-dspro/x-soi-logstat";

    if ((wildcardStringCompare (mimeType, TRACK_TEMPLATE_SOI) ||
         wildcardStringCompare (mimeType, TRACK_TEMPLATE_PHOENIX)) &&
        (pMetadataRankerLocalCfg == NULL || pMetadataRankerLocalCfg->_track.enabled())) {
        return compareTracks (pOldMetadata, pNewMetadata, pNodeCtxt, pMetadataRankerLocalCfg);
    }
    else if (wildcardStringCompare (mimeType, LOGSTAT_TEMPLATE) &&
             (pMetadataRankerLocalCfg == NULL || pMetadataRankerLocalCfg->_track.enabled())) {
        return compareLogstats (pOldMetadata, pNewMetadata, pNodeCtxt, pMetadataRankerLocalCfg);
    }
    // Unsupported types
    return Score::SIGNIFICANT;
}

//-----------------------------------------------------------------------------

float COMPARATOR::compareLogstats (const char *pszOldLogstat, const char *pszNewLogstat)
{
    TiXmlDocument oldDoc, newDoc;
    oldDoc.Parse (pszOldLogstat);
    newDoc.Parse (pszNewLogstat);
    if (oldDoc.Error() || newDoc.Error()) {
        return Score::SIGNIFICANT;
    }

    // SoI Wrapper
    static const char * INFOOBJ = "ns5:InformationObject";
    static const char * PAYLOAD = "ns5:Payload";
    static const char * LOGSTAT = "logstat";

    TiXmlElement *pOldLogStat, *pNewLogStat;
    TiXmlHandle oldDocHandle (&oldDoc);
    TiXmlHandle newDocHandle (&newDoc);
    pOldLogStat = oldDocHandle.FirstChild (INFOOBJ).FirstChild (PAYLOAD).FirstChild (LOGSTAT).ToElement();
    pNewLogStat = newDocHandle.FirstChild (INFOOBJ).FirstChild (PAYLOAD).FirstChild (LOGSTAT).ToElement();
    if ((pOldLogStat == NULL) && (pNewLogStat == NULL)) {
        // Assumes no wrapper
        pOldLogStat = oldDocHandle.FirstChild (LOGSTAT).ToElement();
        pOldLogStat = newDocHandle.FirstChild (LOGSTAT).ToElement();
    }
    if ((pOldLogStat == NULL) && (pNewLogStat == NULL)) {
        return Score::SIGNIFICANT;
    }
    if ((pOldLogStat == NULL) || (pNewLogStat == NULL)) {
        // Didn't have anything and it now has something,
        // or it had something but doesn't have anything now
        return Score::CRITICAL;
    }

#ifdef ANDROID
    return Score::SIGNIFICANT;
#else
    LogstatInfo oldLogstat, newLogstat;
    parseLogstat (pOldLogStat, oldLogstat);
    parseLogstat (pNewLogStat, newLogstat);

    return oldLogstat.compare (newLogstat);
#endif
}

Score::Novelty COMPARATOR::compareLogstats (const MetadataInterface *pOldMetadata, const MetadataInterface *pNewMetadata,
                                            NodeContext *pNodeCtxt, MetadataRankerLocalConfiguration *pMetadataRankerLocalCfg)
{
    String sOldLogstat, sNewLogstat;
    if (pOldMetadata->getFieldValue (MetadataInterface::APPLICATION_METADATA, sOldLogstat) < 0) {
        return Score::SIGNIFICANT;
    }
    if (pNewMetadata->getFieldValue (MetadataInterface::APPLICATION_METADATA, sNewLogstat) < 0) {
        return Score::SIGNIFICANT;
    }

    const float dMax = compareLogstats (sOldLogstat, sNewLogstat);
    const float fInsignPerc = pMetadataRankerLocalCfg == NULL ?
                              MetadataRankerLocalConfiguration::LogStatVoIConf::DEFAULT_INSIGNIFICANT_UPDATE_PERC :
                              pMetadataRankerLocalCfg->_logStat.getInsignificantUpdatePerc();

    if (dMax < fInsignPerc) {
        return Score::INSIGNIFICANT;
    }
    if (dMax > 0.2) {
        return Score::CRITICAL;
    }
    return Score::SIGNIFICANT;
}

//-----------------------------------------------------------------------------

Score::Novelty COMPARATOR::compareTracks (const MetadataInterface *pOldMetadata, const MetadataInterface *pNewMetadata,
                                          NodeContext *pNodeCtxt, MetadataRankerLocalConfiguration *pMetadataRankerLocalCfg)
{
    if (pNewMetadata == NULL) {
        return Score::INSIGNIFICANT;
    }
    if (pOldMetadata == NULL) {
        return Score::CRITICAL;
    }
    String dataFormat;
    pOldMetadata->getFieldValue (MetadataInterface::DATA_FORMAT, dataFormat);

    const String nodeTypeAttr (pMetadataRankerLocalCfg->getRangeOfInfluenceAttributeName ());
    String oldNodeType, newNodeType;
    pOldMetadata->getFieldValue (nodeTypeAttr, oldNodeType);
    pNewMetadata->getFieldValue (nodeTypeAttr, newNodeType);

    if (compareAffiliation (oldNodeType, newNodeType) == Score::CRITICAL) {
        // A change in affiliation (neutral -> enemy, enemy -> friendly, etc.)
        // is always considered critical
        return Score::CRITICAL;
    }

    bool bErr = false;
    Point peerPosition (getCurrentPosition (pNodeCtxt, bErr));
    Score::Novelty novelty = comparePosition (pOldMetadata, pNewMetadata, (bErr ? NULL : &peerPosition),
                                              pNodeCtxt->getUsefulDistance (dataFormat), pMetadataRankerLocalCfg);
    return novelty;
}

