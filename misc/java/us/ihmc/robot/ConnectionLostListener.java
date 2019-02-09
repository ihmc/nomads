
/**
 * ConnectionLostListener is an inteface used to catch an event when
 * the robot is unexpectedly disconnected.
 */
public interface ConnectionLostListener
{
    /**
     * Called when the robot is unexpectedly disconnected.
     */
    public void connectionLost();
}
