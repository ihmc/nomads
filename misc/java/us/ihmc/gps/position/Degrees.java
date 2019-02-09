package us.ihmc.gps.position;

/**
 * Degrees.java
 * <p>Description: 
 * </p>
 * Created on Sep 1, 2004
 * @author Christopher J. Eagle <ceagle@ihmc.us>
 * @version Revision: 1.0
 * $Date$
 * <p>Copyright (c) Institute for Human and Machine Cognition (www.ihmc.us)</p>
 */

import java.text.NumberFormat;


public class Degrees
{
    public Degrees ()
    {
    }

	/**
     * Construct a postition from minutes/seconds.  Direction is N, S, E, W.
     * <p>
     * @param minutes       
     * @param seconds
     */ 
    public Degrees (String direction, int degrees, int minutes, double seconds)
    {
        _degrees = degrees;            
        _minutes = minutes;
        _seconds = seconds;
        
        this.convertToDecimalFormat(degrees, minutes, seconds);
        this.setDirection(direction);
    }

    
    /**
     * Construct a postition from decimal degrees DDD.DDDDD to DDD MM" SS'
     * Must use in conjunction with setDirectionString (String pos, String neg)
     * <p>
     * @param degrees  -- degree decimal form     
     */ 
    public Degrees (double degrees)
    {
        N.setMaximumFractionDigits (3);
        N.setMinimumFractionDigits (3);
        
        _decDegrees = degrees;            
        _degrees = (int)degrees;
        
        _decMinutes = (degrees - (double)_degrees) * 60.0d;
        _decMinutes  = Double.parseDouble(N.format(_decMinutes));
        _minutes = (int) _decMinutes;
        _seconds = ( _decMinutes - (double)_minutes ) * 60;
        
        if (_degrees < 0) {
            _degrees *= -1;
        }            

        if (_seconds < 0) {
            _seconds *= -1;
        }
        
        if (_minutes < 0) {
            _minutes *= -1;
            isNegative = true;
        }        
    }
    
    
    /**
     * Construct a postition from degrees decimalMinutes  DDD mm.mmm to 
     * DDD MM" SS' 
     * <p>
     * @param degrees  -- degree decimal form     
     */ 
    public Degrees (double degrees, double minutes, String bearing)
    {
        _degrees = (int) degrees; 
                   
        _decMinutes = minutes;
        _minutes = (int) minutes;
                
        _seconds = ( _decMinutes - (double)_minutes ) * 60;

        this.convertToDecimalFormat(_degrees, _minutes, _seconds);
        this.setDirection(bearing);    
    }
        
        
    /**
     * Get Direction String
     * <p>
     * @return direction string (N/S/E/W)
     */ 
    public String getDirectionString()
    {
        return _direction;
    }    
            
    public int getDegrees()
    {
        return _degrees;
    }
    
    public int getMinutes()
    {
        return _minutes;
    }    
    
    public double getSeconds()
    {
        return _seconds;
    }
    
    /**
     * Returns Decimal Minutes in form "mm.mmmm".  Can make a call to 
     * getDegrees() for "DDD" and concat DDDmm.mmm for decimal Degrees form.
     * <p> 
     * @return
     */
    public double getDecimalMinutes()
    {
        double decMinutes = _decMinutes;  
                  
        if (this.getDirection()){
            decMinutes = _decMinutes * -1;            
        }
                
        return decMinutes;            
    }
    
    
    /**
     * Returns Decimal Degrees form "ddd.ddddd".
     * <p>
     * @return
     */
    public double getDecimalDegrees()
    {
        double decDegrees = _decDegrees;
        
		if (this.getDirection()){
            decDegrees = _decDegrees * -1;            
        }
                
        return decDegrees;
    }


    /**
     * Set Direction "N", "S", "E", "W"
     * <p> 
     * @param pos
     * @param neg
     */
    protected void setDirectionString (String pos, String neg)
    {
        if (_decDegrees < 0.0d) {
            _direction = neg;
            this.setDirection(neg);
            _decDegrees *= -1;
            _decMinutes *= -1;
        }
        else if (_decDegrees > 0.0d) {
            _direction = pos;
            this.setDirection(pos);
        }
    }
    

    /**
     * A circle has 360 degrees, 60 minutes per degree, and 60 seconds per 
     * minute.  There are 3,600 seconds per degree. 
     * <p> 
     * @param degrees
     * @param minutes
     * @param seconds
     */     
    protected void convertToDecimalFormat(int degrees, int minutes, double seconds)
    {
        _decMinutes = (double)minutes + (seconds / 60.0d);
        _decDegrees = degrees +((minutes * 60 + seconds)/3600.0d);
    }


    /**
     * Get Direction
     * <p>
     * @return  true if value should be negative (S or W)
     *			false if value should be positive (N or E)
     */ 
    protected boolean getDirection()
    {
        return isNegative;
    }
    
    
    /**
     * Set Direction and negative or positive value for this position
     * N and E is positive
     * S and W is negative
     */ 
    protected void setDirection(String dir)
    {
        char selection = dir.charAt(0);
        _direction = "" + selection;
        
        switch(selection){
        case 'N':
        case 'n':  isNegative = false;  
				   break;
        case 'S':
        case 's': isNegative = true;  
				   break;
        case 'E':
        case 'e':  isNegative = false; 
				   break;
        case 'W':
        case 'w':  isNegative = true;
				   break;
        default:
            System.out.println("Direction error occured");
        }
    }
    
    /**
     * Return Position as a string in direction DD MM SS.ss.
     */ 
    public String toString()
    {
        return (_direction + " " + _degrees + " " + _minutes + " " + N.format(_seconds));
    }


	//used for DD MM SS.s
    private int _degrees = 0;
    private int _minutes = 0;
    private double _seconds = 0;

    //used for DD MM.mmm
	private double _decMinutes = 0.0d;

	//used for DD.ddddd
    private double _decDegrees = 0.0d;
	
	//used for N, S, E, W
    private String _direction = "";
	
	//mark N and E as + and S and W as -
    private boolean isNegative = false;
	
	//limits precision of doubles for the DD MM format.
    private NumberFormat N = NumberFormat.getNumberInstance();
    
}// end of class Degrees
