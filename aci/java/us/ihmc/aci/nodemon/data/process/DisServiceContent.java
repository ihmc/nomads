package us.ihmc.aci.nodemon.data.process;


import org.apache.commons.lang3.builder.EqualsBuilder;
import org.apache.commons.lang3.builder.HashCodeBuilder;

/**
 * DisServiceContent.java
 * <p/>
 * Class <code>DisServiceContent</code> is a container for statistics coming from IHMC's DisService.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class DisServiceContent extends ProcessContent
{
    public DisServiceContent ()
    {
        dataMessagesReceived = 0;
        dataBytesReceived = 0;
        dataFragmentsReceived = 0;
        dataFragmentBytesReceived = 0;
        missingFragmentRequestMessagesSent = 0;
        missingFragmentRequestBytesSent = 0;
        missingFragmentRequestMessagesReceived = 0;
        missingFragmentRequestBytesReceived = 0;
        dataCacheQueryMessagesSent = 0;
        dataCacheQueryBytesSent = 0;
        dataCacheQueryMessagesReceived = 0;
        dataCacheQueryBytesReceived = 0;
        topologyStateMessagesSent = 0;
        topologyStateBytesSent = 0;
        topologyStateMessagesReceived = 0;
        topologyStateBytesReceived = 0;
        keepAliveMessagesSent = 0;
        keepAliveMessagesReceived = 0;
        queryMessagesSent = 0;
        queryMessagesReceived = 0;
        queryHitsMessagesSent = 0;
        queryHitsMessagesReceived = 0;
    }

    @Override
    public boolean equals (Object o)
    {
        if (this == o) return true;

        if (o == null || getClass() != o.getClass()) return false;

        DisServiceContent that = (DisServiceContent) o;

        return new EqualsBuilder()
                .append(dataMessagesReceived, that.dataMessagesReceived)
                .append(dataBytesReceived, that.dataBytesReceived)
                .append(dataFragmentsReceived, that.dataFragmentsReceived)
                .append(dataFragmentBytesReceived, that.dataFragmentBytesReceived)
                .append(missingFragmentRequestMessagesSent, that.missingFragmentRequestMessagesSent)
                .append(missingFragmentRequestBytesSent, that.missingFragmentRequestBytesSent)
                .append(missingFragmentRequestMessagesReceived, that.missingFragmentRequestMessagesReceived)
                .append(missingFragmentRequestBytesReceived, that.missingFragmentRequestBytesReceived)
                .append(dataCacheQueryMessagesSent, that.dataCacheQueryMessagesSent)
                .append(dataCacheQueryBytesSent, that.dataCacheQueryBytesSent)
                .append(dataCacheQueryMessagesReceived, that.dataCacheQueryMessagesReceived)
                .append(dataCacheQueryBytesReceived, that.dataCacheQueryBytesReceived)
                .append(topologyStateMessagesSent, that.topologyStateMessagesSent)
                .append(topologyStateBytesSent, that.topologyStateBytesSent)
                .append(topologyStateMessagesReceived, that.topologyStateMessagesReceived)
                .append(topologyStateBytesReceived, that.topologyStateBytesReceived)
                .append(keepAliveMessagesSent, that.keepAliveMessagesSent)
                .append(keepAliveMessagesReceived, that.keepAliveMessagesReceived)
                .append(queryMessagesSent, that.queryMessagesSent)
                .append(queryMessagesReceived, that.queryMessagesReceived)
                .append(queryHitsMessagesSent, that.queryHitsMessagesSent)
                .append(queryHitsMessagesReceived, that.queryHitsMessagesReceived)
                .isEquals();
    }

    @Override
    public int hashCode ()
    {
        return new HashCodeBuilder(17, 37)
                .append(dataMessagesReceived)
                .append(dataBytesReceived)
                .append(dataFragmentsReceived)
                .append(dataFragmentBytesReceived)
                .append(missingFragmentRequestMessagesSent)
                .append(missingFragmentRequestBytesSent)
                .append(missingFragmentRequestMessagesReceived)
                .append(missingFragmentRequestBytesReceived)
                .append(dataCacheQueryMessagesSent)
                .append(dataCacheQueryBytesSent)
                .append(dataCacheQueryMessagesReceived)
                .append(dataCacheQueryBytesReceived)
                .append(topologyStateMessagesSent)
                .append(topologyStateBytesSent)
                .append(topologyStateMessagesReceived)
                .append(topologyStateBytesReceived)
                .append(keepAliveMessagesSent)
                .append(keepAliveMessagesReceived)
                .append(queryMessagesSent)
                .append(queryMessagesReceived)
                .append(queryHitsMessagesSent)
                .append(queryHitsMessagesReceived)
                .toHashCode();
    }

    @Override
    public String toString ()
    {
        return "DisServiceContent{" +
                "dataMessagesReceived=" + dataMessagesReceived +
                ", dataBytesReceived=" + dataBytesReceived +
                ", dataFragmentsReceived=" + dataFragmentsReceived +
                ", dataFragmentBytesReceived=" + dataFragmentBytesReceived +
                ", missingFragmentRequestMessagesSent=" + missingFragmentRequestMessagesSent +
                ", missingFragmentRequestBytesSent=" + missingFragmentRequestBytesSent +
                ", missingFragmentRequestMessagesReceived=" + missingFragmentRequestMessagesReceived +
                ", missingFragmentRequestBytesReceived=" + missingFragmentRequestBytesReceived +
                ", dataCacheQueryMessagesSent=" + dataCacheQueryMessagesSent +
                ", dataCacheQueryBytesSent=" + dataCacheQueryBytesSent +
                ", dataCacheQueryMessagesReceived=" + dataCacheQueryMessagesReceived +
                ", dataCacheQueryBytesReceived=" + dataCacheQueryBytesReceived +
                ", topologyStateMessagesSent=" + topologyStateMessagesSent +
                ", topologyStateBytesSent=" + topologyStateBytesSent +
                ", topologyStateMessagesReceived=" + topologyStateMessagesReceived +
                ", topologyStateBytesReceived=" + topologyStateBytesReceived +
                ", keepAliveMessagesSent=" + keepAliveMessagesSent +
                ", keepAliveMessagesReceived=" + keepAliveMessagesReceived +
                ", queryMessagesSent=" + queryMessagesSent +
                ", queryMessagesReceived=" + queryMessagesReceived +
                ", queryHitsMessagesSent=" + queryHitsMessagesSent +
                ", queryHitsMessagesReceived=" + queryHitsMessagesReceived +
                '}';
    }

    public long getDataMessagesReceived ()
    {
        return dataMessagesReceived;
    }

    public void setDataMessagesReceived (long dataMessagesReceived)
    {
        this.dataMessagesReceived = dataMessagesReceived;
    }

    public long getDataBytesReceived ()
    {
        return dataBytesReceived;
    }

    public void setDataBytesReceived (long dataBytesReceived)
    {
        this.dataBytesReceived = dataBytesReceived;
    }

    public long getDataFragmentsReceived ()
    {
        return dataFragmentsReceived;
    }

    public void setDataFragmentsReceived (long dataFragmentsReceived)
    {
        this.dataFragmentsReceived = dataFragmentsReceived;
    }

    public long getDataFragmentBytesReceived ()
    {
        return dataFragmentBytesReceived;
    }

    public void setDataFragmentBytesReceived (long dataFragmentBytesReceived)
    {
        this.dataFragmentBytesReceived = dataFragmentBytesReceived;
    }

    public long getMissingFragmentRequestMessagesSent ()
    {
        return missingFragmentRequestMessagesSent;
    }

    public void setMissingFragmentRequestMessagesSent (long missingFragmentRequestMessagesSent)
    {
        this.missingFragmentRequestMessagesSent = missingFragmentRequestMessagesSent;
    }

    public long getMissingFragmentRequestBytesSent ()
    {
        return missingFragmentRequestBytesSent;
    }

    public void setMissingFragmentRequestBytesSent (long missingFragmentRequestBytesSent)
    {
        this.missingFragmentRequestBytesSent = missingFragmentRequestBytesSent;
    }

    public long getMissingFragmentRequestMessagesReceived ()
    {
        return missingFragmentRequestMessagesReceived;
    }

    public void setMissingFragmentRequestMessagesReceived (long missingFragmentRequestMessagesReceived)
    {
        this.missingFragmentRequestMessagesReceived = missingFragmentRequestMessagesReceived;
    }

    public long getMissingFragmentRequestBytesReceived ()
    {
        return missingFragmentRequestBytesReceived;
    }

    public void setMissingFragmentRequestBytesReceived (long missingFragmentRequestBytesReceived)
    {
        this.missingFragmentRequestBytesReceived = missingFragmentRequestBytesReceived;
    }

    public long getDataCacheQueryMessagesSent ()
    {
        return dataCacheQueryMessagesSent;
    }

    public void setDataCacheQueryMessagesSent (long dataCacheQueryMessagesSent)
    {
        this.dataCacheQueryMessagesSent = dataCacheQueryMessagesSent;
    }

    public long getDataCacheQueryBytesSent ()
    {
        return dataCacheQueryBytesSent;
    }

    public void setDataCacheQueryBytesSent (long dataCacheQueryBytesSent)
    {
        this.dataCacheQueryBytesSent = dataCacheQueryBytesSent;
    }

    public long getDataCacheQueryMessagesReceived ()
    {
        return dataCacheQueryMessagesReceived;
    }

    public void setDataCacheQueryMessagesReceived (long dataCacheQueryMessagesReceived)
    {
        this.dataCacheQueryMessagesReceived = dataCacheQueryMessagesReceived;
    }

    public long getDataCacheQueryBytesReceived ()
    {
        return dataCacheQueryBytesReceived;
    }

    public void setDataCacheQueryBytesReceived (long dataCacheQueryBytesReceived)
    {
        this.dataCacheQueryBytesReceived = dataCacheQueryBytesReceived;
    }

    public long getTopologyStateMessagesSent ()
    {
        return topologyStateMessagesSent;
    }

    public void setTopologyStateMessagesSent (long topologyStateMessagesSent)
    {
        this.topologyStateMessagesSent = topologyStateMessagesSent;
    }

    public long getTopologyStateBytesSent ()
    {
        return topologyStateBytesSent;
    }

    public void setTopologyStateBytesSent (long topologyStateBytesSent)
    {
        this.topologyStateBytesSent = topologyStateBytesSent;
    }

    public long getTopologyStateMessagesReceived ()
    {
        return topologyStateMessagesReceived;
    }

    public void setTopologyStateMessagesReceived (long topologyStateMessagesReceived)
    {
        this.topologyStateMessagesReceived = topologyStateMessagesReceived;
    }

    public long getTopologyStateBytesReceived ()
    {
        return topologyStateBytesReceived;
    }

    public void setTopologyStateBytesReceived (long topologyStateBytesReceived)
    {
        this.topologyStateBytesReceived = topologyStateBytesReceived;
    }

    public long getKeepAliveMessagesSent ()
    {
        return keepAliveMessagesSent;
    }

    public void setKeepAliveMessagesSent (long keepAliveMessagesSent)
    {
        this.keepAliveMessagesSent = keepAliveMessagesSent;
    }

    public long getKeepAliveMessagesReceived ()
    {
        return keepAliveMessagesReceived;
    }

    public void setKeepAliveMessagesReceived (long keepAliveMessagesReceived)
    {
        this.keepAliveMessagesReceived = keepAliveMessagesReceived;
    }

    public long getQueryMessagesSent ()
    {
        return queryMessagesSent;
    }

    public void setQueryMessagesSent (long queryMessagesSent)
    {
        this.queryMessagesSent = queryMessagesSent;
    }

    public long getQueryMessagesReceived ()
    {
        return queryMessagesReceived;
    }

    public void setQueryMessagesReceived (long queryMessagesReceived)
    {
        this.queryMessagesReceived = queryMessagesReceived;
    }

    public long getQueryHitsMessagesSent ()
    {
        return queryHitsMessagesSent;
    }

    public void setQueryHitsMessagesSent (long queryHitsMessagesSent)
    {
        this.queryHitsMessagesSent = queryHitsMessagesSent;
    }

    public long getQueryHitsMessagesReceived ()
    {
        return queryHitsMessagesReceived;
    }

    public void setQueryHitsMessagesReceived (long queryHitsMessagesReceived)
    {
        this.queryHitsMessagesReceived = queryHitsMessagesReceived;
    }

    private long dataMessagesReceived;
    private long dataBytesReceived;
    private long dataFragmentsReceived;
    private long dataFragmentBytesReceived;
    private long missingFragmentRequestMessagesSent;
    private long missingFragmentRequestBytesSent;
    private long missingFragmentRequestMessagesReceived;
    private long missingFragmentRequestBytesReceived;
    private long dataCacheQueryMessagesSent;
    private long dataCacheQueryBytesSent;
    private long dataCacheQueryMessagesReceived;
    private long dataCacheQueryBytesReceived;
    private long topologyStateMessagesSent;
    private long topologyStateBytesSent;
    private long topologyStateMessagesReceived;
    private long topologyStateBytesReceived;
    private long keepAliveMessagesSent;
    private long keepAliveMessagesReceived;
    private long queryMessagesSent;
    private long queryMessagesReceived;
    private long queryHitsMessagesSent;
    private long queryHitsMessagesReceived;
}
