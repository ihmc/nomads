/*
 * URI.java
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 (GPLv3) as published by the Free Software Foundation.
 *
 * U.S. Government agencies and organizations may redistribute
 * and/or modify this program under terms equivalent to
 * "Government Purpose Rights" as defined by DFARS 
 * 252.227-7014(a)(12) (February 2014).
 *
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 */

package us.ihmc.util;

import java.util.StringTokenizer;
import java.io.Serializable;

/**
 * The URI class takes an URI string and parses it in order to get
 * the host, port and protocol strings.
 * 
 * @author: Maggie Breedy
 * @version: $ 1.0$
 */

public class URI implements Serializable
{

    private static final long serialVersionUID = 6561287150459724813L;

    /**
     * Constructs a URI from a String parameter of the form 
     * protocol://host:port
     * <p>
     * @param nomadsURI String in form of 'protocol://host:port'.
     */
    public URI (String nomadsURI)
    {
        StringTokenizer st = new StringTokenizer (nomadsURI, ":");
        while (st.hasMoreTokens()) {
            _protocol = st.nextToken().trim();
            String str = st.nextToken().trim();
            StringTokenizer st2 = new StringTokenizer (str, "//");
            while (st2.hasMoreTokens()) {
                _host = st2.nextToken().trim();
            }
            String str3 = st.nextToken();
            if (str3.endsWith("/")) {
                str3 = str3.replace('/', ' ');
            }
            _port = str3.trim();
        }
    }
    
    /**
     * Returns host name or IP.
     * <p>
     * @return a String of host.
     */
    public String getHost()
    {
        return _host;
    }
     
    /**
     * Returns port number.
     * <p>
     * @return a String containing port number.
     */
    public String getPort()
    {
        return _port;
    }
    
    /**
     * Returns type of protocol.
     * <p>
     * @return a String denoting protocol, i.e. tcp or udp.
     */
    public String getProtocol()
    {
        return _protocol;
    }
    
    /**
     * Returns a String in format of 'protocol://host:port'.
     * <p>
     * @return a String formatted protocol, host and port.
     */
    public String toExternalForm()
    {
        String sNomadsURI = _protocol + "://";
        sNomadsURI = sNomadsURI + _host + ":" + _port;
        return (sNomadsURI);
    }
	
    /**
     * Returns true if protocol, host and port of the two URI objects are same.
     * <p>
     * @param uri URI object to be compared to calling URI object.
     * @return true if objects are equal.
     */
	public boolean equals (URI uri)
	{
		if (!_protocol.equals (uri.getProtocol())) {
			return false;
		}
		else if (!_host.equals (uri.getHost())) {
			return false;
		}
		else if (!_port.equals (uri.getPort())) {
			return false;
		}
		return true;
	}
    
    // Class variables    
/*    protected String _host;
    protected String _port;
    protected String _protocol;
*/    
    private String _host;
    private String _port;
    private String _protocol;
}
