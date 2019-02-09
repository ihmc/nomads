/*
 *  Copyright 2004 The Apache Software Foundation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

package us.ihmc.aci.servicemanager.utils;

import org.apache.commons.collections.map.*;
import java.io.Serializable;
import java.util.*;

import org.apache.commons.collections.IterableMap;
import org.apache.commons.collections.MapIterator;
import org.apache.commons.collections.keyvalue.MultiKey;

public class MultiKeyMap2 extends MultiKeyMap {

    
    synchronized public boolean removeAll(Object key2) {
        return super.removeAll (key2);
    }
    
    synchronized public boolean removeAll2(Object key2) {
        boolean modified = false;
        MapIterator it = mapIterator();
        while (it.hasNext()) {
            MultiKey multi = (MultiKey) it.next();
            if (multi.size() >= 1 &&
                (key2 == null ? multi.getKey(1) == null : key2.equals(multi.getKey(1)))) {
                it.remove();
                modified = true;
            }
        }
        return modified;
    }

    synchronized public Collection getAll(Object key1) {
        MapIterator it = mapIterator();
        Vector v = new Vector();
        while (it.hasNext()) {
            MultiKey multi = (MultiKey) it.next();
            if (multi.size() >= 1 &&
                (key1 == null ? multi.getKey(0) == null : key1.equals(multi.getKey(0)))) {
                v.add(it.getValue());
            }
        }
        return v;
    }

    synchronized public Collection getAll2(Object key2) {
        MapIterator it = mapIterator();
        Vector v = new Vector();
        while (it.hasNext()) {
            MultiKey multi = (MultiKey) it.next();
            if (multi.size() >= 1 &&
                (key2 == null ? multi.getKey(1) == null : key2.equals(multi.getKey(1)))) {
                v.add(it.getValue());
            }
        }
        return v;
    }
    
    synchronized public Set entrySet() {
        return super.entrySet();
    }
    
    synchronized public Hashtable entrySet2(Object key2) {
        MapIterator it = mapIterator();
        Hashtable ht = new Hashtable();
        while (it.hasNext()) {
            MultiKey multi = (MultiKey) it.next();
            if (multi.size() >= 1 &&
                (key2 == null ? multi.getKey(1) == null : key2.equals(multi.getKey(1)))) {
                ht.put (multi.getKey (0), it.getValue());
            }
        }
        return ht;
    }
    
    synchronized public Object put (Object k1, Object k2, Object value){
        return super.put (k1, k2, value);
    }

    synchronized public Object get (Object k1, Object k2){
        return super.get (k1, k2);
    }
    
    synchronized public Object remove (Object k1, Object k2){
        return super.remove (k1, k2);
    }    
    
    synchronized public boolean contanisKey (Object k1, Object k2){
        return super.containsKey (k1, k2);
    }
}
