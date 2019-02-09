/*
 * SearchService.java
 *
 * Created on October 24, 2006, 11:25 AM
 *
 * To change this template, choose Tools | Template Manager
 * and open the template in the editor.
 */

package us.ihmc.aci.servicemanager;


import us.ihmc.aci.grpMgrOld.*;

import org.dom4j.*;

import java.util.*;
import java.io.IOException;

/**
 *
 * @author sstabellini
 */
public class SearchService implements Runnable
{
    
    /** Creates a new instance of SearchService */
    public SearchService (Document doc, short maxHopCount, GroupManager gm, String groupname)
    {
        this.doc = doc;
        this.maxHopCount = maxHopCount;
        this.groupname = groupname;
        this.gm = gm;
        querysearchwaittime = 20000;       
        flag = true;
        hopcount = 1;
        hopinc = 2;
    }
    
    public void run(){

        while (true){

            synchronized (this) {
                if ( !flag || hopcount >= maxHopCount) {
                    break;
                }
            }

            try {
                gm.startPeerSearch (groupname, doc.asXML().getBytes(), hopcount, gm.getPeerSearchFloodProbability(), 0);
            }
            catch (GroupManagerException gme) {
                gme.printStackTrace();
            }

            synchronized (this)
            {
                hopcount = (short) (hopcount + hopinc);
            }

            try
            {
                Thread.sleep (querysearchwaittime);
            } catch (InterruptedException ex)
            {
                ex.printStackTrace();
            }
        }
    }

    /**
     * Returns the sleep time between searches
     */
    public long getQuerysearchwaittime ()
    {
        return querysearchwaittime;
    }

    /**
     * set the sleep time between the searches
     * @param querysearchwaittime wait time in milliseconds
     */
    public void setQuerysearchwaittime (long querysearchwaittime)
    {
        this.querysearchwaittime = querysearchwaittime;
    }

    /**
     * Returns the number of hops that is added to the expanding ring algorithm's hop count at each iteration
     */
    synchronized public short getHopinc()
    {
        return hopinc;
    }

    /**
     * Sets the number of hops that is added to the expanding ring algorithm's hop count at each iteration
     * @param hopinc the number of hops
     */
    synchronized public void setHopinc (short hopinc)
    {
        this.hopinc = hopinc;
    }
    
    /**
     * Resets the hop count of the expanding ring algorithm
     */
    synchronized public void resetHopCount(){
        hopcount = 1;
    }

    
    /**
     * Stops the Search Thread
     */
    synchronized public void stop(){
        flag = false;
    }

    protected Document doc;
    protected long querysearchwaittime;
    protected String groupname;
    protected boolean flag;
    protected GroupManager gm;
    protected short maxHopCount;
    protected short hopinc, hopcount;
}
