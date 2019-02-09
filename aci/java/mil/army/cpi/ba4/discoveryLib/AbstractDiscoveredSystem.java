package mil.army.cpi.ba4.discoveryLib;

import java.util.Comparator;

public abstract class AbstractDiscoveredSystem implements  Comparator<AbstractDiscoveredSystem>, Comparable {

    public String nodeUUID;
    public String nodeName;
    public int port;
    public int URN;
    public String[] groups;
    public AbstractService[] services;
    public AbstractNetwork[] networks;
    public AbstractIdentity identity;

    public byte areServicesPresent;
    public byte areNetworksPresent;


    public AbstractDiscoveredSystem() {

    }
    
    public AbstractDiscoveredSystem(AbstractDiscoveredSystem system) {
        this.nodeUUID = system.nodeUUID;
        this.nodeName = system.nodeName;
        this.port = system.port;
        this.URN = system.URN;
        this.groups = system.groups;
        this.services = system.services;
        this.networks = system.networks;
        this.identity = system.identity;
        this.areServicesPresent = system.areServicesPresent;
        this.areNetworksPresent = system.areNetworksPresent;
    }
    

    public void checkForServicesAndNetworks() {
        if (services == null) {
            areServicesPresent = 0;
        } else {
            areServicesPresent = 1;
        }
        if (networks == null) {
            areNetworksPresent = 0;
        } else {
            areNetworksPresent = 1;
        }
    }

    @Override
    public int compare(AbstractDiscoveredSystem p1, AbstractDiscoveredSystem p2) {
        String UUID1 = p1.nodeUUID;
        String UUID2 = p2.nodeUUID;

        if (UUID1.equals(UUID2)) {
            return 0;
        } else {
            return 1;
        }
    }

    @Override
    public int compareTo(Object another) {
        AbstractDiscoveredSystem discoveredSystem = (AbstractDiscoveredSystem) another;
        return compare(this, discoveredSystem);
    }

    @Override
    public boolean equals(Object o) {
        if (o instanceof AbstractDiscoveredSystem) {
            AbstractDiscoveredSystem discoveredSystem = (AbstractDiscoveredSystem) o;
            return compareTo(discoveredSystem) == 0;
        } else {
            return false;
        }
    }

    @Override
    public int hashCode() {
        int hash = 1;
        return hash;
    }
}
