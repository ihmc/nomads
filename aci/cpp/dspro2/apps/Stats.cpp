#include "Stats.h"

#include "MetadataInterface.h"
#include "NetLogger.h"

#include "FileWriter.h"

using namespace NOMADSUtil;


const char * IHMC_ACI::MsgCounts::MSG_COUNTS_INTERFACE_NAME = "ifaceName";
const char * IHMC_ACI::MsgCounts::MSG_COUNTS_UNICAST_COUNT_NAME = "uniCount";
const char * IHMC_ACI::MsgCounts::MSG_COUNTS_MULTICAST_COUNT_NAME = "multiCount";

namespace DSPRO_STATS
{
    void increment (const char *pszKey, StringHashtable<uint32> &map)
    {
        if (pszKey == nullptr) {
            return;
        }
        uint32 *pVal = map.get (pszKey);
        if (pVal == nullptr) {
            pVal = new uint32;
            *pVal = 0;
            map.put (pszKey, pVal);
        }
        if (pVal != nullptr) {
            *pVal = (*pVal) + 1;
        }
    }

    void dump (StringHashtable<uint32> &map, FileWriter &fw, const char *pszMsg, const char *pszTemplate, IHMC_ACI::NetLogger::Measure *pMeasure)
    {
        if (pszMsg != nullptr) {
            fw.writeBytes (pszMsg, strlen (pszMsg));
        }
        typename StringHashtable<uint32>::Iterator iter = map.getAllElements();
        for (; !iter.end(); iter.nextElement()) {
            char chVal[23];
            uint32 val = *iter.getValue();
            sprintf (chVal, pszTemplate, val);

            String s (iter.getKey());
            s += ", ";
            s += chVal;
            s += '\n';

            fw.writeBytes (s.c_str(), s.length());

            if (pMeasure != nullptr) pMeasure->addUI32 (iter.getKey(), val);
        }
    }
}

using namespace IHMC_ACI;
using namespace IHMC_VOI;

const int64 Stats::DEFAULT_TIMEOUT = 15000;

MsgCounts::MsgCounts (const uint64 ui64Uni, const uint64 ui64Multi)
    : ui64UnicastMsgs{ui64Uni}, ui64MulticastMsgs{ui64Multi}
{
}

Stats::Stats (const char *pszNodeId, const char *pszFilename, int64 i64Timeout)
    : _i64Timeout (i64Timeout <= 0 ? DEFAULT_TIMEOUT : i64Timeout),
      _filename (pszFilename),
      _nodeId (pszNodeId),
     _pNewCountsByPeer (nullptr),
      // bCaseSensitiveKeys, bCloneKeys, bDeleteKeys, bDeleteValues
      _usageById (false, true, true, true),
      _pubByMimeType (false, true, true, true),
      _pubByGroup (false, true, true, true)
{
}

Stats::~Stats (void)
{
}

Stats * Stats::getInstance (const char *pszNodeId, int64 i64Timeout, const char *pszFilename)
{
    static Stats instance (pszNodeId, pszFilename, i64Timeout); // Guaranteed to be destroyed.
                                                                // Instantiated on first use.
    return &instance;
}

void Stats::run (void)
{
    started();
    if (_filename.length() > 0) {
        while (!terminationRequested()) {
            sleepForMilliseconds (_i64Timeout);
            FileWriter fw (_filename, "w");

            // Publications
            NetLogger::Measure *pByMime = MeasureFactory::createMeasure ("dspro.statistics.pub.types", NetLogger::Measure::PUBS);
            NetLogger::Measure *pByGroup = MeasureFactory::createMeasure ("dspro.statistics.pub.groups", NetLogger::Measure::PUBS);
            _mPub.lock();
            DSPRO_STATS::dump (_pubByMimeType, fw, "\nPublications by MIME type\n", "%u", pByMime);
            DSPRO_STATS::dump (_pubByGroup, fw, "\nPublications by Group name\n", "%u", pByGroup);
            _mPub.unlock();
            checkAndNotify (pByMime);    delete pByMime;
            checkAndNotify (pByGroup);   delete pByGroup;

            // Usage
            _mUsageById.lock();
            DSPRO_STATS::dump (_usageById, fw, "\nUsage by DSPro Id\n", "%u", nullptr);
            _mUsageById.unlock();

            // Matches
            NetLogger::Measure *pByPeer = MeasureFactory::createMeasure ("dspro.statistics.match.peeers", NetLogger::Measure::MATCH);
            _mMatchByPeer.lock();
            DSPRO_STATS::dump (_matchesByMimeType, fw, "\nMatches by MIME type\n", "%u", pByPeer);
            _mMatchByPeer.unlock();
            checkAndNotify (pByPeer);    delete pByPeer;

            pByMime = MeasureFactory::createMeasure ("dspro.statistics.match.types", NetLogger::Measure::MATCH);
            pByGroup = MeasureFactory::createMeasure ("dspro.statistics.match.groups", NetLogger::Measure::MATCH);
            _mMatchByType.lock();
            DSPRO_STATS::dump (_matchesByGroup, fw, "\nMatches by Group name\n", "%u", pByMime);
            DSPRO_STATS::dump (_matchesByPeer, fw, "\nMatches by Peer Id\n", "%u", pByGroup);
            _mMatchByType.unlock();
            checkAndNotify (pByMime);    delete pByMime;
            checkAndNotify (pByGroup);   delete pByGroup;

            // Counts
            _mCountsByPeer.lock();
            std::map<std::string, CountsByInterface> *pCountsByPeer = _pNewCountsByPeer;
            _pNewCountsByPeer = nullptr;
            _mCountsByPeer.unlock();
            if (pCountsByPeer != nullptr) {
                for (auto& byPeer : *pCountsByPeer) {
                    const std::string remotePeer (byPeer.first);
                    for (auto& byRemotePeerInterface : byPeer.second) {
                        const std::string remotePeerIface (byRemotePeerInterface.first);
                        for (auto& byIncomingInterface : byRemotePeerInterface.second) {
                            const std::string incomingIface (byIncomingInterface.first);

                            NetLogger::Measure *pByInterface = MeasureFactory::createMeasure ("dspro.statistics.network.count", NetLogger::Measure::NET);
                            if (pByInterface != nullptr) {
                                pByInterface->addString ("peer_id", _nodeId.c_str());
                                pByInterface->addString ("sensor_ip", incomingIface.c_str());

                                pByInterface->addString ("remote_peer_id", remotePeer.c_str());
                                pByInterface->addString ("remote_peer_ip", remotePeerIface.c_str());

                                pByInterface->addUI64 ("unicastCnt", byIncomingInterface.second.ui64Uni);
                                pByInterface->addUI64 ("multicastCnt", byIncomingInterface.second.ui64Multi);
                                checkAndNotify (pByInterface);   delete pByInterface;
                            }
                        }
                    }
                }

                delete pCountsByPeer;
            }

            fw.flush();
        }
    }
    terminating();
}

void Stats::addMessage (const char *pszGroup, MetadataInterface *pMetadata)
{
    String mimeType;
    if (pMetadata != nullptr) {
        pMetadata->getFieldValue (MetadataInterface::DATA_FORMAT, mimeType);
    }
    addMessage (pszGroup, mimeType);
}

void Stats::addMessage (const char *pszGroup, const char *pszMimeType)
{
    _mPub.lock();
    DSPRO_STATS::increment (pszMimeType, _pubByMimeType);
    DSPRO_STATS::increment (pszGroup, _pubByGroup);
    _mPub.unlock();
}

void Stats::addMatch (const char *pszTargetPeer)
{
    _mMatchByPeer.lock();
    DSPRO_STATS::increment (pszTargetPeer, _matchesByPeer);
    _mMatchByPeer.unlock();
}

void Stats::addMatch (const char *pszGroup, const char *pszMimeType)
{
    _mMatchByType.lock();
    DSPRO_STATS::increment (pszMimeType, _matchesByMimeType);
    DSPRO_STATS::increment (pszGroup, _matchesByGroup);
    _mMatchByType.unlock();
}

void Stats::getData (const char *pszId)
{
    _mUsageById.lock();
    DSPRO_STATS::increment (pszId, _usageById);
    _mUsageById.unlock();
}

void Stats::requestMoreChunks (const char *pszId)
{
    _mUsageById.lock();
    DSPRO_STATS::increment (pszId, _usageById);
    _mUsageById.unlock();
}

void Stats::messageCountUpdated (const char *pszPeerNodeId, const char *pszIncomingInterface, const char *pszPeerIp,
                                 uint64 ui64GroumMsgCount, uint64 ui64UnicastMsgCount)
{
    if (pszPeerNodeId == nullptr) {
        return;
    }

    MutexUnlocker synchronized (&_mCountsByPeer);
    if (_pNewCountsByPeer == nullptr) {
        _pNewCountsByPeer = new std::map<std::string, CountsByInterface>();
    }
    if (_pNewCountsByPeer == nullptr) return;

    (*_pNewCountsByPeer)[pszPeerNodeId][pszPeerIp][pszIncomingInterface].update (ui64GroumMsgCount, ui64UnicastMsgCount);
    _mCumulativeCountsByPeer[pszPeerNodeId][pszPeerIp][pszIncomingInterface].update (ui64GroumMsgCount, ui64UnicastMsgCount);
}

MsgCountsByInterface Stats::getPeerMsgCountsByInterface (const char * pszPeerNodeId) const
{
    MsgCountsByInterface res;
    if (pszPeerNodeId == nullptr) {
        return res;
    }

    MutexUnlocker synchronized (&_mCountsByPeer);
    if (_mCumulativeCountsByPeer.count (pszPeerNodeId) == 0) {
        return res;
    }
    for (const auto & rpLocalIfaceCounts : _mCumulativeCountsByPeer.at (pszPeerNodeId)) {
        uint64 ui64UnicastMsgs = 0, ui64MulticastMsgs = 0;
        for (const auto & rpRemoteIfaceCounts : rpLocalIfaceCounts.second) {
            ui64UnicastMsgs += rpRemoteIfaceCounts.second.ui64Uni;
            ui64MulticastMsgs += rpRemoteIfaceCounts.second.ui64Multi;
        }
        res.emplace (std::piecewise_construct, std::forward_as_tuple (rpLocalIfaceCounts.first),
                     std::forward_as_tuple (ui64UnicastMsgs, ui64MulticastMsgs));
    }

    return res;
}

Stats::Counts::Counts (void)
    : ui64Multi (0U), ui64Uni (0U)
{
}

Stats::Counts::~Counts (void)
{
}

void Stats::Counts::update (uint64 ui64GroumMsgCnt, uint64 ui64UnicastMsgCnt)
{
    ui64Multi = ui64GroumMsgCnt;
    ui64Uni = ui64UnicastMsgCnt;
}

