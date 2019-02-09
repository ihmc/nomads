package us.ihmc.gps.position;

/********************************************************************
 * The Coordinate class manages a coordinate in a grid indicating
 * a location interpretable by the robot.
 *
 * @author Ron van Hoof
 * @version 22 August 2003
 *
 ********************************************************************/

import java.io.Serializable;

public class Coordinate implements Serializable 
{
    public Coordinate() 
    {
    }

    public Coordinate(double x, double y, int unit) 
    {
        m_dX = x;
        m_dY = y;
        m_nUnit = unit;
    }

    public double getX() 
    {
        return m_dX;
    }

    public double getY() 
    {
        return m_dY;
    }

    public void setX(double x) 
    {
        m_dX = x;
    }

    public void setY(double y) 
    {
        m_dY = y;
    }

    public int getUnit() 
    {
        return m_nUnit;
    }

    public String toString() 
    {
        return "(" + getX() + ", " + getY() + ")";
    }


    // Attributes
    private double m_dX;
    private double m_dY;
    private int m_nUnit;

    // Constants  
    public final static int MILLIMETER = 0;
    public final static int CENTIMETER = 1;
    public final static int METER = 2;
    public final static int KILOMETER = 3;
    public final static int INCH = 4;
    public final static int FOOT = 5;
    public final static int MILE = 6;
    public final static int PIXEL = 7;

} // end of Class Coordinate