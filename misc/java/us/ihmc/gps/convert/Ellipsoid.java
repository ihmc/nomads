package us.ihmc.gps.convert;

/**
 * Ellipsoid.java
 * <p>Description: </p>
 * Created on Sep 1, 2004
 * @author Christopher J. Eagle <ceagle@ihmc.us>
 * @version Revision: 1.0
 * Date: Sep 1, 2004
 * <p>Copyright (c) Institute for Human and Machine Cognition (www.ihmc.us)<p>
 */


public class Ellipsoid 
{
    public Ellipsoid()
    {
    }
    
    public Ellipsoid(int id, String name, double radius, double ecc)
    {
        _id = id;
        _name = name;
        _radius = radius;
        _eccentricitySquared = ecc;
    }    
    
    public int getID()
    {
        return _id;
    }
        
    public String getName()
    {
        return _name;
    }    
    
    public double getRadius()
    {
        return _radius;
    }
    
    public double getEccentricity()
    {
        return _eccentricitySquared;
    }
    
    private int _id;
    private double _radius;
    private double _eccentricitySquared;
    private String _name = "";
    
}//end of class Ellipsoid
