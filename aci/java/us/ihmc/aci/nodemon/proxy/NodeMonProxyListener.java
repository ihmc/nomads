package us.ihmc.aci.nodemon.proxy;

import us.ihmc.aci.ddam.Node;

import java.util.Collection;

/**
 * NodeMonProxyListener.java
 * <p>
 * Class <code>NodeMonProxyListener</code> contains a
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public interface NodeMonProxyListener {

    /**
     * Callback. Returns a collection of the <code>Node</code> instances of the <code>WorldState</code>,
     * after the proxy has explicitly requested it with <code>getWorldState()</code>.
     *
     * @param worldState a collection of the <code>Node</code> instances of the <code>WorldState</code>.
     */
    void worldStateUpdate(Collection<Node> worldState);

    /**
     * Callback. Returns a serialized <code>Container</code> including a specific type of data, as specified
     * by the <code>DataType</code> inside the <code>Container</code>
     *
     * @param nodeId the id of the <code>Node</code> related to this update
     * @param data the data in the form of a Container
     */
    void updateData(String nodeId, byte[] data);

    /**
     * Callback. Reports that the connection has been closed from the Server.
     */
    void connectionClosed();
}
