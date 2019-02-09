package us.ihmc.gps.exception;

/*
 * LatLongFormatException.java
 * <p>
 * Description:  
 * LatLongFormatException if serial input does not have appropriate format.
 * </p>
 * Created on Dec 13, 2004
 * @author Christopher J. Eagle <ceagle@ihmc.us>
 * @version $Revision$
 * $Date$
 * <p>Copyright (c) Institute for Human and Machine Cognition (www.ihmc.us)<p>
 */

public class LatLongFormatException extends Exception 
{    
    /**
     * Prints out the StackTrace that caused the Exception.
     * <p>
     * @param e     Exception.
     */
    public LatLongFormatException (Exception e) 
    {
        if (e != null) {
            e.printStackTrace();
        }
    }    
}

