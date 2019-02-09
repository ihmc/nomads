package us.ihmc.aci.nodemon;

/**
 * Controller.java
 * <p/>
 * Interface <code>Controller</code> defines an API shared between all controllers.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public interface Controller
{
    /**
     * Returns whether this <code>Controller</code> is running.
     *
     * @return true if the scheduler is running, false otherwise.
     */
    boolean isRunning();

    /**
     * Starts this <code>Controller</code>.
     */
    void start ();

    /**
     * Stops this <code>Controller</code>.
     */
    void stop ();

}
