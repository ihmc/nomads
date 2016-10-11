/*
 * CertInfo.java
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

/** Serves to store info related to a particular X509v3 certificate, including the DER encoded Certificate itself, the Name - presently a string
 * that is unique and serves as its identifier in MAST, as well as the IP address from which this particular Cert/Name was last seen at.
*/

public class CertInfo
{
    public CertInfo()
    {
    }
    
    public CertInfo (String name, String IPAddress, byte[] DERCert)
    {
        _name = name;
        _IPAddress = IPAddress;
        _DERCert = DERCert;
    }
    
    public String getName ()
    {
        return _name;
    }
    
    public String getIPAddress ()
    {
        return _IPAddress;
    }
    
    public byte[] getDERCert ()
    {
        return _DERCert;
    }
    
    public void setName (String name)
    {
        _name = name;
    }
    
    public void setIPAddress (String IPAddress)
    {
        _IPAddress = IPAddress;
    }
    
    public void setDERCert (byte[] DERCert)
    {
        _DERCert = DERCert;
    }

//    private final static int debug = 0;
    private String _name = "";
    private String _IPAddress = "";
    private byte[] _DERCert = null;
}
