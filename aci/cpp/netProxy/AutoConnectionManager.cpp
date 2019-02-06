#if defined (WIN32)
#define NOMINMAX    // For interference with std::min and std::max
#endif

#include <algorithm>
#include <chrono>

#include "UDPRawDatagramSocket.h"
#include "Logger.h"

#include "TuplesHashFunction.h"
#include "AutoConnectionManager.h"
#include "Connection.h"
#include "ConnectionManager.h"


#define checkAndLogMsg(_f_name_, _log_level_, ...) \
    if (NOMADSUtil::pLogger && (NOMADSUtil::pLogger->getDebugLevel() >= _log_level_)) \
        NOMADSUtil::pLogger->logMsg (_f_name_, _log_level_, __VA_ARGS__)

namespace ACMNetProxy {

    void AutoConnectionManager::run (void)
    {
        _bRunning = true;

        int rc;
        ConnectorType connectorType(CT_UNDEF);
        std::string sIPAddress;
        auto _umAutoConnectionKeys = _rConnectionManager.getAutoConnectionKeysMap();

        checkAndLogMsg ("AutoConnectionManager::run", NOMADSUtil::Logger::L_Info,
                        "AutoConnectionManager started!\n");

        std::unique_lock<std::mutex> ul{_mtx};
        while (!isTerminationRequested()) {
            // Check the AutoConnection Table
            if (_rConnectionManager.getNumberOfValidAutoConnectionEntries() == 0) {
                _bRunning = false;
                checkAndLogMsg ("AutoConnectionManager::run", NOMADSUtil::Logger::L_Warning,
                                "no entries left in the autoConnectionTable; stopping autoConnection thread!\n");
                return;
            }

            // Save current time and start checking all available AutoConnectionEntry instances
            bool bReloadAutoConnectionKeys{false};
            auto i64CurrTime = NOMADSUtil::getTimeInMilliseconds();
            auto i64NextCycleTime = i64CurrTime + ACM_TIME_BETWEEN_ITERATIONS;
            for (auto itNPUID = _umAutoConnectionKeys.begin(); itNPUID != _umAutoConnectionKeys.end(); ) {
                for (auto itpIPv4AddrCT = itNPUID->second.cbegin(); itpIPv4AddrCT != itNPUID->second.cend(); ) {
                    if (isTerminationRequested()) {
                        break;
                    }

                    // Retrieve the autoConnectionEntry from the ConnectionManager
                    auto spAutoConnectionEntry =
                        _rConnectionManager.getAutoConnectionEntryToRemoteProxyWithID (itNPUID->first, itpIPv4AddrCT->first,
                                                                                       static_cast<ConnectorType> (itpIPv4AddrCT->second));
                    if (!spAutoConnectionEntry) {
                        // Entry invalid --> delete corresponding key from the autoConnectionKeys table
                        itpIPv4AddrCT = itNPUID->second.erase (itpIPv4AddrCT);
                        bReloadAutoConnectionKeys = true;
                        continue;
                    }

                    // Acquire lock on the AutoConnectionEntry instance
                    std::lock_guard<std::mutex> lg{spAutoConnectionEntry->_mtx};
                    connectorType = spAutoConnectionEntry->getConnectorType();
                    /* Iterate over all available EncryptionTypes and try to establish a
                    * connection for each one that is present in the encryption descriptor */
                    bool bValidEncryptionType{false};
                    for (EncryptionType encryptionType : ET_AVAILABLE) {
                        if (!isEncryptionTypeInDescriptor (spAutoConnectionEntry->getConnectionEncryptionDescriptor(), encryptionType)) {
                            // Nothing to do for this Encryption Type
                            continue;
                        }

                        bValidEncryptionType = true;
                        if (!spAutoConnectionEntry->isValid (encryptionType)) {
                            // AutoConnection for this Encryption Type is not possible
                            continue;
                        }
                        if (spAutoConnectionEntry->isSynchronized (encryptionType)) {
                            // Already synchronized --> nothing to do
                            continue;
                        }

                        if (!spAutoConnectionEntry->areConnectivitySolutionsAvailableWithEncryption (encryptionType, _rConnectionManager)) {
                            spAutoConnectionEntry->setInvalid (encryptionType);
                            checkAndLogMsg ("AutoConnectionManager::run", NOMADSUtil::Logger::L_Warning,
                                            "impossible to establish a connection via %s using %s encryption to the remote NetProxy "
                                            "with UniqueID %u\n", connectorTypeToString (spAutoConnectionEntry->getConnectorType()),
                                            encryptionTypeToString (encryptionType), spAutoConnectionEntry->getRemoteNetProxyID());
                            continue;
                        }

                        // Save values in local variables to avoid problems in case of memory deallocation at program exit
                        const auto ui32RemoteProxyID = spAutoConnectionEntry->getRemoteNetProxyID();
                        const auto & iaRemoteNetProxyAddress = spAutoConnectionEntry->getRemoteProxyInetAddress();
                        const auto & iaLocalNetProxyAddress = (spAutoConnectionEntry->getLocalinterfaceInetAddress().getIPAddress() != 0) ?
                            spAutoConnectionEntry->getLocalinterfaceInetAddress() :
                            NOMADSUtil::UDPRawDatagramSocket::getLocalIPv4AddressToReachRemoteIPv4Address (iaRemoteNetProxyAddress);
                        if (Connection::isConnectingToRemoteNetProxyOnAddress (ui32RemoteProxyID, &iaLocalNetProxyAddress,
                                                                               &iaRemoteNetProxyAddress, connectorType, encryptionType)) {
                            // Connection is already being established
                            checkAndLogMsg ("AutoConnectionManager::run", NOMADSUtil::Logger::L_HighDetailDebug,
                                            "%sConnection to the remote NetProxy at address <%s:%hu> using %s encryption is not yet established\n",
                                            connectorTypeToString (connectorType), iaRemoteNetProxyAddress.getIPAsString(),
                                            iaRemoteNetProxyAddress.getPort(), encryptionTypeToString (encryptionType));
                            i64NextCycleTime = std::min (i64NextCycleTime, i64CurrTime + ACM_SHORT_TIME_BETWEEN_ITERATIONS);
                            continue;
                        }
                        else if ((i64CurrTime - spAutoConnectionEntry->getLastConnectionAttemptTime (encryptionType)) <
                            spAutoConnectionEntry->getAutoReconnectTimeInMillis()) {
                            // Timeout not yet expired
                            i64NextCycleTime = std::min (i64NextCycleTime, spAutoConnectionEntry->getLastConnectionAttemptTime (encryptionType) +
                                                         spAutoConnectionEntry->getAutoReconnectTimeInMillis());
                            continue;
                        }

                        // Proceed with attempting connection with the remote NetProxy
                        if (0 != (rc = spAutoConnectionEntry->synchronize (encryptionType, _rConnectionManager, _rTCPConnTable,
                                                                          _rTCPManager, _rPacketRouter, _rStatisticsManager))) {
                            checkAndLogMsg ("AutoConnectionManager::run", NOMADSUtil::Logger::L_Warning,
                                            "synchronize() attempt failed with rc = %d when trying to open a %s connection to the remote NetProxy "
                                            "with UniqueID %u at address <%s:%hu> using %s encryption\n", rc, connectorTypeToString (connectorType),
                                            ui32RemoteProxyID, iaRemoteNetProxyAddress.getIPAsString(), iaRemoteNetProxyAddress.getPort(),
                                            connectorTypeToString (connectorType), encryptionTypeToString (encryptionType));
                        }
                        else if (spAutoConnectionEntry->isSynchronized (encryptionType)) {
                            checkAndLogMsg ("AutoConnectionManager::run", NOMADSUtil::Logger::L_Info,
                                            "synchronization with the remote NetProxy with UniqueID %u and address <%s:%hu> "
                                            "was successfully performed via %s using %s encryption!\n", ui32RemoteProxyID,
                                            iaRemoteNetProxyAddress.getIPAsString(), iaRemoteNetProxyAddress.getPort(),
                                            connectorTypeToString (connectorType), encryptionTypeToString (encryptionType));
                        }
                        else {
                            checkAndLogMsg ("AutoConnectionManager::run", NOMADSUtil::Logger::L_HighDetailDebug,
                                            "waiting for the connection to the remote NetProxy with UniqueID %u and address "
                                            "<%s:%hu> to be established via %s using %s encryption\n", ui32RemoteProxyID,
                                            iaRemoteNetProxyAddress.getIPAsString(), iaRemoteNetProxyAddress.getPort(),
                                            connectorTypeToString (connectorType), encryptionTypeToString (encryptionType));
                            i64NextCycleTime = std::min (i64NextCycleTime, i64CurrTime + ACM_SHORT_TIME_BETWEEN_ITERATIONS);
                            continue;       // Avoids updating the last connection attempt time
                        }
                        spAutoConnectionEntry->updateLastConnectionAttemptTime (encryptionType, i64CurrTime);
                    }

                    if (!bValidEncryptionType) {
                        checkAndLogMsg ("AutoConnectionManager::run", NOMADSUtil::Logger::L_Warning,
                                        "no EncryptionType matching type %hhu exists; impossible to open a "
                                        "%sConnection to the remote NetProxy with UniqueID %u at address %s\n",
                                        spAutoConnectionEntry->getConnectionEncryptionDescriptor(),
                                        connectorTypeToString (connectorType), spAutoConnectionEntry->getRemoteNetProxyID(),
                                        spAutoConnectionEntry->getRemoteProxyInetAddress().getIPAsString());
                    }

                    // Next interface IPv4 address
                    ++itpIPv4AddrCT;
                }

                // Check if the current entry in the autoConnectionKeys table can be deleted
                if (itNPUID->second.size() == 0) {
                    _umAutoConnectionKeys.erase (itNPUID++);
                }
                else {
                    ++itNPUID;
                }
            }

            if (bReloadAutoConnectionKeys) {
                _umAutoConnectionKeys = _rConnectionManager.getAutoConnectionKeysMap();
            }

            auto i64TimeToWait = i64NextCycleTime - i64CurrTime;
            if ((i64TimeToWait > 0) || !isTerminationRequested()) {
                _cv.wait_for (ul, std::chrono::milliseconds{i64TimeToWait},
                              [this, i64NextCycleTime] {
                                return (NOMADSUtil::getTimeInMilliseconds() >= i64NextCycleTime) || isTerminationRequested();
                              });
            }
        }

        _bRunning = false;
    }

}
