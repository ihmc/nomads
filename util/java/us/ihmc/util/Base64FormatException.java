package us.ihmc.util;

/**
 * Base64FormatException.java
 * $Id: Base64FormatException.java,v 1.4 2013/03/06 19:18:12 nsuri Exp $
 * (c) COPYRIGHT MIT and INRIA, 1996.
 * Please first read the full copyright statement in file COPYRIGHT.html
 * Exception for invalid BASE64 streams.
 */

public class Base64FormatException extends Exception
{
    private static final long serialVersionUID = 4789905445828521382L;

    /**
     * Create that kind of exception
     * @param msg The associated error message 
     */
    public Base64FormatException (String msg)
	{
        super (msg);
    }
}
