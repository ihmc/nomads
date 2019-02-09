package us.ihmc.aci.nodemon.proxy;

/**
 * NodeMonProxy.java
 * <p/>
 * Interface <code>NodeMonProxy</code> define methods that allow a client to connect to the <code>NodeMon</code> API
 * and retrieve information through a series of asynchronous callbacks.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public interface NodeMonProxy
{

    /**
     * Connects this <code>NodeMonProxy</code> to the <code>NodeMon</code> instance running at
     * <code>host</code> and <code>port</code> specified in the constructor.
     *
     * @return <code>true</code> if the <code>connect()</code> was successful, <code>false</code> otherwise.
     */
    boolean connect ();

    /**
     * Returns the connection status of this <code>NodeMonProxy</code>
     *
     * @return <code>true</code> if this <code>NodeMonProxy</code> is currently connected, <code>false</code>
     * otherwise
     */
    boolean isConnected ();

    /**
     * Registers a <code>NodeMonProxyListener</code> to receive updates about the status of the world.
     *
     * @param listener an instance of <code>NodeMonProxyListener</code>
     */
    void registerNodeMonProxyListener (NodeMonProxyListener listener);

    /**
     * Forwards a request to the NodeMon to asynchronously receive an up-to-date state of the world (collection of the
     * nodes that are currently up and monitored).
     *
     * @return <code>true</code> if the request was successfully forwarded to the NodeMon, <code>false</code> otherwise
     */
    boolean getWorldState ();
}
