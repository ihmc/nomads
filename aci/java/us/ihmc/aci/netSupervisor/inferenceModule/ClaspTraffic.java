package us.ihmc.aci.netSupervisor.inferenceModule;

import com.google.protobuf.Duration;
import us.ihmc.aci.ddam.MicroFlow;

public class ClaspTraffic
{

    private int _trafficId;
    private String _destinationHostIp;
    private String _sourceHostIp;
    private String _destinationHostPort;
    private String _sourceHostPort;
    private String _protocol;
    private int _priorityValue;

    private long _duration;

    public ClaspTraffic ()
    {

    }

    public ClaspTraffic (MicroFlow microFlow)
    {
        _trafficId = microFlow.getId();
        _sourceHostIp = microFlow.getIpSrc();
        _destinationHostIp = microFlow.getIpDst();
        _sourceHostPort = microFlow.getPortSrc();
        _destinationHostPort = microFlow.getPortDst();
        _protocol = microFlow.getProtocol();
        _priorityValue = microFlow.getDSCP();
        if (microFlow.getDuration().isInitialized()) {
            _duration = microFlow.getDuration().getSeconds();
        }
        else {
            _duration = Long.MAX_VALUE;
        }
    }

    public ClaspTraffic (int trafficId, String sourceHostIp, String sourceHostPort, String destinationHostIp,
                         String destinationHostPort, String protocol, int priorityValue)
    {
        _trafficId = trafficId;
        _destinationHostIp = destinationHostIp;
        _sourceHostIp = sourceHostIp;
        _destinationHostPort = destinationHostPort;
        _sourceHostPort = sourceHostPort;
        _protocol = protocol;
        _priorityValue = priorityValue;
        _duration = Long.MAX_VALUE;
    }

    public String getDestinationHostIp ()
    {
        return _destinationHostIp;
    }

    public String getSourceHostIp ()
    {
        return _sourceHostIp;
    }

    public String getDestinationHostPort ()
    {
        return _destinationHostPort;
    }

    public String getSourceHostPort ()
    {
        return _sourceHostPort;
    }

    public int getPriorityValue ()
    {
        return _priorityValue;
    }

    public String getProtocol ()
    {
        return _protocol;
    }

    public int getTrafficId()
    {
        return _trafficId;
    }

    public long getDuration()
    {
        return _duration;
    }

    public MicroFlow getMicroFlowFromClaspTraffic ()
    {
        MicroFlow.Builder microFlowBuilder = MicroFlow.newBuilder();

        microFlowBuilder.setId(_trafficId);
        microFlowBuilder.setIpSrc(_sourceHostIp);
        microFlowBuilder.setIpDst(_destinationHostIp);
        microFlowBuilder.setPortSrc(_sourceHostPort);
        microFlowBuilder.setPortDst(_destinationHostPort);
        microFlowBuilder.setProtocol(_protocol);
        microFlowBuilder.setDSCP(_priorityValue);
        microFlowBuilder.setDuration(Duration.newBuilder().setSeconds(_duration));

        return microFlowBuilder.build();
    }

    public String print ()
    {
        return "(" + _trafficId + "," + _sourceHostIp + "," + _sourceHostPort + "," + _destinationHostIp + "," +
                _destinationHostPort + "," + _protocol + "," + _priorityValue + "," +  _duration + ")";
    }

    public void setDestinationHostIp (String destinationHostIp)
    {
        _destinationHostIp = destinationHostIp;
    }

    public void setSourceHostIp (String sourceHostIp)
    {
        _sourceHostIp = sourceHostIp;
    }

    public void setDestinationHostPort (String destinationHostPort)
    {
        _destinationHostPort = destinationHostPort;
    }

    public void setSourceHostPort (String sourceHostPort)
    {
        _sourceHostPort = sourceHostPort;
    }

    public void setProtocol (String protocol)
    {
        _protocol = protocol;
    }

    public void setPriorityValue (int priorityValue)
    {
        _priorityValue = priorityValue;
    }

    public void setTrafficId(int trafficId)
    {
        _trafficId = trafficId;
    }

    public void setDuration(long duration)
    {
        _duration = duration;
    }
}
