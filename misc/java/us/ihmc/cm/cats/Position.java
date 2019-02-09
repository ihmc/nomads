package us.ihmc.cm.cats;

import java.io.Serializable;

public class Position implements Serializable, Cloneable
{
    public Position()
    {
        _x = 0;
        _y = 0;
        _z = 0;
    }

    public Position (double x, double y, double z)
    {
        _x = x;
        _y = y;
        _z = z;
    }

    public double getX()
    {
        return _x;
    }
    
    public double getY()
    {
        return _y;
    }
    
    public double getZ()
    {
        return _z;
    }

    public void setX (double x)
    {
        _x = x;
    }
    
    public void setY (double y)
    {
        _y = y;
    }
    
    public void setZ (double z)
    {
        _z = z;
    }

    public String toString()
    {
        return new String ("(" + _x + ", " + _y +  ", " + _z + ")");
    }

    protected double _x;
    protected double _y;
    protected double _z;
}