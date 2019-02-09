package mil.army.cpi.ba4.utils;

import java.util.ArrayList;
import java.util.List;

import mil.army.cpi.ba4.discoveryLib.AbstractIdentity;
import mil.army.cpi.ba4.discoveryLib.AbstractNetwork;
import mil.army.cpi.ba4.discoveryLib.AbstractService;

class Identity extends AbstractIdentity {
    
}

class Service extends AbstractService {
    
}

class Network extends AbstractNetwork {
    
}

public class Config {
    private Identity identity;
    private List<Service> services;
    private List<Network> networks;
    private String nodeName;
    private int port;
    private int URN;
    private List<String> groups;

    public Config() {
        this.services = new ArrayList<>();
        this.networks = new ArrayList<>();
        this.groups = new ArrayList<>();
    }

    public void setIdentity(int systemType, int platformType, int clazz, char agency, int ADCONURN, int OPCONURN, int ORGANICURN, int nationality, String symbolCode) {
        Identity identity = new Identity();
        identity.systemType = (byte) systemType;
        identity.platformType = platformType;
        identity.secClass = (byte) clazz;
        identity.agency = agency;
        identity.ADCONURN = ADCONURN;
        identity.OPCONURN = OPCONURN;
        identity.ORGANICURN = ORGANICURN;
        identity.nationality = nationality;
        identity.symbolCode = symbolCode;

        this.identity = identity;
    }

    public Identity getIdentity() {
        return this.identity;
    }

    public void addService(String serviceName, int serviceType, int servicePort) {
        Service service = new Service();
        service.serviceName = serviceName;
        service.serviceType = serviceType;
        service.port = servicePort;

        this.services.add(service);
    }

    public List<Service> getServices() {
        return services;
    }

    public void addNetwork(int networkType, int ipVersion, int priority, String ip) {
        Network network = new Network();
        network.networkType = networkType;
        network.priority = (byte) priority;
        network.ip = ip;

        this.networks.add(network);
    }

    public List<Network> getNetworks() {
        return this.networks;
    }

    public void setNodeName(String nodeName) {
        this.nodeName = nodeName;
    }

    public String getNodeName() {
        return this.nodeName;
    }

    public void setPort(int port) {
        this.port = port;
    }

    public int getPort() {
        return this.port;
    }

    public void setURN(int URN) {
        this.URN = URN;
    }

    public int getURN() {
        return this.URN;
    }

    public void setGroups(List<String> groups) {
        this.groups = groups;
    }

    public List<String> getGroups() {
        return this.groups;
    }
}
