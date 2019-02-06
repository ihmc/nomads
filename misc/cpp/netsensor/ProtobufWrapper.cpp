
#include "ProtobufWrapper.h"
using namespace NOMADSUtil;
#define checkAndLogMsg if (pLogger) pLogger->logMsg
namespace IHMC_NETSENSOR
{
    ProtobufWrapper::ProtobufWrapper(void)
    {

    }

    ProtobufWrapper::~ProtobufWrapper(void)
    {

    }

    measure::Measure * ProtobufWrapper::createNewMeasure()
    {
        return new measure::Measure();
    }

    measure::Measure * ProtobufWrapper::getMeasureIW (NOMADSUtil::String sSensorIP, IWStationDump& dump)
    {
        measure::Measure *pNewMeasure = new measure::Measure();
        setSubject(pNewMeasure, measure::Subject::iw);
        google::protobuf::Map<std::string, std::string> *pStrMap = pNewMeasure->mutable_strings();
        google::protobuf::Map<std::string, google::protobuf::int64> *pIntMap = pNewMeasure->mutable_integers();
        google::protobuf::Map<std::string, double> *pDoubleMap = pNewMeasure->mutable_doubles();

        (*pStrMap)[SENSOR_IP] = sSensorIP.c_str();
        (*pStrMap)[MAC_ADDR] = dump.sMacAddr.c_str();
        (*pStrMap)[DEV_NAME] = dump.sDevName.c_str();
        (*pStrMap)[TX_BITRATE] = dump.sTxBitrate.c_str();
        (*pStrMap)[P_LINK_STATE] = dump.sLinkState.c_str();
        (*pStrMap)[PEER_POWER_MODE] = dump.sPeerPowerMode.c_str();
        (*pStrMap)[LOCAL_POWER_MODE] = dump.sLocalPowerMode.c_str();
        (*pStrMap)[PREAMBLE] = dump.sPreamble.c_str();

        (*pIntMap)[INACTIVE_TIME] = dump.ui32InactiveTime;
        (*pIntMap)[RX_BYTES] = dump.ui32RxBytes;
        (*pIntMap)[RX_PACKETS] = dump.ui32RxPackets;
        (*pIntMap)[TX_BYTES] = dump.ui32TxBytes;
        (*pIntMap)[TX_PACKETS] = dump.ui32TxPackets;
        (*pIntMap)[TX_RETRIES] = dump.ui32TxRetries;
        (*pIntMap)[TX_FAILED] = dump.ui32TxFailed;
        (*pIntMap)[SIGNAL_PWR] = dump.ui8SignalPower;
        (*pIntMap)[SIGNAL_PWR_AVG] = dump.ui8SignalPowerAvg;
        (*pIntMap)[T_OFFSET] = dump.ui64TOffset;
        (*pIntMap)[MESH_LLID] = dump.meshLLID;
        (*pIntMap)[MESH_PLID] = dump.meshPLID;
        (*pIntMap)[AUTHENTICATED] = dump.bAuthenticated ? 1 : 0;
        (*pIntMap)[AUTHORIZED] = dump.bAuthorized ? 1 : 0;
        (*pIntMap)[WMM_WME] = dump.WMM_WME ? 1 : 0;
        (*pIntMap)[MFP] = dump.MFP ? 1 : 0;
        (*pIntMap)[TDLS] = dump.TDLS ? 1 : 0;

        return pNewMeasure;
    }

    measure::Measure * ProtobufWrapper::getMeasureRTT (String sSensorIP, String sSourceIP,
        String sDestIP, String sProtocol,
        String sSourcePort, String sDestPort,
        long minRTT, long maxRTT, long mostRecentRTT, long resolution, float avgRTT)
    {
        measure::Measure *pNewMeasure = new measure::Measure();
        setSubject(pNewMeasure, measure::Subject::rtt);
        google::protobuf::Map<std::string, std::string> *pStrMap = pNewMeasure->mutable_strings();
        google::protobuf::Map<std::string, google::protobuf::int64> *pIntMap = pNewMeasure->mutable_integers();
        google::protobuf::Map<std::string, double> *pDoubleMap = pNewMeasure->mutable_doubles();

        if (sSourceIP.c_str() == nullptr) {
            sSourceIP = "0";
        }
        if (sDestIP.c_str() == nullptr) {
            sDestIP = "0";
        }
        if (sProtocol.c_str() == nullptr) {
            sProtocol = "0";
        }
        if (sSourcePort.c_str() == nullptr) {
            sSourcePort = "0";
        }
        if (sDestPort.c_str() == nullptr) {
            sDestPort = "0";
        }

        (*pStrMap)[C_RTT_SENSOR_IP] = sSensorIP.c_str();
        (*pStrMap)[C_RTT_SRC_IP]    = sSourceIP.c_str();
        (*pStrMap)[C_RTT_DEST_IP]   = sDestIP.c_str();
        (*pStrMap)[C_RTT_PROTOCOL]  = sProtocol.c_str();
        (*pStrMap)[C_RTT_SRC_PORT]  = sSourcePort.c_str();
        (*pStrMap)[C_RTT_DEST_PORT] = sDestPort.c_str();

        (*pIntMap)[C_RTT_MIN_RTT] = minRTT;
        (*pIntMap)[C_RTT_MAX_RTT] = maxRTT;
        (*pIntMap)[C_RTT_RECENT_RTT] = mostRecentRTT;
        (*pIntMap)[C_RTT_RESOLUTION] = resolution;

        (*pDoubleMap)[C_RTT_AVG_VALUE] = avgRTT;


        return pNewMeasure;
    }

    void ProtobufWrapper::setSubject(measure::Measure *pMeasure, measure::Subject subject)
    {
        pMeasure->set_subject(subject);
    }

    void ProtobufWrapper::setMeasureTimestamp(measure::Measure *pMeasure, google::protobuf::Timestamp *pTs)
    {
        pMeasure->set_allocated_timestamp(pTs);
    }
}
