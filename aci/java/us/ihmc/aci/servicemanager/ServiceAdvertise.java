/*
 * ServiceAdvertise.java
 *
 * Created on October 24, 2006, 10:33 AM
 *
 * To change this template, choose Tools | Template Manager
 * and open the template in the editor.
 */

package us.ihmc.aci.servicemanager;

import us.ihmc.aci.grpMgrOld.*;
import us.ihmc.util.xml.XmlProcessor;

import org.dom4j.*;

import java.util.*;
import java.io.*;

/**
 *
 * @author sstabellini
 */
public class ServiceAdvertise implements Runnable
{
    
    /** Creates a new instance of ServiceAdvertise */
    public ServiceAdvertise (XmlProcessor doc, GroupManager gm, String groupname, int maxAdvertises, short hopCount)
    {
        this.hopCount = hopCount;
        this.doc = doc;
        this.gm = gm;
        flag = true;
        this.groupname = groupname;
        advertisewaittime = 15000;
        this.maxAdvertises = maxAdvertises;
    }
    
    public void run(){
        
        while (true){         
                    
            synchronized (this){
                if (!flag || maxAdvertises == 0) {
                    break;
                }                
                try
                {

                    gm.updatePeerGroupData(groupname, doc.getDocument().asXML().getBytes(), hopCount, gm.getPingFloodProbability());
                } catch (GroupManagerException ex)
                {
                    ex.printStackTrace();
                }                
                
                if (maxAdvertises > 0){
                    maxAdvertises--;
                }
                
                doc.removeNodeUnderRoot("serviceInfo[@remove = \"true\"]");
            }
            
            try
            {
                        
                Thread.sleep (advertisewaittime);
            } catch (InterruptedException ex)
            {
                ex.printStackTrace();
            }
        }
    }

    /*
     * Stops this advertisement thread
     */
    synchronized public void stop(){
        flag = false;
    }
    
    /*
     * Returns the sleep time between advertisements
     */
    public long getAdvertisewaittime ()
    {
        return advertisewaittime;
    }

    /*
     * Sets the sleep time between advertisements
     */
    public void setAdvertisewaittime (long sleep)
    {
        this.advertisewaittime = sleep;
    }

    /*
     * Returns the maximum number of advertisements to make
     */
    synchronized public int getMaxAdvertises()
    {
        return maxAdvertises;
    }

    /*
     * Sets the maximum number of advertisements to make
     *
     */
    synchronized public void setMaxAdvertises (int maxAdvertises)
    {
        this.maxAdvertises = maxAdvertises;
    }
    
    /*
     * adds a service to remove
     *
     * @param e an xml Element that describes the services.
     */
    synchronized public void removeService (String uuid){
        List l = doc.findElementValues ("/services/serviceInfo[serviceUId = \"" + uuid + "\"]");
        if (l.size() == 1) {
            Element e = (Element) l.get (0);
            e.addAttribute ("remove", "true");
        }
        if (maxAdvertises == 0) {
            maxAdvertises = 1;
        }
    }

    /*
     * removes all the services
     *
     * @param e an xml Element that describes the services.
     */
    synchronized public void removeAll(){
        Element root = doc.getRootElement();
        Iterator it = root.elementIterator();
        while (it.hasNext()) {
            Element e = (Element) it.next();
            e.addAttribute ("remove", "true");
        }
        maxAdvertises = 1;
    }    
    
    synchronized public XmlProcessor getXmlProcessor() {
        return doc;
    }
    
    protected boolean flag;
    protected GroupManager gm;
    protected String groupname;
    protected long advertisewaittime;
    protected int maxAdvertises;
    protected short hopCount;
    protected XmlProcessor doc;
}
