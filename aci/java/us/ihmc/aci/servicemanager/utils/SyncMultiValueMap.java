/*
 * MultiValueMap.java
 *
 * Created on November 14, 2006, 6:05 PM
 *
 * To change this template, choose Tools | Template Manager
 * and open the template in the editor.
 */

package us.ihmc.aci.servicemanager.utils;

import java.util.*;

import org.apache.commons.collections.*;
import org.apache.commons.collections.map.MultiValueMap;

/**
 *
 * @author sstabellini
 */
public class SyncMultiValueMap extends MultiValueMap
{
    
    /** Creates a new instance of MultiValueMap */
    public SyncMultiValueMap ()
    {
        super(new HashMap(), new HashSetFactory());
    }
    
    synchronized public void clear(){
        super.clear ();
    }
    
    synchronized public boolean  containsValue(Object value){
        return super.containsValue (value);
    }

    synchronized public boolean containsValue(Object key, Object value) {
        return super.containsValue (key, value);
    }
    
    
    synchronized public Collection getCollection(Object key) {
        return super.getCollection (key);
    }
    
    synchronized public Iterator iterator(Object key) {
        return super.iterator (key);
    }

    synchronized public Object put(Object key, Object value){
        return super.put (key, value);
    }

    synchronized public void putAll(Map map){
        super.putAll (map);
    }
    
    synchronized public boolean putAll(Object key, Collection values){
        return super.putAll (key, values);
    }
    
    synchronized public Object remove(Object key, Object value){
        return super.remove (key, value);
    }
    
    synchronized public int size(Object key){
        return super.size (key);
    }
    
    synchronized public int totalSize(){
        return super.totalSize();
    }
    
    synchronized public Collection values(){
        return super.values();
    }
    
}
