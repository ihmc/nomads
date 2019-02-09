package us.ihmc.aci.nodemon.sensors.custom;

import org.apache.log4j.Logger;
import us.ihmc.aci.ddam.DataType;
import us.ihmc.aci.disService.monitor.DisServiceMonitor;
import us.ihmc.aci.disService.monitor.DisServiceStatus;
import us.ihmc.aci.disService.monitor.DisServiceStatusListener;
import us.ihmc.aci.nodemon.NodeMon;
import us.ihmc.aci.nodemon.data.process.DisServiceContent;
import us.ihmc.aci.nodemon.data.process.Process;
import us.ihmc.aci.nodemon.sensors.PolledNodeSensor;

import java.net.SocketException;
import java.util.HashMap;
import java.util.Map;
import java.util.Objects;
import java.util.concurrent.ConcurrentHashMap;

/**
 * DisServicePolledNodeSensor.java
 * <p>
 * Class <code>DisServicePolledNodeSensor</code> defines a DisService protocol
 * sensor that requires periodic invocations to the method <code>updateData()</code> to receive data.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class DisServicePolledNodeSensor implements PolledNodeSensor, DisServiceStatusListener
{

    public DisServicePolledNodeSensor (NodeMon nodeMon, int port) throws SocketException
    {
        _nodeMon = Objects.requireNonNull(nodeMon, "NodeMon can't be null");
        if (port < 0 || port > 65535) {
            throw new IllegalArgumentException("Port has to be between 0 and 65535");
        }
        _statsByPeer = new ConcurrentHashMap<>();
        DisServiceMonitor m = new DisServiceMonitor(port);
        m.setListener(this);
        m.start();
    }

    @Override
    public void update (DataType type)
    {
//        if (!type.equals(NodeMonDataType.PROCESS)) {
//            return;
//            //throw new IllegalArgumentException("Sensor doesn't support type " + type);
//        }

        //log.debug("Requested DisService update for data: " + type);

        String localNodeId = _nodeMon.getWorldState().getLocalNodeId();

        StatWrapper sw = _statsByPeer.get(localNodeId);
        // check if local peer is// present
        if (sw == null) {
            //log.warn("No DisService stats found for local node");
            return;
        }

        DisServiceContent dsc = new DisServiceContent();

        if (sw._dsbsi != null) {
            dsc.setDataMessagesReceived(sw._dsbsi.dataMessagesReceived);
            dsc.setDataBytesReceived(sw._dsbsi.dataBytesReceived);
            dsc.setDataFragmentsReceived(sw._dsbsi.dataFragmentsReceived);
            dsc.setDataFragmentBytesReceived(sw._dsbsi.dataFragmentBytesReceived);
            dsc.setMissingFragmentRequestMessagesSent(sw._dsbsi.missingFragmentRequestMessagesSent);
            dsc.setMissingFragmentRequestBytesSent(sw._dsbsi.missingFragmentRequestBytesSent);
            dsc.setMissingFragmentRequestMessagesReceived(sw._dsbsi.missingFragmentRequestMessagesReceived);
            dsc.setMissingFragmentRequestBytesReceived(sw._dsbsi.missingFragmentRequestBytesSent);
            dsc.setDataCacheQueryMessagesSent(sw._dsbsi.dataCacheQueryMessagesSent);
            dsc.setDataCacheQueryBytesSent(sw._dsbsi.dataCacheQueryBytesSent);
            dsc.setDataCacheQueryMessagesReceived(sw._dsbsi.dataCacheQueryMessagesReceived);
            dsc.setDataCacheQueryBytesReceived(sw._dsbsi.dataCacheQueryBytesReceived);
            dsc.setTopologyStateMessagesSent(sw._dsbsi.topologyStateMessagesSent);
            dsc.setTopologyStateBytesSent(sw._dsbsi.topologyStateBytesSent);
            dsc.setTopologyStateBytesReceived(sw._dsbsi.topologyStateBytesReceived);
            dsc.setTopologyStateMessagesReceived(sw._dsbsi.topologyStateMessagesReceived);
            dsc.setKeepAliveMessagesSent(sw._dsbsi.keepAliveMessagesSent);
            dsc.setKeepAliveMessagesReceived(sw._dsbsi.keepAliveMessagesReceived);
            dsc.setQueryMessagesSent(sw._dsbsi.queryMessagesSent);
            dsc.setQueryMessagesReceived(sw._dsbsi.queryMessagesReceived);
            dsc.setQueryHitsMessagesSent(sw._dsbsi.queryHitsMessagesSent);
            dsc.setQueryMessagesReceived(sw._dsbsi.queryMessagesReceived);
        }

        Process p = new Process(localNodeId,
                Process.ProcessContentType.DISSERVICE,
                dsc);

        log.debug("Found DisService stats for local node");

        //TODO create a Process data structure
        //_nodeMon.updateData(localNodeId, NodeMonDataType.PROCESS, p);
    }


    @Override
    public void basicStatisticsInfoUpdateArrived (DisServiceStatus.DisServiceBasicStatisticsInfo dsbsi)
    {
        StatWrapper wr = _statsByPeer.get(dsbsi.peerId);
        if (wr == null) {
            wr = new StatWrapper();
            _statsByPeer.put(dsbsi.peerId, wr);
        }
        wr._dsbsi = dsbsi;
    }

    @Override
    public void overallStatsInfoUpdateArrived (DisServiceStatus.DisServiceStatsInfo dssi)
    {
        StatWrapper wr = _statsByPeer.get(dssi.peerId);
        if (wr == null) {
            wr = new StatWrapper();
            _statsByPeer.put(dssi.peerId, wr);
        }
        wr._dssi = dssi;
    }

    @Override
    public void duplicateTrafficStatisticInfoUpdateArrived (DisServiceStatus.DisServiceDuplicateTrafficInfo dsdti)
    {
        StatWrapper wr = _statsByPeer.get(dsdti.peerId);
        if (wr == null) {
            wr = new StatWrapper();
            _statsByPeer.put(dsdti.peerId, wr);
        }
        wr._dsdti = dsdti;

    }

    @Override
    public void peerGroupStatisticInfoUpdateArrived (DisServiceStatus.DisServiceBasicStatisticsInfoByPeer dsbsip)
    {
        StatWrapper wr = _statsByPeer.get(dsbsip.peerId);
        if (wr == null) {
            wr = new StatWrapper();
            _statsByPeer.put(dsbsip.peerId, wr);
        }
        wr._dsbsibps.put(dsbsip.remotePeerId, dsbsip);
    }

    class StatWrapper
    {
        StatWrapper ()
        {
            this(null, null, null, null);
        }

        StatWrapper (DisServiceStatus.DisServiceBasicStatisticsInfo dsbsi,
                     DisServiceStatus.DisServiceStatsInfo dssi,
                     DisServiceStatus.DisServiceDuplicateTrafficInfo dsdti,
                     DisServiceStatus.DisServiceBasicStatisticsInfoByPeer dsbsibp)
        {
            _dsbsi = dsbsi;
            _dssi = dssi;
            _dsdti = dsdti;
            _dsbsibps = new HashMap<>();
        }

        DisServiceStatus.DisServiceBasicStatisticsInfo _dsbsi;
        DisServiceStatus.DisServiceStatsInfo _dssi;
        DisServiceStatus.DisServiceDuplicateTrafficInfo _dsdti;
        Map<String, DisServiceStatus.DisServiceBasicStatisticsInfoByPeer> _dsbsibps;
    }

    private final Map<String, StatWrapper> _statsByPeer; // peerId -> StatWrapper
    private final NodeMon _nodeMon;
    private static final Logger log = Logger.getLogger(DisServicePolledNodeSensor.class);
}
