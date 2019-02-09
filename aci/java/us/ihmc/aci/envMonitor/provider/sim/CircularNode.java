package us.ihmc.aci.envMonitor.provider.sim;

import java.util.Random;

/**
 * CircularNode
 * 
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$ Created on May 24, 2004 at 3:36:48 PM $Date$ Copyright (c) 2004, The
 *          Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class CircularNode extends Node
{
    public CircularNode (double radius, double speed, double xCenter, double yCenter, boolean clockwise)
    {
        _rand = new Random();
        _xcenter = xCenter;
        _ycenter = yCenter;
        _radius = Math.abs(radius);
        _nodeID = "CN" + _rand.nextLong();
        if (_radius == 0) {
            _radius = 1;
        }
        _direction = clockwise;
        reposition();
    }

    public String getID()
    {
        return (_nodeID);
    }

    public void reposition()
    {
        _prevArc = _prevArc + .1*(_direction? 1 : -1);
//        System.out.println (_prevArc + "Sin(" + Math.sin(_prevArc) + "), Cos(" + Math.cos(_prevArc) + ")");
        if (_prevArc > (6.2832 * _radius)) {
            _prevArc = 0;
        }
        _xpos = Math.cos(_prevArc) * _radius + _xcenter;
        if (_xpos < 0) {
            _xpos = 0;
        }
        if (_xpos > 1000) {
            _xpos = 1000;
        }

        _ypos = Math.sin(_prevArc) * _radius + _ycenter;
        if (_ypos < 0) {
            _ypos = 0;
        }
        if (_ypos > 1000) {
            _ypos = 1000;
        }
    }

    private Random _rand;
    private double _radius;
    private double _prevArc = 0;
    private double _xcenter = 0;
    private double _ycenter = 0;
    private String _nodeID;
    private boolean _direction = true;
}
