package us.ihmc.gst.util;

/**
 *
 * @author infofed
 */
public class ThreadHandler implements Thread.UncaughtExceptionHandler
{

    public void uncaughtException(Thread t, Throwable e) {
        synchronized (_obj) {
            System.err.println ("Thread" + t.getName() + " terminated " + e.getStackTrace());
        }
    }

    private Object _obj; 
}
