package us.ihmc.aci.netSupervisor.inferenceModule;

import us.ihmc.aci.ddam.NetworkEvent;

import java.util.List;

/**
 * SolverLauncher.java
 * <p/>
 * Interface <code>SolverLauncher</code> defines an API for different type of solvers.
 *
 * @author Emanuele Tagliaferro (etagliaferro@ihmc.us)
 */

public interface SolverLauncher
{

    /**
     * Finds the traffic entries with a new priority level
     *
     * @return false in case of errors
     */
    public List<ClaspTraffic> calculateNewPriorityClassification (List<ClaspTraffic> inputTraffic,
                                                                  List<ClaspHostDescription> inputHostsDescription);

    /**
     * Finds the targets of the events
     *
     * @return false in case of errors
     */
    public List<ClaspGroup> findEventsTargets (List<ClaspEvent> events);

    /**
     * Creates the default events for a specific tag
     *
     * @return false in case of errors
     */
    public boolean createClaspTag (String tagName, List<ClaspTraffic> relatedTraffic);

    /**
     * Creates the defalut events for a specific tag
     *
     * @return false in case of errors
     */
    public boolean createEvent (ClaspEvent event, List<ClaspEvent> relatedEvents);

    /**
     * Updates the group for the link type
     *
     * @return the group id (< 0 in case of errors)
     */
    public boolean updateLinkTypeGroup (String ipList, String linkType);

    /**
     * Init the solver with some parameters
     *
     * @return false in case of errors
     */
    public boolean initSolver(String lpSourcesPath);

    /**
     * get all the actions entries after the last update
     *
     * @return all the current actions after the last update
     */
    public List<ClaspAction> getAllCurrentActions ();

    /**
     * Init the solver with some parameters
     *
     * @return restore all the groups and the traffic entries
     */
    public boolean clearAll();
}
