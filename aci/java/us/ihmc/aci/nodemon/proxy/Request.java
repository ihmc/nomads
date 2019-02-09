package us.ihmc.aci.nodemon.proxy;

/**
 * Class <code>Request</code> lists all types of request to and from the proxy.
 *
 * @author Enrico Casini (ecasin@ihmc.us)
 */
enum Request
{
    confirm,
    error,
    registerProxy,
    registerProxyCallback,
    worldState,
    worldStateUpdate,
    updateData,
}

