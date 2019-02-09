/*
 * ServiceException.java
 *
 * Created on September 12, 2006, 3:13 PM
 *
 * To change this template, choose Tools | Template Manager
 * and open the template in the editor.
 */

package us.ihmc.aci.servicemanager;

/**
 *
 * @author sstabellini
 */
public class ServiceException extends Exception {
    
    /** Creates a new instance of ServiceException */
    public ServiceException() {
        super();
    }
    
    public ServiceException(String s) {
        super(s);
    }
    
    public ServiceException(Throwable t) {
        super(t);
    }
}
