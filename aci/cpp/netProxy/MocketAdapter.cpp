#include "Logger.h"

#include "MocketAdapter.h"
#include "ConfigurationParameters.h"


#define checkAndLogMsg(_f_name_, _log_level_, ...) \
    if (NOMADSUtil::pLogger && (NOMADSUtil::pLogger->getDebugLevel() >= _log_level_)) \
        NOMADSUtil::pLogger->logMsg (_f_name_, _log_level_, __VA_ARGS__)

namespace ACMNetProxy
{
    bool peerUnreachableWarning (void * pCallbackArg, unsigned long ulTimeSinceLastContact)
    {
        MocketAdapter * const pMocketAdapter = reinterpret_cast<MocketAdapter *> (pCallbackArg);
        const NOMADSUtil::InetAddr iaRemoteAddress{pMocketAdapter->_upMocket->getRemoteAddress(),
                                                   pMocketAdapter->_upMocket->getRemotePort()};
        if (ulTimeSinceLastContact > NetProxyApplicationParameters::MOCKET_TIMEOUT) {
            checkAndLogMsg ("Connection::peerUnreachableWarning", NOMADSUtil::Logger::L_Info,
                            "disconnecting %s connection with the remote NetProxy at address <%s:%hu>\n",
                            connectorTypeToString (CT_MOCKETS), iaRemoteAddress.getIPAsString(),
                            iaRemoteAddress.getPort());
            pMocketAdapter->close();
            return true;
        }

        return false;
    }
}
