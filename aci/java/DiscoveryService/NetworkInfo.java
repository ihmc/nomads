package DiscoveryService;

import mil.army.cpi.ba4.discoveryLib.AbstractNetwork;


public class NetworkInfo extends AbstractNetwork{
    private int networkType;
    private long ip;
    private byte priority;
    private int gatewayURN;

    private long _networkInfo;

    public int getNetworkType() {
        networkType = getNetworkTypeNative();
        return networkType;
    }

    private native int getNetworkTypeNative();

    public long getIP() {
        ip = getIPNative();
        return ip;
    }

    private native long getIPNative();

    public byte getPriority() {
        priority = getPriorityNative();
        return priority;
    }

    private native byte getPriorityNative();

    public int getGatewayURN() {
        gatewayURN = getGatewayURNNative();
        return gatewayURN;
    }

    private native int getGatewayURNNative();
}
