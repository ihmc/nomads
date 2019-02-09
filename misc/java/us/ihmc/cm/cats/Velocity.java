package us.ihmc.cm.cats;

import java.io.Serializable;

public class Velocity implements Serializable, Cloneable
{
    public Velocity()
    {
        _dx = 0;
        _dy = 0;
        _dz = 0;
    }

    public Velocity (double dx, double dy, double dz)
    {
        _dx = dx;
        _dy = dy;
        _dz = dz;
    }

    public double getDX()
    {
        return _dx;
    }
    
    public double getDY()
    {
        return _dy;
    }
    
    public double getDZ()
    {
        return _dz;
    }

    public void setDX (double dx)
    {
        _dx = dx;
    }
    
    public void setDY (double dy)
    {
        _dy = dy;
    }
    
    public void setDZ (double dz)
    {
        _dz = dz;
    }

    public String toString()
    {
        return new String ("(" + _dx + ", " + _dy +  ", " + _dz + ")");
    }

    protected double _dx;
    protected double _dy;
    protected double _dz;
}