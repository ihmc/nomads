package us.ihmc.aci.nodemon.scheduler;

import us.ihmc.aci.ddam.Container;

/**
 * Scheduler.java
 * <p/>
 * Interface <code>Scheduler</code> defines methods for a module responsible for scheduling message updates about the
 * current WorldState to other nodes.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public interface Scheduler
{
    void start ();

    void stop ();

    boolean addOutgoingMessage (Container c);

    boolean addIncomingMessage (Container c);
}
