package us.ihmc.aci.envMonitor.provider.sim;

/**
 * Node
 * 
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$ Created on May 24, 2004 at 5:42:49 PM $Date: 
 *          2004/06/03 15:49:27 $ Copyright (c) 2004, The
 *          Institute for Human and Machine Cognition (www.ihmc.us)
 */
public abstract class Node
{
    public double getDistance (Node nd)
    {
        double distance = 0;
        distance = (nd.getX() - _xpos) * (nd.getX() - _xpos);
        distance = (nd.getY() - _ypos) * (nd.getY() - _ypos) + distance;
        distance = Math.sqrt (distance);
        return distance;
    }

    public abstract void reposition();

    public double getX()
    {
        return _xpos;
    }

    public double getY()
    {
        return _ypos;
    }

    protected double _xpos = 0;
    protected double _ypos = 0;
}
