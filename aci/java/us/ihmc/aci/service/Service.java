/**
 * Service.java
 *
 * @author      Marco Arguedas      <marguedas@ihmc.us>
 *
 * @version     $Revision$
 *              $Date$
 */

package us.ihmc.aci.service;

import us.ihmc.aci.kernel.ServiceManager;

/**
 *
 */
public interface Service
{
    public void init (ServiceManager serviceManager);
}
