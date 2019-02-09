/*
 * SyncTreeMap.java
 *
 * Created on October 24, 2006, 11:22 AM
 *
 * To change this template, choose Tools | Template Manager
 * and open the template in the editor.
 */

package us.ihmc.aci.servicemanager.utils;

import java.util.*;

/**
 *
 * @author sstabellini
 */
public class SyncTreeMap extends TreeMap
{
    
    /** Creates a new instance of SyncTreeMap */
    public SyncTreeMap ()
    {
    }
    
    synchronized public SortedMap headMap(Object o) {
        return super.headMap (o);
    }
    
    synchronized public Object put(Object k,  Object o) {
        return super.put (k, o);
    }
    
    synchronized public Object remove(Object k) {
        return super.remove (k);
    }
    
}
