package DiscoveryService;

import mil.army.cpi.ba4.discoveryLib.AbstractDiscoveredSystem;


public class DiscoveryService extends AbstractDiscoveredSystem{
    private int URN;
    private long Timestamp;

    SelfIdentity identity;

    //SelfIdentity
    private int systemType;
    private int platformType;
    private int secClass;
    private int ADCONURN;
    private int OPCONURN;
    private int ORGANICURN;
    private char agency;
    private int nationality;
    private String symbolCode;

    //ServiceInfo services
    //Networks networks

    public DiscoveryService(){
        URN = 0;
        Timestamp = 0;
        identity = new SelfIdentity();
    }

    public void setURN(int URN){
        this.URN = URN;
        int rv = setURNNative(URN);
    }

    private native int setURNNative(int URN);

    public native int getPeerURN(String peerUUID);
    
    public native String addService(String serviceName, int serviceType, int port);
    
    public native String addNetwork(boolean gatewayFlag, boolean ipVersion, byte priority, byte networkType, long ip, int gatewayURN);
    
    public native int removeService(String serviceID);
    public native int removeNetwork(String networkID);
    
    private native int addSelfIdentityNative(byte systemType, byte platformType, 
            byte secClass, int ADCONURN, int OPCONURN, int ORGANICURN, byte agency, byte nationality, String symbolCode);

    public void updateSelfIdentity(){
//        addSelfIdentityNative(identity.getSystemType(), identity.getPlatformType(),
//                identity.getSecClass(), identity.getADCONURN(), identity.getOPCONURN(), identity.getORGANICURN(),
//                identity.getAgency(), identity.getNationality(), identity.getSymbolCode());

        addSelfIdentityNative((byte)systemType, (byte)platformType, (byte)secClass, ADCONURN, OPCONURN, ORGANICURN,
                (byte)agency, (byte)nationality, symbolCode);
    }

    public void addSelfIdentity(int systemType, int platformType, 
            int secClass, int ADCONURN, int OPCONURN, int ORGANICURN, byte agency, int nationality, String symbolCode){
//        identity.setSystemType(systemType);
//        identity.setPlatformType(platformType);
//        identity.setSecClass(secClass);
//        identity.setADCONURN(ADCONURN);
//        identity.setOPCONURN(OPCONURN);
//        identity.setORGANICURN(ORGANICURN);
//        identity.setAgency(agency);
//        identity.setNationality(nationality);
//        identity.setSymbolCode(symbolCode);

        this.systemType = systemType;
        this.platformType = platformType;
        this.secClass = secClass;
        this.ADCONURN = ADCONURN;
        this.OPCONURN = OPCONURN;
        this.ORGANICURN = ORGANICURN;
        this.agency = (char)agency;
        this.nationality = nationality;
        this.symbolCode = symbolCode;

        updateSelfIdentity();
    }

    public SelfIdentity getPeerSelfIdentity(String peerUUID){
        SelfIdentity identity = new SelfIdentity();
        try{
            identity = getPeerSelfIdentityNative(peerUUID, identity);
            return identity;
        }catch(NullPointerException e){
            System.out.println("SelfIdentity does not exist");
        }
        return null;
    }

    private native SelfIdentity getPeerSelfIdentityNative(String peerUUID, SelfIdentity selfIdentity);

    private native String[] getPeerServiceNames(String peerUUID);
    private native ServiceInfo getPeerServiceNative(String peerUUID, String serviceName, ServiceInfo service);

    public ServiceInfo[] getPeerServices(String peerUUID){
        String[] serviceNames = getPeerServiceNames(peerUUID);

        if(serviceNames != null){
            ServiceInfo[] services = new ServiceInfo[serviceNames.length];
            for(int i = 0; i < services.length; i++){
                ServiceInfo service = new ServiceInfo();
                service = getPeerServiceNative(peerUUID, serviceNames[i], service);
                services[i] = service;
            }
            return services;
        }
        else{
            return null;
        }
    }

    public ServiceInfo getService(String serviceName){
        ServiceInfo service = new ServiceInfo();
        return getServiceNative(serviceName, service);
    }

    private native ServiceInfo getServiceNative(String serviceName, ServiceInfo service);

    public NetworkInfo getNetwork(String networkName){
        NetworkInfo network = new NetworkInfo();
        return getNetworkNative(networkName, network);
    }
        
    private native NetworkInfo getNetworkNative(String serviceName, NetworkInfo service);

    private native String[] getPeerNetworkNames(String peerUUID);
    private native NetworkInfo getPeerNetworkNative(String peerUUID, String networksName, NetworkInfo network);
    //
    public NetworkInfo[] getPeerNetworks(String peerUUID){
        String[] networkNames = getPeerNetworkNames(peerUUID);

        if(networkNames != null){
            NetworkInfo[] networks = new NetworkInfo[networkNames.length];
            for(int i = 0; i < networks.length; i++){
                NetworkInfo network = new NetworkInfo();
                network = getPeerNetworkNative(peerUUID, networkNames[i], network);
                networks[i] = network;
            }
            return networks;
        }
        else{
            return null;
        }
    }

    public long getTimestamp() {
        return Timestamp;
    }

    public void setTimestamp(long Timestamp) {
        this.Timestamp = Timestamp;
    }

    public SelfIdentity getSelfIdentity(){
        return identity;
    }

    public native int setRetransmit(boolean retransmit);

    public native boolean getRetransmit();

    private long _discoveryService;

    static {
        System.loadLibrary("grpmgrjavawrapper");
    }
}