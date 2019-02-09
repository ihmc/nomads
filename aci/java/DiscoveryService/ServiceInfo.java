package DiscoveryService;

import mil.army.cpi.ba4.discoveryLib.AbstractService;


public class ServiceInfo extends AbstractService{
    private String serviceName;
    private int serviceType;
    private int port;

    private long _serviceInfo;

    public String getServiceName() {
        serviceName = getServiceNameNative();
        return serviceName;
    }

    public native String getServiceNameNative();

    public int getServiceType() {
        serviceType = getServiceTypeNative();
        return serviceType;
    }

    private native int getServiceTypeNative();

    public int getPort() {
        port = getPortNative();
        return port;
    }

    private native int getPortNative();
}
