package us.ihmc.media.util;

public class InvalidOperationException extends Exception
{
    public InvalidOperationException()
    {
    }
    
    public InvalidOperationException (String msg)
    {
        super (msg);
    }
}
