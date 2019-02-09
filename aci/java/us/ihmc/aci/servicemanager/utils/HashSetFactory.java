/*
 * HashsetFactory.java
 *
 * Created on November 14, 2006, 6:02 PM
 *
 * To change this template, choose Tools | Template Manager
 * and open the template in the editor.
 */

package us.ihmc.aci.servicemanager.utils;

import java.util.HashSet;
import org.apache.commons.collections.Factory;

/**
 *
 * @author sstabellini
 */
public class HashSetFactory implements Factory
{
    
    /** Creates a new instance of HashsetFactory */
    public HashSetFactory ()
    {
    }
    
    public Object create(){
        return new HashSet();
    }
    
}
