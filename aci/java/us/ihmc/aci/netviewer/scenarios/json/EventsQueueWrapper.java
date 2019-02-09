package us.ihmc.aci.netviewer.scenarios.json;

import java.util.ArrayList;
import java.util.List;

/**
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class EventsQueueWrapper
{
    // General
    private Integer sleep;
    private String type;
    private String id;
    private String name;
    private List<String> ips = new ArrayList<>();
    private List<String> neighbors = new ArrayList<>();

    // Updates
    private String nmDataType;
    private String nvType;
    private String localAddr;
    private String remoteAddr;
    private Integer value;
    
    // Disservice Updates
    private Integer dataMessagesReceived;
    private Integer dataBytesReceived;
    private Integer dataFragmentsReceived;
    private Integer dataFragmentBytesReceived;
    private Integer missingFragmentRequestMessagesSent;
    private Integer missingFragmentRequestBytesSent;
    private Integer missingFragmentRequestMessagesReceived;
    private Integer missingFragmentRequestBytesReceived;
    private Integer dataCacheQueryMessagesSent;
    private Integer dataCacheQueryBytesSent;
    private Integer dataCacheQueryMessagesReceived;
    private Integer dataCacheQueryBytesReceived;
    private Integer topologyStateMessagesSent;
    private Integer topologyStateBytesSent;
    private Integer topologyStateMessagesReceived;
    private Integer topologyStateBytesReceived;
    private Integer keepAliveMessagesSent;
    private Integer keepAliveMessagesReceived;
    private Integer queryMessagesSent;
    private Integer queryMessagesReceived;
    private Integer queryHitsMessagesSent;
    private Integer queryHitsMessagesReceived;

    // Mockets Updates
    private String lastContactTime;
    private Integer sentBytes;
    private Integer sentPackets;
    private Integer retransmits;
    private Integer receivedBytes;
    private Integer receivedPackets;
    private Integer duplicatedDiscardedPackets;
    private Integer noRoomDiscardedPackets;
    private Integer reassemblySkippedDiscardedPackets;
    private Integer estimatedRTT;
    private Integer unacknowledgedDataSize;
    private Integer unacknowledgedQueueSize;
    private Integer pendingDataSize;
    private Integer pendingPacketQueueSize;
    private Integer reliableSequencedDataSize;
    private Integer reliableSequencedPacketQueueSize;
    private Integer reliableUnsequencedDataSize;
    private Integer reliableUnsequencedPacketQueueSize;

    /**
     *
     * @return
     * The sleep
     */
    public Integer getSleep()
    {
        return sleep;
    }

    /**
     *
     * @param sleep
     * The sleep
     */
    public void setSleep (Integer sleep)
    {
        this.sleep = sleep;
    }

    /**
     *
     * @return
     * The type
     */
    public String getType()
    {
        return type;
    }

    /**
     *
     * @param type
     * The type
     */
    public void setType (String type)
    {
        this.type = type;
    }

    /**
     *
     * @return
     * The id
     */
    public String getId()
    {
        return id;
    }

    /**
     *
     * @param id
     * The id
     */
    public void setId (String id)
    {
        this.id = id;
    }

    /**
     *
     * @return
     * The name
     */
    public String getName()
    {
        return name;
    }

    /**
     *
     * @param name
     * The name
     */
    public void setName (String name)
    {
        this.name = name;
    }

    /**
     *
     * @return
     * The ips
     */
    public List<String> getIps()
    {
        return ips;
    }

    /**
     *
     * @param ips
     * The ips
     */
    public void setIps (List<String> ips)
    {
        this.ips = ips;
    }

    /**
     *
     * @return
     * The neighbors
     */
    public List<String> getNeighbors()
    {
        return neighbors;
    }

    /**
     *
     * @param neighbors
     * The neighbors
     */
    public void setNeighbors (List<String> neighbors)
    {
        this.neighbors = neighbors;
    }

    /**
     *
     * @return
     * The nmDataType
     */
    public String getNmDataType()
    {
        return nmDataType;
    }

    /**
     *
     * @param nmDataType
     * The nmDataType
     */
    public void setNmDataType (String nmDataType)
    {
        this.nmDataType = nmDataType;
    }

    /**
     *
     * @return
     * The nvType
     */
    public String getNvType()
    {
        return nvType;
    }

    /**
     *
     * @param nvType
     * The nvType
     */
    public void setNvType (String nvType)
    {
        this.nvType = nvType;
    }

    /**
     *
     * @return
     * The local addr
     */
    public String getLocalAddr()
    {
        return localAddr;
    }

    /**
     *
     * @param localAddr
     * The local addr
     */
    public void setLocalAddr (String localAddr)
    {
        this.localAddr = localAddr;
    }

    /**
     *
     * @return
     * The remote addr
     */
    public String getRemoteAddr()
    {
        return remoteAddr;
    }

    /**
     *
     * @param remoteAddr
     * The remote addr
     */
    public void setRemoteAddr (String remoteAddr)
    {
        this.remoteAddr = remoteAddr;
    }

    /**
     *
     * @return
     * The value
     */
    public Integer getValue()
    {
        return value;
    }

    /**
     *
     * @param value
     * The value
     */
    public void setValue (Integer value)
    {
        this.value = value;
    }

    /**
     *
     * @return
     * The dataMessagesReceived
     */
    public Integer getDataMessagesReceived()
    {
        return dataMessagesReceived;
    }

    /**
     *
     * @param dataMessagesReceived
     * The dataMessagesReceived
     */
    public void setDataMessagesReceived (Integer dataMessagesReceived)
    {
        this.dataMessagesReceived = dataMessagesReceived;
    }

    /**
     *
     * @return
     * The dataBytesReceived
     */
    public Integer getDataBytesReceived()
    {
        return dataBytesReceived;
    }

    /**
     *
     * @param dataBytesReceived
     * The dataBytesReceived
     */
    public void setDataBytesReceived (Integer dataBytesReceived)
    {
        this.dataBytesReceived = dataBytesReceived;
    }

    /**
     *
     * @return
     * The dataFragmentsReceived
     */
    public Integer getDataFragmentsReceived()
    {
        return dataFragmentsReceived;
    }

    /**
     *
     * @param dataFragmentsReceived
     * The dataFragmentsReceived
     */
    public void setDataFragmentsReceived (Integer dataFragmentsReceived)
    {
        this.dataFragmentsReceived = dataFragmentsReceived;
    }

    /**
     *
     * @return
     * The dataFragmentBytesReceived
     */
    public Integer getDataFragmentBytesReceived()
    {
        return dataFragmentBytesReceived;
    }

    /**
     *
     * @param dataFragmentBytesReceived
     * The dataFragmentBytesReceived
     */
    public void setDataFragmentBytesReceived (Integer dataFragmentBytesReceived)
    {
        this.dataFragmentBytesReceived = dataFragmentBytesReceived;
    }

    /**
     *
     * @return
     * The missingFragmentRequestMessagesSent
     */
    public Integer getMissingFragmentRequestMessagesSent()
    {
        return missingFragmentRequestMessagesSent;
    }

    /**
     *
     * @param missingFragmentRequestMessagesSent
     * The missingFragmentRequestMessagesSent
     */
    public void setMissingFragmentRequestMessagesSent (Integer missingFragmentRequestMessagesSent)
    {
        this.missingFragmentRequestMessagesSent = missingFragmentRequestMessagesSent;
    }

    /**
     *
     * @return
     * The missingFragmentRequestBytesSent
     */
    public Integer getMissingFragmentRequestBytesSent()
    {
        return missingFragmentRequestBytesSent;
    }

    /**
     *
     * @param missingFragmentRequestBytesSent
     * The missingFragmentRequestBytesSent
     */
    public void setMissingFragmentRequestBytesSent (Integer missingFragmentRequestBytesSent)
    {
        this.missingFragmentRequestBytesSent = missingFragmentRequestBytesSent;
    }

    /**
     *
     * @return
     * The missingFragmentRequestMessagesReceived
     */
    public Integer getMissingFragmentRequestMessagesReceived()
    {
        return missingFragmentRequestMessagesReceived;
    }

    /**
     *
     * @param missingFragmentRequestMessagesReceived
     * The missingFragmentRequestMessagesReceived
     */
    public void setMissingFragmentRequestMessagesReceived (Integer missingFragmentRequestMessagesReceived)
    {
        this.missingFragmentRequestMessagesReceived = missingFragmentRequestMessagesReceived;
    }

    /**
     *
     * @return
     * The missingFragmentRequestBytesReceived
     */
    public Integer getMissingFragmentRequestBytesReceived()
    {
        return missingFragmentRequestBytesReceived;
    }

    /**
     *
     * @param missingFragmentRequestBytesReceived
     * The missingFragmentRequestBytesReceived
     */
    public void setMissingFragmentRequestBytesReceived (Integer missingFragmentRequestBytesReceived)
    {
        this.missingFragmentRequestBytesReceived = missingFragmentRequestBytesReceived;
    }

    /**
     *
     * @return
     * The dataCacheQueryMessagesSent
     */
    public Integer getDataCacheQueryMessagesSent()
    {
        return dataCacheQueryMessagesSent;
    }

    /**
     *
     * @param dataCacheQueryMessagesSent
     * The dataCacheQueryMessagesSent
     */
    public void setDataCacheQueryMessagesSent (Integer dataCacheQueryMessagesSent)
    {
        this.dataCacheQueryMessagesSent = dataCacheQueryMessagesSent;
    }

    /**
     *
     * @return
     * The dataCacheQueryBytesSent
     */
    public Integer getDataCacheQueryBytesSent()
    {
        return dataCacheQueryBytesSent;
    }

    /**
     *
     * @param dataCacheQueryBytesSent
     * The dataCacheQueryBytesSent
     */
    public void setDataCacheQueryBytesSent (Integer dataCacheQueryBytesSent)
    {
        this.dataCacheQueryBytesSent = dataCacheQueryBytesSent;
    }

    /**
     *
     * @return
     * The dataCacheQueryMessagesReceived
     */
    public Integer getDataCacheQueryMessagesReceived()
    {
        return dataCacheQueryMessagesReceived;
    }

    /**
     *
     * @param dataCacheQueryMessagesReceived
     * The dataCacheQueryMessagesReceived
     */
    public void setDataCacheQueryMessagesReceived (Integer dataCacheQueryMessagesReceived)
    {
        this.dataCacheQueryMessagesReceived = dataCacheQueryMessagesReceived;
    }

    /**
     *
     * @return
     * The dataCacheQueryBytesReceived
     */
    public Integer getDataCacheQueryBytesReceived()
    {
        return dataCacheQueryBytesReceived;
    }

    /**
     *
     * @param dataCacheQueryBytesReceived
     * The dataCacheQueryBytesReceived
     */
    public void setDataCacheQueryBytesReceived (Integer dataCacheQueryBytesReceived)
    {
        this.dataCacheQueryBytesReceived = dataCacheQueryBytesReceived;
    }

    /**
     *
     * @return
     * The topologyStateMessagesSent
     */
    public Integer getTopologyStateMessagesSent()
    {
        return topologyStateMessagesSent;
    }

    /**
     *
     * @param topologyStateMessagesSent
     * The topologyStateMessagesSent
     */
    public void setTopologyStateMessagesSent (Integer topologyStateMessagesSent)
    {
        this.topologyStateMessagesSent = topologyStateMessagesSent;
    }

    /**
     *
     * @return
     * The topologyStateBytesSent
     */
    public Integer getTopologyStateBytesSent()
    {
        return topologyStateBytesSent;
    }

    /**
     *
     * @param topologyStateBytesSent
     * The topologyStateBytesSent
     */
    public void setTopologyStateBytesSent (Integer topologyStateBytesSent)
    {
        this.topologyStateBytesSent = topologyStateBytesSent;
    }

    /**
     *
     * @return
     * The topologyStateMessagesReceived
     */
    public Integer getTopologyStateMessagesReceived()
    {
        return topologyStateMessagesReceived;
    }

    /**
     *
     * @param topologyStateMessagesReceived
     * The topologyStateMessagesReceived
     */
    public void setTopologyStateMessagesReceived (Integer topologyStateMessagesReceived)
    {
        this.topologyStateMessagesReceived = topologyStateMessagesReceived;
    }

    /**
     *
     * @return
     * The topologyStateBytesReceived
     */
    public Integer getTopologyStateBytesReceived()
    {
        return topologyStateBytesReceived;
    }

    /**
     *
     * @param topologyStateBytesReceived
     * The topologyStateBytesReceived
     */
    public void setTopologyStateBytesReceived (Integer topologyStateBytesReceived)
    {
        this.topologyStateBytesReceived = topologyStateBytesReceived;
    }

    /**
     *
     * @return
     * The keepAliveMessagesSent
     */
    public Integer getKeepAliveMessagesSent()
    {
        return keepAliveMessagesSent;
    }

    /**
     *
     * @param keepAliveMessagesSent
     * The keepAliveMessagesSent
     */
    public void setKeepAliveMessagesSent (Integer keepAliveMessagesSent)
    {
        this.keepAliveMessagesSent = keepAliveMessagesSent;
    }

    /**
     *
     * @return
     * The keepAliveMessagesReceived
     */
    public Integer getKeepAliveMessagesReceived()
    {
        return keepAliveMessagesReceived;
    }

    /**
     *
     * @param keepAliveMessagesReceived
     * The keepAliveMessagesReceived
     */
    public void setKeepAliveMessagesReceived (Integer keepAliveMessagesReceived)
    {
        this.keepAliveMessagesReceived = keepAliveMessagesReceived;
    }

    /**
     *
     * @return
     * The queryMessagesSent
     */
    public Integer getQueryMessagesSent()
    {
        return queryMessagesSent;
    }

    /**
     *
     * @param queryMessagesSent
     * The queryMessagesSent
     */
    public void setQueryMessagesSent (Integer queryMessagesSent)
    {
        this.queryMessagesSent = queryMessagesSent;
    }

    /**
     *
     * @return
     * The queryMessagesReceived
     */
    public Integer getQueryMessagesReceived()
    {
        return queryMessagesReceived;
    }

    /**
     *
     * @param queryMessagesReceived
     * The queryMessagesReceived
     */
    public void setQueryMessagesReceived (Integer queryMessagesReceived)
    {
        this.queryMessagesReceived = queryMessagesReceived;
    }

    /**
     *
     * @return
     * The queryHitsMessagesSent
     */
    public Integer getQueryHitsMessagesSent()
    {
        return queryHitsMessagesSent;
    }

    /**
     *
     * @param queryHitsMessagesSent
     * The queryHitsMessagesSent
     */
    public void setQueryHitsMessagesSent (Integer queryHitsMessagesSent)
    {
        this.queryHitsMessagesSent = queryHitsMessagesSent;
    }

    /**
     *
     * @return
     * The queryHitsMessagesReceived
     */
    public Integer getQueryHitsMessagesReceived()
    {
        return queryHitsMessagesReceived;
    }

    /**
     *
     * @param queryHitsMessagesReceived
     * The queryHitsMessagesReceived
     */
    public void setQueryHitsMessagesReceived (Integer queryHitsMessagesReceived)
    {
        this.queryHitsMessagesReceived = queryHitsMessagesReceived;
    }

    /**
     *
     * @return
     * The lastContactTime
     */
    public String getLastContactTime()
    {
        return lastContactTime;
    }

    /**
     *
     * @param lastContactTime
     * The lastContactTime
     */
    public void setLastContactTime (String lastContactTime)
    {
        this.lastContactTime = lastContactTime;
    }

    /**
     *
     * @return
     * The sentBytes
     */
    public Integer getSentBytes()
    {
        return sentBytes;
    }

    /**
     *
     * @param sentBytes
     * The sentBytes
     */
    public void setSentBytes (Integer sentBytes)
    {
        this.sentBytes = sentBytes;
    }

    /**
     *
     * @return
     * The sentPackets
     */
    public Integer getSentPackets()
    {
        return sentPackets;
    }

    /**
     *
     * @param sentPackets
     * The sentPackets
     */
    public void setSentPackets (Integer sentPackets)
    {
        this.sentPackets = sentPackets;
    }

    /**
     *
     * @return
     * The retransmits
     */
    public Integer getRetransmits()
    {
        return retransmits;
    }

    /**
     *
     * @param retransmits
     * The retransmits
     */
    public void setRetransmits (Integer retransmits)
    {
        this.retransmits = retransmits;
    }

    /**
     *
     * @return
     * The receivedBytes
     */
    public Integer getReceivedBytes()
    {
        return receivedBytes;
    }

    /**
     *
     * @param receivedBytes
     * The receivedBytes
     */
    public void setReceivedBytes (Integer receivedBytes)
    {
        this.receivedBytes = receivedBytes;
    }

    /**
     *
     * @return
     * The receivedPackets
     */
    public Integer getReceivedPackets()
    {
        return receivedPackets;
    }

    /**
     *
     * @param receivedPackets
     * The receivedPackets
     */
    public void setReceivedPackets (Integer receivedPackets)
    {
        this.receivedPackets = receivedPackets;
    }

    /**
     *
     * @return
     * The duplicatedDiscardedPackets
     */
    public Integer getDuplicatedDiscardedPackets()
    {
        return duplicatedDiscardedPackets;
    }

    /**
     *
     * @param duplicatedDiscardedPackets
     * The duplicatedDiscardedPackets
     */
    public void setDuplicatedDiscardedPackets (Integer duplicatedDiscardedPackets)
    {
        this.duplicatedDiscardedPackets = duplicatedDiscardedPackets;
    }

    /**
     *
     * @return
     * The noRoomDiscardedPackets
     */
    public Integer getNoRoomDiscardedPackets()
    {
        return noRoomDiscardedPackets;
    }

    /**
     *
     * @param noRoomDiscardedPackets
     * The noRoomDiscardedPackets
     */
    public void setNoRoomDiscardedPackets (Integer noRoomDiscardedPackets)
    {
        this.noRoomDiscardedPackets = noRoomDiscardedPackets;
    }

    /**
     *
     * @return
     * The reassemblySkippedDiscardedPackets
     */
    public Integer getReassemblySkippedDiscardedPackets()
    {
        return reassemblySkippedDiscardedPackets;
    }

    /**
     *
     * @param reassemblySkippedDiscardedPackets
     * The reassemblySkippedDiscardedPackets
     */
    public void setReassemblySkippedDiscardedPackets (Integer reassemblySkippedDiscardedPackets)
    {
        this.reassemblySkippedDiscardedPackets = reassemblySkippedDiscardedPackets;
    }

    /**
     *
     * @return
     * The estimatedRTT
     */
    public Integer getEstimatedRTT()
    {
        return estimatedRTT;
    }

    /**
     *
     * @param estimatedRTT
     * The estimatedRTT
     */
    public void setEstimatedRTT (Integer estimatedRTT)
    {
        this.estimatedRTT = estimatedRTT;
    }

    /**
     *
     * @return
     * The unacknowledgedDataSize
     */
    public Integer getUnacknowledgedDataSize()
    {
        return unacknowledgedDataSize;
    }

    /**
     *
     * @param unacknowledgedDataSize
     * The unacknowledgedDataSize
     */
    public void setUnacknowledgedDataSize (Integer unacknowledgedDataSize)
    {
        this.unacknowledgedDataSize = unacknowledgedDataSize;
    }

    /**
     *
     * @return
     * The unacknowledgedQueueSize
     */
    public Integer getUnacknowledgedQueueSize()
    {
        return unacknowledgedQueueSize;
    }

    /**
     *
     * @param unacknowledgedQueueSize
     * The unacknowledgedQueueSize
     */
    public void setUnacknowledgedQueueSize (Integer unacknowledgedQueueSize)
    {
        this.unacknowledgedQueueSize = unacknowledgedQueueSize;
    }

    /**
     *
     * @return
     * The pendingDataSize
     */
    public Integer getPendingDataSize()
    {
        return pendingDataSize;
    }

    /**
     *
     * @param pendingDataSize
     * The pendingDataSize
     */
    public void setPendingDataSize (Integer pendingDataSize)
    {
        this.pendingDataSize = pendingDataSize;
    }

    /**
     *
     * @return
     * The pendingPacketQueueSize
     */
    public Integer getPendingPacketQueueSize()
    {
        return pendingPacketQueueSize;
    }

    /**
     *
     * @param pendingPacketQueueSize
     * The pendingPacketQueueSize
     */
    public void setPendingPacketQueueSize (Integer pendingPacketQueueSize)
    {
        this.pendingPacketQueueSize = pendingPacketQueueSize;
    }

    /**
     *
     * @return
     * The reliableSequencedDataSize
     */
    public Integer getReliableSequencedDataSize()
    {
        return reliableSequencedDataSize;
    }

    /**
     *
     * @param reliableSequencedDataSize
     * The reliableSequencedDataSize
     */
    public void setReliableSequencedDataSize (Integer reliableSequencedDataSize)
    {
        this.reliableSequencedDataSize = reliableSequencedDataSize;
    }

    /**
     *
     * @return
     * The reliableSequencedPacketQueueSize
     */
    public Integer getReliableSequencedPacketQueueSize()
    {
        return reliableSequencedPacketQueueSize;
    }

    /**
     *
     * @param reliableSequencedPacketQueueSize
     * The reliableSequencedPacketQueueSize
     */
    public void setReliableSequencedPacketQueueSize (Integer reliableSequencedPacketQueueSize)
    {
        this.reliableSequencedPacketQueueSize = reliableSequencedPacketQueueSize;
    }

    /**
     *
     * @return
     * The reliableUnsequencedDataSize
     */
    public Integer getReliableUnsequencedDataSize()
    {
        return reliableUnsequencedDataSize;
    }

    /**
     *
     * @param reliableUnsequencedDataSize
     * The reliableUnsequencedDataSize
     */
    public void setReliableUnsequencedDataSize (Integer reliableUnsequencedDataSize)
    {
        this.reliableUnsequencedDataSize = reliableUnsequencedDataSize;
    }

    /**
     *
     * @return
     * The reliableUnsequencedPacketQueueSize
     */
    public Integer getReliableUnsequencedPacketQueueSize()
    {
        return reliableUnsequencedPacketQueueSize;
    }

    /**
     *
     * @param reliableUnsequencedPacketQueueSize
     * The reliableUnsequencedPacketQueueSize
     */
    public void setReliableUnsequencedPacketQueueSize (Integer reliableUnsequencedPacketQueueSize)
    {
        this.reliableUnsequencedPacketQueueSize = reliableUnsequencedPacketQueueSize;
    }
}
