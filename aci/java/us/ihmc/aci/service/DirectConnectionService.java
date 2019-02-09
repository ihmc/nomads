/**
 * DirectConnectionService
 * 
 * @author      Marco Arguedas      <marguedas@ihmc.us>
 * 
 * @version     $Revision$
 *              $Date$
 */

package us.ihmc.aci.service;

import java.io.InputStream;
import java.io.OutputStream;

/**
 * 
 */
public interface DirectConnectionService extends Service
{
    public void handleConnection (InputStream is, OutputStream os);
}

