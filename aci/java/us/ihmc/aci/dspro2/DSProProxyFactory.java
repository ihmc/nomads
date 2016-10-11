/*
 * DSProProxyFactory.java
 *
 * This file is part of the IHMC ACI Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.aci.dspro2;

import us.ihmc.aci.util.dspro.test.DSProTCPBridge;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class DSProProxyFactory
{
    public enum Type
    {
        Proxy,
        SUDAProxy,
        TCPBridge;

        public static boolean hasType (String s)
        {
            for (Type t : Type.values()) {
                if (t.toString().compareTo (s) == 0) {
                    return true;
                }
            }
            return false;
        }
    }

    /**
     * Instantiate, initialize and start (when it applies) the proper instance
     * of DSProProxy
     *
     * @param type
     * @return 
     */
    public static DSProProxy getProxy (Type type)
    {
        switch (type) {
            case SUDAProxy:
                return getProxy (type, new SUDADSProProxy());

            case TCPBridge:
                return getProxy (type, new DSProTCPBridge());

            case Proxy:
            default:
                return getProxy (type, new DSProProxy());
        }
    }

    public static DSProProxy getProxy (Type type, short applicationId)
    {
        switch (type) {
            case SUDAProxy:
                return getProxy (type, new SUDADSProProxy (applicationId));

            case TCPBridge:
                return getProxy (type, new DSProTCPBridge (applicationId));

            case Proxy:
            default:
                return getProxy (type, new DSProProxy (applicationId));
        }
    }

    public static DSProProxy getProxy (Type type, short applicationId, String host, int iPort)
    {
        return getProxy (type, applicationId, host, iPort, false);
    }

    public static DSProProxy getProxy (Type type, short applicationId, String host, int iPort, boolean bPermissive)
    {
        switch (type) {
            case SUDAProxy:
                return getProxy (type, new SUDADSProProxy (applicationId, host, iPort), bPermissive);

            case TCPBridge:
                return getProxy (type, new DSProTCPBridge (applicationId, host, iPort), bPermissive);

            case Proxy:
            default:
                return getProxy (type, new DSProProxy (applicationId, host, iPort), bPermissive);
        }
    }

    private static DSProProxy getProxy (Type type, DSProProxy proxy)
    {
        return getProxy (type, proxy, false);
    }

    private static DSProProxy getProxy (Type type, DSProProxy proxy, boolean bPermissive)
    {
        int rc = proxy.init();
        if (rc != 0 && !bPermissive) {
            throw new DSProProxy.DSProProxyNotInitializedException ("Could not initialize proxy. Returned code " + rc);
        }
        switch (type) {
            case TCPBridge: {
                new Thread ((Runnable) proxy).start();
            }
        }
        return proxy;
    }      
}
