package us.ihmc.aci.nodemon.proxy;

import java.io.IOException;

/**
 * <code>NodeMonProxyServer.java</code>
 * <p/>
 * Interface <code>NodeMonProxyServer</code> contains the API for connections towards clients.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public interface NodeMonProxyServer
{
    void init () throws IOException;

    void start ();

    NodeMonProxyServerChild getServerChild (short clientId);

    int getChildrenSize ();
}
