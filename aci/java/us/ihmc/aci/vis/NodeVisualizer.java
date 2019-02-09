package us.ihmc.aci.vis;

import us.ihmc.aci.grpMgrOld.GroupManager;

/**
 * Node Visualizer
 *
 * Created on June 24, 2007, 10:00 PM
 * 
 * @author nsuri
 */
public class NodeVisualizer
{
    public static void main (String[] args)
        throws Exception
    {
        GroupManager gm = new GroupManager (2015);
        gm.start();
        NodeStateMonitor nsm = new NodeStateMonitor (gm, "ihmc.aci");
        gm.setListener (nsm);
        gm.createPublicPeerGroup ("ihmc.aci", null);
    }
}
