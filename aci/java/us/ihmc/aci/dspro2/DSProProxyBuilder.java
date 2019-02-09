package us.ihmc.aci.dspro2;


public class DSProProxyBuilder
{
    int _port = DSProProxy.DIS_SVC_PROXY_SERVER_PORT_NUMBER;
    String _host = "127.0.0.1";

    short _applicationId = 20;
    String _applicationDescription = "";

    long _reinitializationAttemptInterval = 5000;

    @Deprecated
    public DSProProxyBuilder()
    {
    }

    public DSProProxyBuilder(short desiredApplicationId)
    {
        _applicationId = desiredApplicationId;
    }

    public DSProProxyBuilder setPort(int port)
    {
        _port = port;
        return this;
    }

    public DSProProxyBuilder setHost(String host)
    {
        _host = host;
        return this;
    }

    public DSProProxyBuilder setApplicationDescription(String appDescription)
    {
        _applicationDescription = appDescription;
        return this;
    }

    public DSProProxyBuilder setReinitializationAttemptInterval(long reinitializationAttemptInterval)
    {
        _reinitializationAttemptInterval = reinitializationAttemptInterval;
        return this;
    }

    public DSProProxy build()
    {
        return new DSProProxy (_applicationId, _applicationDescription, _host, _port, _reinitializationAttemptInterval);
    }

    public AsyncDSProProxy buildAsyncProxy()
    {
        return new AsyncDSProProxy(new QueuedDSProProxy(build()));
    }
}
