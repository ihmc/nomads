/*
 * ServiceManager.java
 *
 * Created on 22 October 2006, 17:36
 *
 * To change this template, choose Tools | Template Manager
 * and open the template in the editor.
 */
package us.ihmc.aci.servicemanager;

import us.ihmc.aci.grpMgrOld.*;

import us.ihmc.util.IDGenerator;

import us.ihmc.util.xml.XmlProcessor;

import org.dom4j.*;

import java.io.*;
import java.util.*;

/**
 *
 * @author sstabellini
 */
public class ServiceManager implements GroupManagerListener, Runnable {

    /*
     * Creates a new ServiceManager instance and a new GroupManager instance, then starts the GroupManager.
     *
     * @param nodename  the nodename that will be set in the GroupManager
     */
    public ServiceManager(String nodename) throws GroupManagerException {
        services = new XmlProcessor("<services></services>");
        advertisewaittime = 15000;
        querysearchwaittime = 20000;
        cacheexpiretime = 20000;
        hopinc = 2;
        searches = new Hashtable();
        advertisements = new Hashtable();
        ;
        advertisedservices = new Hashtable();
        serviceuuid2sa = new Hashtable();
        expirecaching = new Hashtable();
        gm = new GroupManager();
        gm.setListener(this);
        gm.setNodeName(nodename);
        gm.start();
        Thread expireoldservices = new Thread(this);
        expireoldservices.start();
        lock = new Object();
    }

    /*
     * Creates a new ServiceManager using the GroupManager instance passed as an argument.
     * The GroupManager should be already correctly initialized and started.
     *
     * @param gm the GroupManager instance that will be used by the ServiceManager; it must be already initialized and started.
     */
    public ServiceManager(GroupManager gm) {
        services = new XmlProcessor("<services></services>");
        advertisewaittime = 15000;
        querysearchwaittime = 20000;
        cacheexpiretime = 20000;
        hopinc = 2;
        searches = new Hashtable();
        advertisements = new Hashtable();
        ;
        advertisedservices = new Hashtable();
        serviceuuid2sa = new Hashtable();
        expirecaching = new Hashtable();
        this.gm = gm;
        Thread expireoldservices = new Thread(this);
        expireoldservices.start();
        lock = new Object();
    }

    public String getGroupname() {
        return groupname;
    }

    public void setGroupname(String groupname) {
        this.groupname = groupname;
    }

    public GroupManager getGroupManager() {
        return gm;
    }

    /*
     * Enables/Disables the Group Manager PING/INFO packets:
     * If the node advertisement is disabled
     * PING/INFO packet are not sent in a regular basis, and the GroupManager is used
     * only for peer searches.
     * 
     * @param enabled
     * 
     */
    public void setNodeAdvertisement(boolean enabled) {
        gm.setNodeAdvertisement(enabled);
    }

    /*
     * Creates a public PeerGroup and sets it as the default group for the ServiceManager
     *
     * @param groupname the name of the group to create
     *
     */
    public void createPublicPeerGroup(String groupname) throws GroupManagerException {
        gm.createPublicPeerGroup(groupname, null);
        this.groupname = groupname;
    }

    /*
     * Creates a private peergroup and sets it as the default group for the ServiceManager
     *
     * @param groupname the name of the group to create
     * @param password the password of the group to create
     *
     */
    public void createPrivatePeerGroup(String groupname, String password) throws GroupManagerException {
        gm.createPrivatePeerGroup(groupname, password, null);
        this.groupname = groupname;
    }

    /*
     * Returns the XmlProcessor that contains the xml document used to store all the published and cached service descriptors
     */
    public XmlProcessor getServicesAsXml() {
        return services;
    }

    /*
     * Returns in a string the XML document containing all the published and cached service descriptors
     */
    public String getServicesAsString() {
        XmlProcessor xp = this.getServicesAsXml();
        Document doc = xp.getDocument();
        return xp.pretty(doc);
    }

    /*
     * Publishes a new service
     *
     * @param e xml element that describes a service (serviceInfo). It must contain the serviceUId.
     *
     */
    synchronized public void publishService(Element e) throws ServiceException {
        if (!e.getName().equals("serviceInfo")) {
            throw new ServiceException("Malformed Service Descriptor");
        }
        Element uuid = e.element("serviceUId");
        if (uuid == null) {
            throw new ServiceException("Malformed Service Descriptor");
        }
        services.insertElementUnderRoot(e, 0);
    }

    /*
     * Publishes a new service
     *
     * @param doc an xml Document that describes services. The root element must be services, children must all be serviceInfo elements.
     *
     */
    synchronized public void publishService(Document doc) throws ServiceException {
        Element root = doc.getRootElement();
        if (!root.getName().equals("services")) {
            throw new ServiceException("Malformed Service Descriptors Document");
        }
        List l = root.elements();
        Iterator it = l.iterator();
        while (it.hasNext()) {
            Element e = (Element) it.next();
            if (!e.getName().equals("serviceInfo")) {
                throw new ServiceException("Malformed Service Descriptor");
            }
            Element uuid = e.element("serviceUId");
            if (uuid == null) {
                throw new ServiceException("Malformed Service Descriptor");
            }
            services.insertElementUnderRoot(e, 0);
        }
    }

    /*
     * Removes a published service
     *
     * @param uuid  the serviceUId of the service to remove
     */
    synchronized public void removeService(String uuid) throws GroupManagerException {
        Integer type = (Integer) advertisedservices.get(uuid);
        if (type != null) {
            advertisedservices.remove(uuid);
            if (type.equals(ADVERTISING)) {
                ServiceAdvertise sa = (ServiceAdvertise) serviceuuid2sa.get(uuid);
                sa.removeService(uuid);
                serviceuuid2sa.remove(uuid);

            } else if (type.equals(ADVERTISED)) {
                List l = services.findElementValues("/services/serviceInfo[serviceUId = \"" + uuid + "\"]");
                if (l.size() == 1) {
                    ByteArrayOutputStream baos = new ByteArrayOutputStream();
                    PrintWriter pw = new PrintWriter(new OutputStreamWriter(baos));
                    pw.println("<services>");
                    Element e = (Element) l.get(0);
                    e.addAttribute("remove", "true");
                    pw.println(e.asXML());
                    pw.println("</services>");
                    pw.flush();
                    try {
                        gm.updatePeerGroupData(groupname, baos.toByteArray(), gm.getPingHopCount(), gm.getPingFloodProbability());
                        baos.close();
                    } catch (IOException ex) {
                        ex.printStackTrace();
                    }
                }
            }
        }
        //services.removeNodeUnderRoot ("serviceInfo[serviceUId = \"" + uuid + "\"]");
        //mmarcon: this is a workaround to prevent the problems due to the removeNodeUnderRoot call
        List l = services.findElementValues("/services/serviceInfo[serviceUId != \"" + uuid + "\"]");
        services = new XmlProcessor("<services></services>");//.removeNodeUnderRoot("serviceInfo[serviceUId = \"" + uuid + "\"]");
        if (!l.isEmpty()) {
            Iterator i = l.iterator();
            while (i.hasNext()) {
                services.insertElementUnderRoot((Element) i.next(), 0);
            }
        }
    }

    /*
     * Removes a published service
     *
     * @param e xml element that describes the service to remove (serviceInfo). It must contain the serviceUId.
     *
     */
    synchronized public void removeService(Element e) throws ServiceException, GroupManagerException {
        if (!e.getName().equals("serviceInfo")) {
            throw new ServiceException("Malformed Service Descriptor");
        }
        Element uuid = e.element("serviceUId");
        if (uuid == null) {
            throw new ServiceException("Malformed Service Descriptor");
        }
        removeService(uuid.getText());
    }

    /*
     * Removes published services
     *
     * @param auuid an array of serviceUId corresponding to the services to remove
     */
    synchronized public void removeServices(String[] auuid) throws GroupManagerException {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        PrintWriter pw = new PrintWriter(new OutputStreamWriter(baos));
        pw.println("<services>");
        boolean flag = false;
        for (int i = 0; i < auuid.length; i++) {
            Integer type = (Integer) advertisedservices.get(auuid[i]);
            if (type != null) {
                advertisedservices.remove(auuid[i]);
                if (type.equals(ADVERTISING)) {
                    ServiceAdvertise sa = (ServiceAdvertise) serviceuuid2sa.get(auuid[i]);
                    sa.removeService(auuid[i]);
                    serviceuuid2sa.remove(auuid[i]);

                } else if (type.equals(ADVERTISED)) {
                    List l = services.findElementValues("/services/serviceInfo[serviceUId = \"" + auuid[i] + "\"]");
                    if (l.size() == 1) {
                        Element e = (Element) l.get(0);
                        e.addAttribute("remove", "true");
                        pw.println(e.asXML());
                        flag = true;
                    }
                }
            }
            //services.removeNodeUnderRoot ("serviceInfo[serviceUId = \"" + auuid[i] + "\"]");
            List l = services.findElementValues("/services/serviceInfo[serviceUId != \"" + auuid[i] + "\"]");
            services = new XmlProcessor("<services></services>");//.removeNodeUnderRoot("serviceInfo[serviceUId = \"" + uuid + "\"]");
            if (!l.isEmpty()) {
                Iterator iter = l.iterator();
                while (iter.hasNext()) {
                    services.insertElementUnderRoot((Element) iter.next(), 0);
                }
            }
        }
        if (flag) {
            pw.println("</services>");
            pw.flush();
            gm.updatePeerGroupData(groupname, baos.toByteArray(), gm.getPingHopCount(), gm.getPingFloodProbability());
        }
        try {
            baos.close();
        } catch (IOException ex) {
            ex.printStackTrace();
        }
    }

    /*
     * Removes published services
     *
     * @param ae  an array of xml elements, each of these describes a service to remove (serviceInfo). They must contain the serviceUId.
     */
    synchronized public void removeServices(Element[] ae) throws ServiceException, GroupManagerException {
        String[] as = new String[ae.length];
        for (int i = 0; i < ae.length; i++) {
            if (!ae[i].getName().equals("serviceInfo")) {
                throw new ServiceException("Malformed Service Descriptor");
            }
            Element e = ae[i].element("serviceUId");
            if (e == null) {
                throw new ServiceException("Malformed Service Descriptor");
            }
            as[i] = e.getText();
        }
        removeServices(as);
    }

    /*
     * Removes published services
     *
     * @param doc an xml Document that describes services to remove. The root element must be services, children must all be serviceInfo elements.
     */
    synchronized public void removeServices(Document doc) throws ServiceException, GroupManagerException {
        Element root = doc.getRootElement();
        if (!root.getName().equals("services")) {
            throw new ServiceException("Malformed Service Descriptors Document");
        }
        List l = root.elements();
        String[] as = new String[l.size()];
        Iterator it = l.iterator();
        int i = 0;
        while (it.hasNext()) {
            Element e = (Element) it.next();
            if (!e.getName().equals("serviceInfo")) {
                throw new ServiceException("Malformed Service Descriptor");
            }
            Element uuid = e.element("serviceUId");
            if (uuid == null) {
                throw new ServiceException("Malformed Service Descriptor");
            }
            as[i] = uuid.getText();
            i++;
        }
        removeServices(as);
    }


    /*
     * Checks if a service is already published
     *
     * @param uuid  the serviceUId of the service
     */
    synchronized public boolean isPublished(String uuid) {
        return services.exists("/services/serviceInfo[serviceUId = \"" + uuid + "\"]");
    }

    /*
     * Checks if a service is already published
     *
     * @param e  xml element that describes the service (serviceInfo). It must contain the serviceUId.
     */
    synchronized public boolean isPublished(Element e) throws ServiceException {
        if (!e.getName().equals("serviceInfo")) {
            throw new ServiceException("Malformed Service Descriptor");
        }
        Element uuid = e.element("serviceUId");
        if (uuid == null) {
            throw new ServiceException("Malformed Service Descriptor");
        }
        return isPublished(uuid.getText());
    }

    /*
     * Starts to advertise the local published services on the network
     *
     * @param aserviceUId an array of uuid of the published services to advertise
     * @param maxAdvertises maximum number of times you want to advertise the services
     * @param hopCount the hop count
     *
     * @return the advertisementUUID
     */
    synchronized public String startServiceAdvertise(String[] aserviceUId, int maxAdvertises, short hopCount) throws ServiceException {
        if (groupname == null) {
            throw new ServiceException("groupname not set");
        }
        if (gm == null) {
            throw new ServiceException("GroupManager not set");
        }
        XmlProcessor xp = new XmlProcessor("<services></services>");
        ServiceAdvertise sa = new ServiceAdvertise(xp, gm, groupname, maxAdvertises, hopCount);
        sa.setAdvertisewaittime(advertisewaittime);
        boolean flag = false;
        for (int i = 0; i < aserviceUId.length; i++) {
            List l = services.findElementValues("/services/serviceInfo[serviceUId = " + aserviceUId[i] + "]");
            if (l.size() == 1) {
                Element e = (Element) l.get(0);
                xp.insertElementUnderRoot(e, 0);
                advertisedservices.put(aserviceUId[i], ADVERTISING);
                serviceuuid2sa.put(aserviceUId[i], sa);
                flag = true;
            }
        }
        if (flag) {
            Thread t = new Thread(sa);
            t.start();
            String uuid = IDGenerator.generateID();
            advertisements.put(uuid, sa);
            return uuid;
        } else {
            return null;
        }
    }

    /*
     * Starts to advertise the local published services on the network
     *
     * @param xpathquery an xpath query to select local services to adverise
     * @param maxAdvertises maximum number of times you want to advertise the services
     * @param hopCount the hop count
     *
     * @return the advertisementUUID
     */
    synchronized public String startServiceAdvertise(String xpathquery, int maxAdvertises, short hopCount) throws ServiceException {
        if (groupname == null) {
            throw new ServiceException("groupname not set");
        }
        if (gm == null) {
            throw new ServiceException("GroupManager not set");
        }

        List l = services.findElementValues(xpathquery);
        if (l.size() > 0) {
            XmlProcessor xp = new XmlProcessor("<services></services>");
            ServiceAdvertise sa = new ServiceAdvertise(xp, gm, groupname, maxAdvertises, hopCount);
            sa.setAdvertisewaittime(advertisewaittime);
            Iterator it = l.iterator();
            while (it.hasNext()) {
                Element e = (Element) it.next();
                xp.insertElementUnderRoot(e, 0);
                Element uuid = e.element("serviceUId");
                advertisedservices.put(uuid.getText(), ADVERTISING);
                serviceuuid2sa.put(uuid.getText(), sa);
            }
            Thread t = new Thread(sa);
            t.start();
            String uuid = IDGenerator.generateID();
            advertisements.put(uuid, sa);
            return uuid;
        } else {
            return null;
        }
    }

    /*
     * Stops an advertisement
     *
     * @param UUID          the advertisementUUID as returned by startServiceAdvertise
     *
     */
    synchronized public void stopServiceAdvertise(String uuid) {
        ServiceAdvertise sa = (ServiceAdvertise) advertisements.get(uuid);
        if (sa != null) {
            sa.stop();
            advertisements.remove(uuid);
            XmlProcessor xp = sa.getXmlProcessor();
            List l = xp.findElementValues("/services/serviceInfo/serviceUId");
            Iterator it = l.iterator();
            while (it.hasNext()) {
                Element e = (Element) it.next();
                advertisedservices.put(e.getText(), ADVERTISED);
                serviceuuid2sa.remove(e.getText());
            }
        }
    }


    /*
     * Starts to search for services on the network.
     * The search is done using an expanding ring algorithm.
     *
     * @param query an xpath query
     * @param maxHopCount the maximum diameter of the expanding ring in terms of hops
     *
     * @return the searchUUID; if the maxHopCount is zero just queries the cache and returns null.
     *
     */
    synchronized public String serviceSearchExpandingRing(String query, short maxHopCount) throws ServiceException {
        if (groupname == null) {
            throw new ServiceException("groupname not set");
        }
        if (gm == null) {
            throw new ServiceException("GroupManager not set");
        }

        List l = services.findElementValues(query);
        if (l.size() > 0) {
            dl.searchResultReceived(gm.getNodeUUID(), l);
        }

        if (maxHopCount > 0) {
            XmlProcessor xp = new XmlProcessor("<services></services>");
            xp.createTextElement("/services/xpathQuery", query);
            SearchService ss = new SearchService(xp.getDocument(), maxHopCount, gm, groupname);
            ss.setQuerysearchwaittime(querysearchwaittime);
            ss.setHopinc(hopinc);
            Thread t = new Thread(ss);
            t.start();
            String uuid = IDGenerator.generateID();
            searches.put(uuid, ss);
            return uuid;
        } else {
            return null;
        }

    }

    /*
     * Starts to search for services on the network.
     * The search is done using the persistent search mechanism provided by the GroupManager
     *
     * @param query             an xpath query
     * @param hopCount          specifies the number of hops (retransmissions) that must be performed for 
     *                          the search request
     * @param ttl               the time to live of the peer search
     *
     * @return                   the searchUUID
     *
     */
    synchronized public String serviceSearch(String query, short hopCount, int ttl) throws ServiceException {
        if (groupname == null) {
            throw new ServiceException("groupname not set");
        }
        if (gm == null) {
            throw new ServiceException("GroupManager not set");
        }

        List l = services.findElementValues(query);
        if (l.size() > 0) {
            dl.searchResultReceived(gm.getNodeUUID(), l);
        }

        if (hopCount > 0) {
            XmlProcessor xp = new XmlProcessor("<services></services>");
            xp.createTextElement("/services/xpathQuery", query);
            try {
                return gm.startPeerSearch(groupname, xp.getDocument().asXML().getBytes(), hopCount, gm.getPeerSearchFloodProbability(), ttl);
            } catch (GroupManagerException gme) {
                gme.printStackTrace();
                return null;
            }
        } else {
            return null;
        }
    }

    /*
     * Stops a search
     *
     * @param UUID          the searchUUID as returned by serviceSearchPersistent
     *
     */
    synchronized void stopServiceSearch(String uuid) throws ServiceException {
        if (groupname == null) {
            throw new ServiceException("groupname not set");
        }
        if (gm == null) {
            throw new ServiceException("GroupManager not set");
        }
        SearchService ss = (SearchService) searches.get(uuid);
        if (ss != null) {
            ss.stop();
            searches.remove(uuid);
        } else {
            gm.stopPersistentPeerSearch(uuid);
        }
    }

    public void newPeer(String uuid) {
    }

    public void groupListChange(String uuid) {
    }

    public void newGroupMember(String groupName, String memberUUID, byte[] data) {
    }

    public void conflictWithPrivatePeerGroup(String groupName, String peerUUID) {
    }

    public void peerGroupDataChanged(String groupName, String peerUUID, byte[] data) {
        XmlProcessor xp = new XmlProcessor(new String(data));
        List l = xp.findElementValues("/services/serviceInfo");
        Iterator it = l.iterator();
        while (it.hasNext()) {
            Element e = (Element) it.next();
            Attribute at = e.attribute("remove");
            String uuid = e.element("serviceUId").getText();
            synchronized (lock) {
                services.removeNodeUnderRoot("serviceInfo[serviceUId = \"" + uuid + "\"]");
                if (at == null || !at.getValue().equals("true")) {
                    services.insertElementUnderRoot(e, 0);
                    expirecaching.put(uuid, new Long(System.currentTimeMillis()));
                }
                else {
                    expirecaching.remove(uuid);
                }
            }
        }
    }

    public void peerSearchRequestReceived(String groupName, String nodeUUID, String searchUUID, byte[] param) {
        XmlProcessor xpin = new XmlProcessor(new String(param));
        List l = xpin.findElementValues("/services/xpathQuery");
        Element e = null;
        if (l.size() > 0) {
            e = (Element) l.get(0);
            List lresult = null;
            synchronized (services) {
                lresult = services.findElementValues(e.getText());
            }
            if (lresult.size() > 0) {
                Iterator it = lresult.iterator();
                XmlProcessor xpout = new XmlProcessor("<services></services>");
                while (it.hasNext()) {
                    e = (Element) it.next();
                    xpout.insertElementUnderRoot(e, 0);
                }
                try {
                    gm.respondToPeerSearch(searchUUID, xpout.getDocument().asXML().getBytes());
                } catch (GroupManagerException gme) {
                    gme.printStackTrace();
                }
            }
        }
    }

    public void peerSearchResultReceived(String groupName, String nodeUUID, String searchUUID, byte[] param) {
        XmlProcessor xp = new XmlProcessor(new String(param));
        List l = xp.findElementValues("/services/serviceInfo");
        if (l.size() > 0) {
            dl.searchResultReceived(nodeUUID, l);
        }
    }

    public void peerMessageReceived(String groupName, String nodeUUID, byte[] data) {
    }

    public void persistentPeerSearchTerminated(String groupName, String nodeUUID, String searchUUI) {
    }

    synchronized public void deadPeer(String uuid) {
//        services.removeNodeUnderRoot ("serviceInfo[nodeUId = \"" + uuid + "\"]");
    }

    synchronized public void groupMemberLeft(String groupName, String memberUUID) {
//        services.removeNodeUnderRoot ("serviceInfo[nodeUId = \"" + memberUUID + "\"]");
    }

    /*
     * Returns the ServiceManager's listener
     */
    public DiscoveryListener getDiscoveryListener() {
        return dl;
    }

    /*
     * Sets the ServiceManager's listener
     */
    public void setDiscoveryListener(DiscoveryListener dl) {
        this.dl = dl;
    }

    /*
     * Returns the sleep time between advertisements
     */
    public long getAdvertisewaittime() {
        return advertisewaittime;
    }

    /*
     * Sets the sleep time between advertisements
     */
    public void setAdvertisewaittime(long advertisewaittime) {
        this.advertisewaittime = advertisewaittime;
    }

    /*
     * Returns the sleep time between searches made with the expanding ring algorithm
     */
    public long getQuerysearchwaittime() {
        return querysearchwaittime;
    }

    /*
     * Sets the sleep time between searches made with the expanding ring algorithm
     */
    public void setQuerysearchwaittime(long querysearchwaittime) {
        this.querysearchwaittime = querysearchwaittime;
    }

    /*
     * Gets the hop incrementation in each iteration of the expanding ring algorithm
     */
    public short getHopinc() {
        return hopinc;
    }

    /*
     * Sets the hop incrementation in each iteration of the expanding ring algorithm
     */
    public void setHopinc(short hopinc) {
        this.hopinc = hopinc;
    }

    public long getCacheexpiretime() {
        return cacheexpiretime;
    }

    public void setCacheexpiretime(long cacheexpiretime) {
        this.cacheexpiretime = cacheexpiretime;
    }

    public void run() {
        while (true) {
            long ctime = System.currentTimeMillis();
            Set s = expirecaching.entrySet();
            Iterator it = s.iterator();
            while (it.hasNext()) {
                Map.Entry me = (Map.Entry) it.next();
                String uuid = (String) me.getKey();
                Long timestamp = (Long) me.getValue();
                if (timestamp.longValue() < ctime - cacheexpiretime) {
                    services.removeNodeUnderRoot("serviceInfo[serviceUId = \"" + uuid + "\"]");
                    it.remove();
                }
            }
            try {
                Thread.sleep(getCacheexpiretime());
            } catch (InterruptedException ex) {
                ex.printStackTrace();
            }
        }
    }
    protected XmlProcessor services;
    protected GroupManager gm;
    protected long advertisewaittime,  querysearchwaittime,  cacheexpiretime;
    protected DiscoveryListener dl;
    protected String groupname;
    protected Hashtable searches;
    protected Hashtable advertisements;
    protected short hopinc;
    protected Hashtable advertisedservices;
    protected Hashtable serviceuuid2sa;
    protected Hashtable expirecaching;
    private Object lock;
    public static final Integer ADVERTISED = new Integer(0);
    public static final Integer ADVERTISING = new Integer(1);
}
