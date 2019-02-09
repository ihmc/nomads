package us.ihmc.aci.nodemon.scheduler;

import us.ihmc.aci.nodemon.proxy.NodeMonProxyListener;

/**
 * ProxyScheduler.java
 * <p>
 * Interface <code>ProxyScheduler</code> defines methods for a module responsible for scheduling message updates
 * about the current WorldState to the proxy module.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public interface ProxyScheduler extends Scheduler {

    /**
     * Gets the number of currently registered proxies.
     *
     * @return a count of currently registered proxies
     */
    int getProxyListenersCount();

    /**
     * Adds a new <code>NodeMonProxyListener</code> to this <code>ProxyScheduler</code>.
     *
     * @param listener an instance of <code>NodeMonProxyListener</code>
     */
    void addNodeMonProxyListener(NodeMonProxyListener listener);

    /**
     * Removes the specified <code>NodeMonProxyListener</code> from this <code>ProxyScheduler</code>.
     *
     * @param listener an instance of <code>NodeMonProxyListener</code>
     */
    void removeNodeMonProxyListener(NodeMonProxyListener listener);
}
