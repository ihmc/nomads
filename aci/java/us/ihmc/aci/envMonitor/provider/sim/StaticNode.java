package us.ihmc.aci.envMonitor.provider.sim;

/**
 * StaticNode
 * 
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$ Created on May 24, 2004 at 3:36:48 PM $Date$ Copyright (c) 2004, The
 *          Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class StaticNode extends Node
{
    public StaticNode (double xpos, double ypos)
    {
         _xpos = xpos;
         _ypos = ypos;
    }

    public void reposition()
    {
    }
}
