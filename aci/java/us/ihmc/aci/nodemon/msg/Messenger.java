package us.ihmc.aci.nodemon.msg;

/**
 * Messenger.java
 * <p>
 * Interface <code>Messenger</code> contains public APIs to send messages between nodes.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public interface Messenger
{
    void broadcastMessage (String groupName, byte[] message);

    void sendMessage (String nodeId, byte[] message);

    void onMessage (String groupName, String nodeId, byte[] message);
}
