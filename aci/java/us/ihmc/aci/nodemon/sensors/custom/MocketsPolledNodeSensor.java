package us.ihmc.aci.nodemon.sensors.custom;

import com.google.protobuf.util.TimeUtil;
import org.apache.log4j.Logger;
import us.ihmc.aci.ddam.*;
import us.ihmc.aci.nodemon.NodeMon;
import us.ihmc.aci.nodemon.data.process.MocketsContent;
import us.ihmc.aci.nodemon.data.process.Process;
import us.ihmc.aci.nodemon.proto.ProtoUtils;
import us.ihmc.aci.nodemon.sensors.PolledNodeSensor;
import us.ihmc.aci.nodemon.util.Utils;
import us.ihmc.mockets.monitor.MocketStatus;
import us.ihmc.mockets.monitor.MocketStatusListener;
import us.ihmc.mockets.monitor.Monitor;

import java.net.SocketException;
import java.util.*;

/**
 * MocketsPolledNodeSensor.java
 * <p/>
 * Class <code>MocketsPolledNodeSensor</code> defines a Mockets protocol
 * sensor that requires periodic invocations to the method <code>updateData()</code> to receive data.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class MocketsPolledNodeSensor implements PolledNodeSensor, MocketStatusListener
{
    public MocketsPolledNodeSensor (NodeMon nodeMon, int port) throws SocketException
    {
        _nodeMon = Objects.requireNonNull(nodeMon, "NodeMon can't be null");
        if (port < 0 || port > 65535) {
            throw new IllegalArgumentException("Port has to be between 0 and 65535");
        }
        _statsByPID = new HashMap<>();
        Monitor m = new Monitor(port);
        m.setListener(this);
        m.start();
    }

    @Override
    public void update (DataType type)
    {
        if (!type.equals(DataType.LINK)) {
            return;
        }

        //log.debug("Requested Mockets update for data: " + type);
        Collection<StatWrapper> col = new HashSet<>(_statsByPID.values());
        MocketsContent mc = new MocketsContent();

        for (StatWrapper sw : col) {

            if (sw.epi == null) {
                continue;
            }

            mc.setPID(sw.epi.PID);
            mc.setIdentifier(sw.epi.identifier);
            if (sw.epi.localAddress != null) mc.setLocalAddr(sw.epi.localAddress.getHostAddress());
            mc.setLocalAddrLong(sw.epi.localAddr);
            mc.setLocalPort((int) sw.epi.localPort);
            if (sw.epi.remoteAddress != null) mc.setRemoteAddr(sw.epi.remoteAddress.getHostAddress());
            mc.setRemoteAddrLong(sw.epi.remoteAddr);
            mc.setRemotePort((int) sw.epi.remotePort);

            if (sw.msi != null) {
                mc.setLastContactTime(String.valueOf(sw.msi.lastContactTime));
                mc.setSentBytes(sw.msi.sentBytes);
                mc.setSentPackets(sw.msi.sentPackets);
                mc.setRetransmits(sw.msi.retransmits);
                mc.setReceivedBytes(sw.msi.receivedBytes);
                mc.setReceivedPackets(sw.msi.receivedPackets);
                mc.setDuplicatedDiscardedPackets(sw.msi.duplicatedDiscardedPackets);
                mc.setNoRoomDiscardedPackets(sw.msi.noRoomDiscardedPackets);
                mc.setReassemblySkippedDiscardedPackets(sw.msi.reassemblySkippedDiscardedPackets);
                mc.setEstimatedRTT(sw.msi.estimatedRTT);
                mc.setUnacknowledgedDataSize(sw.msi.unacknowledgedDataSize);
                mc.setPendingDataSize(sw.msi.pendingDataSize);
                mc.setPendingPacketQueueSize(sw.msi.pendingPacketQueueSize);
                mc.setReliableSequencedDataSize(sw.msi.reliableSequencedDataSize);
                mc.setReliableSequencedPacketQueueSize(sw.msi.reliableSequencedPacketQueueSize);
                mc.setReliableUnsequencedDataSize(sw.msi.reliableUnsequencedDataSize);
                mc.setReliableUnsequencedPacketQueueSize(sw.msi.reliableUnsequencedPacketQueueSize);
            }

            //TODO add sw.messSI
            String localNodeId = _nodeMon.getWorldState().getLocalNode().getId();
            //String id = getMocketsCustomProcessId(sw.epi.PID, localNodeId);


            processMessage(mc);
        }
    }

    private void processMessage (MocketsContent mc)
    {
        Integer localAddr = Utils.convertIPToInteger(mc.getLocalAddr());
        if (localAddr == null) {
            log.warn("{*M*} Mockets local address is null, unable to process");
            return;
        }
        Integer remoteAddr = Utils.convertIPToInteger(mc.getRemoteAddr());
        if (remoteAddr == null) {
            log.warn("{*M*} Mockets remote address is null, unable to process");
            return;
        }

        double RTT = mc.getEstimatedRTT();
        log.debug("{*M*} Mockets link " + mc.getLocalAddr() + " -> " + mc.getRemoteAddr() + " RTT: " + mc
                .getEstimatedRTT());
        Link newLink = Link.newBuilder()
                .setIpSrc(localAddr)
                .setIpDst(remoteAddr)
                .setMocketSensor(MocketSensor.newBuilder()
                        .setRTT(RTT)
                        .build())
                .setTimestamp(TimeUtil.getCurrentTime())
                .build();

        Container c = ProtoUtils.toContainer(DataType.LINK, _nodeMon.getWorldState().getLocalNodeId(), newLink);
        _nodeMon.getWorldState().updateData(_nodeMon.getWorldState().getLocalNodeId(), c);
    }

    private String getMocketsCustomProcessId (long PID, String localNodeId)
    {
        return localNodeId + "-" + Process.ProcessContentType.MOCKETS.toString() + "-" + String.valueOf(PID);
    }

    @Override
    public void statusUpdateArrived (MocketStatus.EndPointsInfo epi, byte msgType)
    {
        //do nothing
    }

    @Override
    public void statisticsInfoUpdateArrived (MocketStatus.EndPointsInfo epi, MocketStatus.MocketStatisticsInfo msi)
    {
        StatWrapper sw;
        sw = _statsByPID.get(epi.PID);
        if (sw == null) {
            sw = new StatWrapper();
        }
        sw.epi = epi;
        sw.msi = msi;
        _statsByPID.put(epi.PID, sw);
    }

    @Override
    public void statisticsInfoUpdateArrived (MocketStatus.EndPointsInfo epi, MocketStatus.MessageStatisticsInfo messSi)
    {
        StatWrapper sw;
        sw = _statsByPID.get(epi.PID);
        if (sw == null) {
            sw = new StatWrapper();
        }
        sw.epi = epi;
        sw.messSi = messSi;
        _statsByPID.put(epi.PID, sw);
    }

    class StatWrapper
    {
        StatWrapper ()
        {
            this(null, null, null);
        }

        StatWrapper (MocketStatus.EndPointsInfo epi,
                     MocketStatus.MocketStatisticsInfo msi,
                     MocketStatus.MessageStatisticsInfo messSi)
        {
            this.epi = epi;
            this.msi = msi;
            this.messSi = messSi;
        }

        MocketStatus.EndPointsInfo epi;
        MocketStatus.MocketStatisticsInfo msi;
        MocketStatus.MessageStatisticsInfo messSi;
    }

    private final NodeMon _nodeMon;
    private final Map<Long, StatWrapper> _statsByPID; // PID -> StatWrapper
    private static final Logger log = Logger.getLogger(MocketsPolledNodeSensor.class);
}
