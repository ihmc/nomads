/**
 * MocketStatusListener
 *
 * @version     $Revision: 1.1 $
 *              $Date: 2004/12/10 23:10:44 $
 */
package us.ihmc.mockets;

/**
 * MocketStatusListener interface is used by the Mocket to periodically notify
 * the listeners about the mocket status.
 *
 */
public interface MocketStatusListener
{
    public boolean peerUnreachableWarning (long timeSinceLastContact);
}