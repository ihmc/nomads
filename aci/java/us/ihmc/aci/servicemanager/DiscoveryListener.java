/*
 * DiscoveryListener.java
 *
 * Created on October 23, 2006, 11:48 AM
 *
 * To change this template, choose Tools | Template Manager
 * and open the template in the editor.
 */

package us.ihmc.aci.servicemanager;

import java.util.*;

/**
 * Callback interface for the ServiceManager
 *
 * @author sstabellini
 */
public interface DiscoveryListener
{

    /*
     * Receives new search results
     *
     * @param elementslist  a List of xml elements, each of these describes a service (commonInfo)
     * @param peerUUID  the peer that sent us the response
     *
     **/
    public void searchResultReceived (String peerUUID, List elementslist);
    
}
