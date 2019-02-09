package us.ihmc.aci.nodemon.data.process;


import org.apache.commons.lang3.builder.EqualsBuilder;
import org.apache.commons.lang3.builder.HashCodeBuilder;

/**
 * MocketsContent.java
 * <p>
 * Class <code>MocketsContent</code> is a container for statistics coming from IHMC's Mockets.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class MocketsContent extends ProcessContent
{
    public MocketsContent ()
    {
        PID = 0;

        //end point info message
        identifier = "";
        localAddr = "";
        localPort = 0;
        remoteAddr = "";
        remotePort = 0;
        //mockets link info message
        lastContactTime = "";
        sentBytes = 0;
        sentPackets = 0;
        retransmits = 0;
        receivedBytes = 0;
        receivedPackets = 0;
        duplicatedDiscardedPackets = 0;
        noRoomDiscardedPackets = 0;
        reassemblySkippedDiscardedPackets = 0;
        estimatedRTT = 0;
        unacknowledgedDataSize = 0;
        unacknowledgedQueueSize = 0;
        pendingDataSize = 0;
        pendingPacketQueueSize = 0;
        reliableSequencedDataSize = 0;
        reliableSequencedPacketQueueSize = 0;
        reliableUnsequencedDataSize = 0;
        reliableUnsequencedPacketQueueSize = 0;
    }

    @Override
    public boolean equals (Object o)
    {
        if (this == o) return true;

        if (o == null || getClass() != o.getClass()) return false;

        MocketsContent that = (MocketsContent) o;

        return new EqualsBuilder()
                .append(PID, that.PID)
                .append(localPort, that.localPort)
                .append(remotePort, that.remotePort)
                .append(sentBytes, that.sentBytes)
                .append(sentPackets, that.sentPackets)
                .append(retransmits, that.retransmits)
                .append(receivedBytes, that.receivedBytes)
                .append(receivedPackets, that.receivedPackets)
                .append(duplicatedDiscardedPackets, that.duplicatedDiscardedPackets)
                .append(noRoomDiscardedPackets, that.noRoomDiscardedPackets)
                .append(reassemblySkippedDiscardedPackets, that.reassemblySkippedDiscardedPackets)
                .append(estimatedRTT, that.estimatedRTT)
                .append(unacknowledgedDataSize, that.unacknowledgedDataSize)
                .append(unacknowledgedQueueSize, that.unacknowledgedQueueSize)
                .append(pendingDataSize, that.pendingDataSize)
                .append(pendingPacketQueueSize, that.pendingPacketQueueSize)
                .append(reliableSequencedDataSize, that.reliableSequencedDataSize)
                .append(reliableSequencedPacketQueueSize, that.reliableSequencedPacketQueueSize)
                .append(reliableUnsequencedDataSize, that.reliableUnsequencedDataSize)
                .append(reliableUnsequencedPacketQueueSize, that.reliableUnsequencedPacketQueueSize)
                .append(identifier, that.identifier)
                .append(localAddr, that.localAddr)
                .append(localAddrLong, that.localAddrLong)
                .append(remoteAddr, that.remoteAddr)
                .append(remoteAddrLong, that.remoteAddrLong)
                .append(lastContactTime, that.lastContactTime)
                .isEquals();
    }

    @Override
    public int hashCode ()
    {
        return new HashCodeBuilder(17, 37)
                .append(PID)
                .append(identifier)
                .append(localAddr)
                .append(localAddrLong)
                .append(localPort)
                .append(remoteAddr)
                .append(remoteAddrLong)
                .append(remotePort)
                .append(lastContactTime)
                .append(sentBytes)
                .append(sentPackets)
                .append(retransmits)
                .append(receivedBytes)
                .append(receivedPackets)
                .append(duplicatedDiscardedPackets)
                .append(noRoomDiscardedPackets)
                .append(reassemblySkippedDiscardedPackets)
                .append(estimatedRTT)
                .append(unacknowledgedDataSize)
                .append(unacknowledgedQueueSize)
                .append(pendingDataSize)
                .append(pendingPacketQueueSize)
                .append(reliableSequencedDataSize)
                .append(reliableSequencedPacketQueueSize)
                .append(reliableUnsequencedDataSize)
                .append(reliableUnsequencedPacketQueueSize)
                .toHashCode();
    }

    public long getPID ()
    {
        return PID;
    }

    public void setPID (long PID)
    {
        this.PID = PID;
    }

    public String getIdentifier ()
    {
        return identifier;
    }

    public void setIdentifier (String identifier)
    {
        this.identifier = identifier;
    }

    public String getLocalAddr ()
    {
        return localAddr;
    }

    public void setLocalAddr (String localAddr)
    {
        this.localAddr = localAddr;
    }

    public int getLocalPort ()
    {
        return localPort;
    }

    public void setLocalPort (int localPort)
    {
        this.localPort = localPort;
    }

    public String getRemoteAddr ()
    {
        return remoteAddr;
    }

    public void setRemoteAddr (String remoteAddr)
    {
        this.remoteAddr = remoteAddr;
    }

    public int getRemotePort ()
    {
        return remotePort;
    }

    public void setRemotePort (int remotePort)
    {
        this.remotePort = remotePort;
    }

    public String getLastContactTime ()
    {
        return lastContactTime;
    }

    public void setLastContactTime (String lastContactTime)
    {
        this.lastContactTime = lastContactTime;
    }

    public long getSentBytes ()
    {
        return sentBytes;
    }

    public void setSentBytes (long sentBytes)
    {
        this.sentBytes = sentBytes;
    }

    public long getSentPackets ()
    {
        return sentPackets;
    }

    public void setSentPackets (long sentPackets)
    {
        this.sentPackets = sentPackets;
    }

    public long getRetransmits ()
    {
        return retransmits;
    }

    public void setRetransmits (long retransmits)
    {
        this.retransmits = retransmits;
    }

    public long getReceivedBytes ()
    {
        return receivedBytes;
    }

    public void setReceivedBytes (long receivedBytes)
    {
        this.receivedBytes = receivedBytes;
    }

    public long getReceivedPackets ()
    {
        return receivedPackets;
    }

    public void setReceivedPackets (long receivedPackets)
    {
        this.receivedPackets = receivedPackets;
    }

    public long getDuplicatedDiscardedPackets ()
    {
        return duplicatedDiscardedPackets;
    }

    public void setDuplicatedDiscardedPackets (long duplicatedDiscardedPackets)
    {
        this.duplicatedDiscardedPackets = duplicatedDiscardedPackets;
    }

    public long getNoRoomDiscardedPackets ()
    {
        return noRoomDiscardedPackets;
    }

    public void setNoRoomDiscardedPackets (long noRoomDiscardedPackets)
    {
        this.noRoomDiscardedPackets = noRoomDiscardedPackets;
    }

    public long getReassemblySkippedDiscardedPackets ()
    {
        return reassemblySkippedDiscardedPackets;
    }

    public void setReassemblySkippedDiscardedPackets (long reassemblySkippedDiscardedPackets)
    {
        this.reassemblySkippedDiscardedPackets = reassemblySkippedDiscardedPackets;
    }

    public double getEstimatedRTT ()
    {
        return estimatedRTT;
    }

    public void setEstimatedRTT (double estimatedRTT)
    {
        this.estimatedRTT = estimatedRTT;
    }

    public long getUnacknowledgedDataSize ()
    {
        return unacknowledgedDataSize;
    }

    public void setUnacknowledgedDataSize (long unacknowledgedDataSize)
    {
        this.unacknowledgedDataSize = unacknowledgedDataSize;
    }

    public long getUnacknowledgedQueueSize ()
    {
        return unacknowledgedQueueSize;
    }

    public void setUnacknowledgedQueueSize (long unacknowledgedQueueSize)
    {
        this.unacknowledgedQueueSize = unacknowledgedQueueSize;
    }

    public long getPendingDataSize ()
    {
        return pendingDataSize;
    }

    public void setPendingDataSize (long pendingDataSize)
    {
        this.pendingDataSize = pendingDataSize;
    }

    public long getPendingPacketQueueSize ()
    {
        return pendingPacketQueueSize;
    }

    public void setPendingPacketQueueSize (long pendingPacketQueueSize)
    {
        this.pendingPacketQueueSize = pendingPacketQueueSize;
    }

    public long getReliableSequencedDataSize ()
    {
        return reliableSequencedDataSize;
    }

    public void setReliableSequencedDataSize (long reliableSequencedDataSize)
    {
        this.reliableSequencedDataSize = reliableSequencedDataSize;
    }

    public long getReliableSequencedPacketQueueSize ()
    {
        return reliableSequencedPacketQueueSize;
    }

    public void setReliableSequencedPacketQueueSize (long reliableSequencedPacketQueueSize)
    {
        this.reliableSequencedPacketQueueSize = reliableSequencedPacketQueueSize;
    }

    public long getReliableUnsequencedDataSize ()
    {
        return reliableUnsequencedDataSize;
    }

    public void setReliableUnsequencedDataSize (long reliableUnsequencedDataSize)
    {
        this.reliableUnsequencedDataSize = reliableUnsequencedDataSize;
    }

    public long getReliableUnsequencedPacketQueueSize ()
    {
        return reliableUnsequencedPacketQueueSize;
    }

    public void setReliableUnsequencedPacketQueueSize (long reliableUnsequencedPacketQueueSize)
    {
        this.reliableUnsequencedPacketQueueSize = reliableUnsequencedPacketQueueSize;
    }

    public long getLocalAddrLong ()
    {
        return localAddrLong;
    }

    public void setLocalAddrLong (long localAddrLong)
    {
        this.localAddrLong = localAddrLong;
    }

    public long getRemoteAddrLong ()
    {
        return remoteAddrLong;
    }

    public void setRemoteAddrLong (long remoteAddrLong)
    {
        this.remoteAddrLong = remoteAddrLong;
    }

    private long PID;
    //end point info message
    private String identifier;
    private String localAddr;
    private long localAddrLong;
    private int localPort;
    private String remoteAddr;
    private long remoteAddrLong;
    private int remotePort;
    //mockets link info message
    private String lastContactTime;
    private long sentBytes;
    private long sentPackets;
    private long retransmits;
    private long receivedBytes;
    private long receivedPackets;
    private long duplicatedDiscardedPackets;
    private long noRoomDiscardedPackets;
    private long reassemblySkippedDiscardedPackets;
    private double estimatedRTT;
    private long unacknowledgedDataSize;
    private long unacknowledgedQueueSize;
    private long pendingDataSize;
    private long pendingPacketQueueSize;
    private long reliableSequencedDataSize;
    private long reliableSequencedPacketQueueSize;
    private long reliableUnsequencedDataSize;
    private long reliableUnsequencedPacketQueueSize;
}
