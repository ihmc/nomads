package us.ihmc.aci.kernel;

/**
 * Interface constants for each possible state of a service instance
 *
 * @author rquitadamo
 */
interface ServiceInstanceStates {

    public static final int DEPLOYED = 0;
    public static final int ACTIVATED = 1;
    public static final int MIGRATING = 2;
    public static final int MIGRATED = 3;
    public static final int DEACTIVATED = 4;
    public static final int RESTORING = 5;

    public static final String stateNames[] =
    {
        "DEPLOYED", "ACTIVATED", "MIGRATING", "MIGRATED", "DEACTIVATED", "RESTORING"
    };

}
