package us.ihmc.aci.disServiceProProxy;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class DisServiceProProxyFactory
{
    public enum Type {
        Proxy,
        AsyncProxy,
        SUDAProxy,
        AsyncSUDAProxy
    }

    /**
     * Instantiate, initialize and start (when it applies) the proper instance
     * of DisServiceProProxy
     *
     * @param type
     * @return
     * @throws Exception 
     */
    public static DisServiceProProxyInterface getProxy (Type type)
        throws Exception
    {
        switch (type) {
            case Proxy: {
                DisServiceProProxy proxy = new DisServiceProProxy();
                int rc = proxy.init();
                if (rc != 0) {
                    throw new Exception ("Could not initialize proxy. Returned code " + rc);
                }
                return proxy;
            }

            case AsyncProxy: {
                AsyncDisServiceProProxy proxy = new AsyncDisServiceProProxy();
                int rc = proxy.init();
                if (rc != 0) {
                    throw new Exception ("Could not initialize proxy. Returned code " + rc);
                }
                return proxy;
            }

            case SUDAProxy: {
                SUDADisServiceProProxy proxy = new SUDADisServiceProProxy();
                int rc = proxy.init();
                if (rc != 0) {
                    throw new Exception ("Could not initialize proxy. Returned code " + rc);
                }
                return proxy;
            }
    
            case AsyncSUDAProxy: {
                AsyncSUDADisServiceProProxy proxy = new AsyncSUDADisServiceProProxy();
                int rc = proxy.init();
                if (rc != 0) {
                    throw new Exception ("Could not initialize proxy. Returned code " + rc);
                }
                return proxy;
            }

        }

        return null;
    }

    public static DisServiceProProxyInterface getProxy (Type type, short applicationId)
        throws Exception
    {
        switch (type) {
            case Proxy: {
                DisServiceProProxy proxy = new DisServiceProProxy (applicationId);
                int rc = proxy.init();
                if (rc != 0) {
                    throw new Exception ("Could not initialize proxy. Returned code " + rc);
                }
                return proxy;
            }

            case AsyncProxy: {
                AsyncDisServiceProProxy proxy = new AsyncDisServiceProProxy (applicationId);
                int rc = proxy.init();
                if (rc != 0) {
                    throw new Exception ("Could not initialize proxy. Returned code " + rc);
                }
                return proxy;
            }

            case SUDAProxy: {
                SUDADisServiceProProxy proxy = new SUDADisServiceProProxy (applicationId);
                int rc = proxy.init();
                if (rc != 0) {
                    throw new Exception ("Could not initialize proxy. Returned code " + rc);
                }
                return proxy;
            }

            case AsyncSUDAProxy: {
                AsyncSUDADisServiceProProxy proxy = new AsyncSUDADisServiceProProxy (applicationId);
                int rc = proxy.init();
                if (rc != 0) {
                    throw new Exception ("Could not initialize proxy. Returned code " + rc);
                }
                return proxy;
            }

        }

        return null;
    }
    
    public static DisServiceProProxyInterface getProxy (Type type, short applicationId,
                                                        String host, int iPort)
        throws Exception
    {
        switch (type) {
            case Proxy: {
                DisServiceProProxy proxy = new DisServiceProProxy (applicationId, host, iPort);
                int rc = proxy.init();
                if (rc != 0) {
                    throw new Exception ("Could not initialize proxy. Returned code " + rc);
                }
                return proxy;
            }

            case AsyncProxy: {
                AsyncDisServiceProProxy proxy = new AsyncDisServiceProProxy (applicationId, host, iPort);
                int rc = proxy.init();
                if (rc != 0) {
                    throw new Exception ("Could not initialize proxy. Returned code " + rc);
                }
                return proxy;
            }

            case SUDAProxy: {
                SUDADisServiceProProxy proxy = new SUDADisServiceProProxy (applicationId, host, iPort);
                int rc = proxy.init();
                if (rc != 0) {
                    throw new Exception ("Could not initialize proxy. Returned code " + rc);
                }
                return proxy;
            }
    
            case AsyncSUDAProxy: {
                AsyncSUDADisServiceProProxy proxy = new AsyncSUDADisServiceProProxy (applicationId, host, iPort);
                int rc = proxy.init();
                if (rc != 0) {
                    throw new Exception ("Could not initialize proxy. Returned code " + rc);
                }
                return proxy;
            }
        }

        return null;
    }

}
